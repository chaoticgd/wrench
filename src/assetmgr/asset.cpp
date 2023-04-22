/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "asset.h"

#include <algorithm>
#include <filesystem>

#include "asset_types.h"

#ifndef _MSC_VER
#include <unistd.h> // getpid
#endif

Asset::Asset(AssetFile& file, Asset* parent, AssetType type, std::string tag, AssetDispatchTable& func_table)
	: funcs(func_table)
	, _type(type)
	, _file(file)
	, _parent(parent)
	, _tag(tag) {}

Asset::~Asset() {
	disconnect_precedence_pointers();
}

AssetForest& Asset::forest() { return file()._forest; }
const AssetForest& Asset::forest() const { return file()._forest; }
AssetBank& Asset::bank() { return file()._bank; }
const AssetBank& Asset::bank() const { return file()._bank; }
AssetFile& Asset::file() { return _file; }
const AssetFile& Asset::file() const { return _file; }
Asset* Asset::parent() { return _parent; }
const Asset* Asset::parent() const { return _parent; }
const std::string& Asset::tag() const { return _tag; }
Asset* Asset::lower_precedence() { return _lower_precedence; }
const Asset* Asset::lower_precedence() const { return _lower_precedence; }
Asset* Asset::higher_precedence() { return _higher_precedence; }
const Asset* Asset::higher_precedence() const { return _higher_precedence; }

Asset& Asset::lowest_precedence() {
	Asset* asset = this;
	while(asset->lower_precedence() != nullptr) {
		asset = asset->lower_precedence();
	}
	return *asset;
}

const Asset& Asset::lowest_precedence() const {
	return const_cast<Asset&>(*this).lowest_precedence();
}

Asset& Asset::highest_precedence() {
	Asset* asset = this;
	while(asset->higher_precedence() != nullptr) {
		asset = asset->higher_precedence();
	}
	return *asset;
}

const Asset& Asset::highest_precedence() const {
	return const_cast<Asset&>(*this).highest_precedence();
}

AssetLink Asset::absolute_link() const {
	if(parent()) {
		AssetLink link = parent()->absolute_link();
		link.add_tag(tag().c_str());
		return link;
	} else {
		return AssetLink();
	}
}

AssetLink Asset::link_relative_to(Asset& base) const {
	if(parent()) {
		bool match = false;
		for(Asset* asset = &base.highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
			if(asset == parent()) {
				match = true;
				break;
			}
		}
		AssetLink link;
		if(match) {
			link.add_prefix(asset_type_to_string(parent()->logical_type()));
		} else {
			link = parent()->link_relative_to(base);
		}
		link.add_tag(tag().c_str());
		return link;
	} else {
		return AssetLink();
	}
}

AssetType Asset::physical_type() const {
	return _type;
}

AssetType Asset::logical_type() const {
	for(const Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
		if(asset->physical_type() != PlaceholderAsset::ASSET_TYPE) {
			return asset->physical_type();
		}
	}
	return physical_type();
}

Asset& Asset::as(AssetType type) {
	if(Asset* asset = maybe_as(type)) {
		return *asset;
	}
	verify_not_reached("Failed to convert asset %s to type %s.",
		absolute_link().to_string().c_str(),
		asset_type_to_string(type));
}

Asset* Asset::maybe_as(AssetType type) {
	for(Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
		if(asset->physical_type() == type) {
			return asset;
		} else if(asset->physical_type() != PlaceholderAsset::ASSET_TYPE) {
			break;
		}
	}
	return nullptr;
}

bool Asset::has_child(const char* tag) const {
	for(const Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
		for(const std::unique_ptr<Asset>& child : asset->_children) {
			if(child->tag() == tag && !child->is_deleted()) {
				return true;
			}
		}
	}
	return false;
}

bool Asset::has_child(s32 tag) const {
	std::string str = std::to_string(tag);
	return has_child(str.c_str());
}

Asset& Asset::get_child(const char* tag) {
	for(Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
		for(std::unique_ptr<Asset>& child : asset->_children) {
			if(child->tag() == tag) {
				return child->resolve_references();
			}
		}
	}
	verify_not_reached("No child of \"%s\" with tag \"%s\".",
		absolute_link().to_string().c_str(), tag);
}

const Asset& Asset::get_child(const char* tag) const {
	return const_cast<Asset&>(*this).get_child(tag);
}

Asset& Asset::get_child(s32 tag) {
	std::string str = std::to_string(tag);
	return get_child(str.c_str());
}

const Asset& Asset::get_child(s32 tag) const {
	return const_cast<Asset&>(*this).get_child(tag);
}

Asset& Asset::physical_child(AssetType type, const char* tag) {
	// Hitting this verify_fatal in packing code means you probably meant to use
	// the get_child function (or a get_<child name> function) instead.
	verify_fatal(bank().is_writeable());
	for(std::unique_ptr<Asset>& child : _children) {
		if(child->tag() == tag) {
			return *child.get();
		}
	}
	return add_child(create_asset(type, file(), this, tag));
}

Asset* Asset::get_physical_child(const char* tag) {
	for(std::unique_ptr<Asset>& child : _children) {
		if(child->tag() == tag) {
			return child.get();
		}
	}
	return nullptr;
}

bool Asset::remove_physical_child(Asset& asset) {
	for(auto child = _children.begin(); child != _children.end(); child++) {
		if(child->get() == &asset) {
			_children.erase(child);
			return true;
		}
	}
	return false;
}

Asset& Asset::foreign_child_impl(const fs::path& path, bool is_absolute, AssetType type, const char* tag) {
	AssetLink link = absolute_link();
	Asset* asset = &bank().asset_file(is_absolute ? path.relative_path() : file()._relative_directory/path).root();
	auto [prefix, tags] = link.get();
	for(const char* tag : tags) {
		asset = &asset->physical_child(PlaceholderAsset::ASSET_TYPE, tag);
	}
	return asset->physical_child(type, tag);
}

void Asset::read(WtfNode* node) {
	const WtfAttribute* strongly_deleted = wtf_attribute(node, "strongly_deleted");
	if(strongly_deleted && strongly_deleted->type == WTF_BOOLEAN) {
		flags |= ASSET_HAS_STRONGLY_DELETED_FLAG;
		if(strongly_deleted->boolean) {
			flags |= ASSET_IS_STRONGLY_DELETED;
		}
	}
	const WtfAttribute* weakly_deleted = wtf_attribute(node, "weakly_deleted");
	if(weakly_deleted && weakly_deleted->type == WTF_BOOLEAN && weakly_deleted->boolean) {
		flags |= ASSET_IS_WEAKLY_DELETED;
	}
	read_attributes(node);
	for(WtfNode* child = node->first_child; child != nullptr; child = child->next_sibling) {
		// Determine the type of the asset.
		AssetType type;
		if(strlen(child->type_name) == 0) {
			if(child->collapsed) {
				type = PlaceholderAsset::ASSET_TYPE;
			} else {
				type = CollectionAsset::ASSET_TYPE;
			}
		} else {
			type = asset_string_to_type(child->type_name);
			verify(type != NULL_ASSET_TYPE, "Invalid asset type '%s'.", child->type_name);
		}
		
		Asset* asset = nullptr;
		
		// Handle the case where the same asset id defined multiple times in the
		// same file.
		for(std::unique_ptr<Asset>& asset_child : _children) {
			if(asset_child->tag() == child->tag) {
				asset = asset_child.get();
				break;
			}
		}
		
		// If the asset hasn't been defined before in this file, create it.
		if(asset == nullptr) {
			asset = &add_child(create_asset(type, file(), this, child->tag));
		}
		
		// Read its attributes and child assets.
		asset->read(child);
	}
}

void Asset::write(WtfWriter* ctx, std::string prefix) const {
	if(_attrib_exists == 0 && _children.size() == 1 && physical_type() == PlaceholderAsset::ASSET_TYPE) {
		Asset& child = *_children[0].get();
		child.write(ctx, prefix + tag() + ".");
	} else {
		const char* type_name;
		if(logical_type() == CollectionAsset::ASSET_TYPE) {
			type_name = nullptr;
		} else {
			type_name = asset_type_to_string(logical_type());
		}
		std::string qualified_tag = prefix + tag();
		wtf_begin_node(ctx, type_name, qualified_tag.c_str());
		write_body(ctx);
		wtf_end_node(ctx);
	}
}

void Asset::write_body(WtfWriter* ctx) const {
	if(flags & ASSET_HAS_STRONGLY_DELETED_FLAG) {
		wtf_begin_attribute(ctx, "strongly_deleted");
		wtf_write_boolean(ctx, (flags & ASSET_IS_STRONGLY_DELETED) != 0);
		wtf_end_attribute(ctx);
	}
	if(flags & ASSET_IS_WEAKLY_DELETED) {
		wtf_begin_attribute(ctx, "weakly_deleted");
		wtf_write_boolean(ctx, (flags & ASSET_IS_WEAKLY_DELETED) != 0);
		wtf_end_attribute(ctx);
	}
	write_attributes(ctx);
	for_each_physical_child([&](Asset& child) {
		child.write(ctx, std::string());
	});
}

void Asset::validate() const {
	validate_attributes();
	for_each_physical_child([&](Asset& child) {
		child.validate();
	});
}

bool Asset::weakly_equal(const Asset& rhs) const {
	if(this == &rhs) {
		return true;
	}
	for(Asset* lower = _lower_precedence; lower != nullptr; lower = lower->_lower_precedence) {
		if(lower == &rhs) {
			return true;
		}
	}
	for(Asset* higher = _higher_precedence; higher != nullptr; higher = higher->_higher_precedence) {
		if(higher == &rhs) {
			return true;
		}
	}
	return false;
}

void Asset::rename(std::string new_tag) {
	verify_fatal(parent());
	verify_fatal(_children.size() == 0); // TODO: Do something *fancy* with the precedence pointers to handle this case.
	disconnect_precedence_pointers();
	parent()->for_each_logical_child([&](Asset& asset) {
		verify(asset.tag() != new_tag || &asset == this, "Asset with new tag already exists.");
	});
	_tag = std::move(new_tag);
	connect_precedence_pointers();
}

bool Asset::is_deleted() const {
	if((highest_precedence().flags & ASSET_IS_WEAKLY_DELETED) != 0) {
		return true;
	}
	for(const Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
		if(asset->flags & ASSET_HAS_STRONGLY_DELETED_FLAG) {
			return (asset->flags & ASSET_IS_STRONGLY_DELETED) != 0;
		}
	}
	return false;
}

Asset& Asset::add_child(std::unique_ptr<Asset> child) {
	verify_fatal(child.get());
	Asset& asset = *_children.emplace_back(std::move(child)).get();
	asset.connect_precedence_pointers();
	return asset;
}

Asset& Asset::resolve_references() {
	Asset* asset = &highest_precedence();
	verify(!asset->is_deleted(), "Asset '%s' is deleted.", asset->absolute_link().to_string().c_str());
	while(ReferenceAsset* reference = asset->maybe_as<ReferenceAsset>()) {
		asset = &forest().lookup_asset(reference->asset(), asset->parent());
		verify(!asset->is_deleted(), "Tried to find deleted asset \"%s\".", reference->asset().to_string().c_str());
		verify(asset, "Failed to find asset \"%s\".", reference->asset().to_string().c_str());
	}
	return *asset;
}


void Asset::connect_precedence_pointers() {
	// Connect asset nodes from adjacent trees so that if a given node doesn't
	// contain a given attribute we can check lower precedence nodes.
	if(parent()) {
		// Check for a lower precedence node first. This should be the more
		// common case while editing.
		for(Asset* lower_parent = parent()->lower_precedence(); lower_parent != nullptr; lower_parent = lower_parent->lower_precedence()) {
			verify_fatal(lower_parent != parent());
			if(Asset* lower = lower_parent->get_physical_child(tag().c_str())) {
				Asset* higher = lower->higher_precedence();
				_lower_precedence = lower;
				_higher_precedence = higher;
				verify_fatal(_lower_precedence != this);
				verify_fatal(_higher_precedence != this);
				lower->_higher_precedence = this;
				verify_fatal(lower->_higher_precedence != lower);
				if(higher) {
					higher->_lower_precedence = this;
					verify_fatal(higher->_lower_precedence != higher);
				}
				return;
			}
		}
		// There was no lower precedence node, so now check if there's is a
		// higher precedence node.
		for(Asset* higher_parent = parent()->higher_precedence(); higher_parent != nullptr; higher_parent = higher_parent->higher_precedence()) {
			verify_fatal(higher_parent != parent());
			if(Asset* higher = higher_parent->get_physical_child(tag().c_str())) {
				Asset* lower = higher->lower_precedence();
				_lower_precedence = lower;
				_higher_precedence = higher;
				verify_fatal(_lower_precedence != this);
				verify_fatal(_higher_precedence != this);
				if(lower) {
					lower->_higher_precedence = this;
					verify_fatal(lower->_higher_precedence != lower);
				}
				higher->_lower_precedence = this;
				verify_fatal(higher->_lower_precedence != higher);
				return;
			}
		}
	} else {
		AssetFile* lower = file().lower_precedence();
		if(lower) {
			_lower_precedence = &lower->root();
			verify_fatal(_lower_precedence != &file().root());
			lower->root()._higher_precedence = this;
		}
		AssetFile* higher = file().higher_precedence();
		if(higher) {
			_higher_precedence = &higher->root();
			verify_fatal(_higher_precedence != &file().root());
			higher->root()._lower_precedence = this;
		}
	}
}

void Asset::disconnect_precedence_pointers() {
	if(lower_precedence()) {
		_lower_precedence->_higher_precedence = higher_precedence();
		verify_fatal(_lower_precedence->_higher_precedence != _lower_precedence);
	}
	if(higher_precedence()) {
		_higher_precedence->_lower_precedence = lower_precedence();
		verify_fatal(_higher_precedence->_lower_precedence != _higher_precedence);
	}
}

// *****************************************************************************

AssetFile::AssetFile(AssetForest& forest, AssetBank& pack, const fs::path& relative_path)
	: _forest(forest)
	, _bank(pack)
	, _relative_directory(relative_path.parent_path())
	, _file_name(relative_path.filename().string())
	, _root(std::make_unique<RootAsset>(*this, nullptr, "")) {}

Asset& AssetFile::root() {
	verify_fatal(_root.get());
	return *_root.get();
}

void AssetFile::write() const {
	std::string dest;
	WtfWriter* ctx = wtf_begin_file(dest);
	defer([&]() {
		wtf_end_file(ctx);
	});
	_root->write_body(ctx);
	_bank.write_text_file(_relative_directory/_file_name, dest.c_str());
}

std::unique_ptr<InputStream> AssetFile::open_binary_file_for_reading(const FileReference& reference, fs::file_time_type* modified_time_dest) const {
	verify_fatal(reference.owner == this);
	return _bank.open_binary_file_for_reading(_relative_directory/reference.path, modified_time_dest);
}

std::pair<std::unique_ptr<OutputStream>, FileReference> AssetFile::open_binary_file_for_writing(const fs::path& path) const {
	return {_bank.open_binary_file_for_writing(_relative_directory/path), FileReference(*this, path)};
}

std::string AssetFile::read_text_file(const fs::path& path) const {
	return _bank.read_text_file(_relative_directory/path);
}

FileReference AssetFile::write_text_file(const fs::path& path, const char* contents) const {
	_bank.write_text_file(_relative_directory/path, contents);
	return FileReference(*this, path);
}

bool AssetFile::file_exists(const fs::path& path) const {
	return _bank.file_exists(_relative_directory/path);
}

AssetFile* AssetFile::lower_precedence() {
	verify_fatal(_bank._asset_files.size() > 0);
	for(size_t i = 1; i < _bank._asset_files.size(); i++) {
		if(_bank._asset_files[i].get() == this) {
			return _bank._asset_files[i - 1].get();
		}
	}
	for(AssetBank* lower = _bank.lower_precedence(); lower != nullptr; lower = lower->lower_precedence()) {
		if(lower->_asset_files.size() >= 1) {
			return lower->_asset_files.back().get();
		}
	}
	return nullptr;
}

AssetFile* AssetFile::higher_precedence() {
	verify_fatal(_bank._asset_files.size() > 0);
	for(size_t i = 0; i < _bank._asset_files.size() - 1; i++) {
		if(_bank._asset_files[i].get() == this) {
			return _bank._asset_files[i + 1].get();
		}
	}
	for(AssetBank* higher = _bank.higher_precedence(); higher != nullptr; higher = higher->higher_precedence()) {
		if(higher->_asset_files.size() >= 1) {
			return higher->_asset_files.front().get();
		}
	}
	return nullptr;
}

Asset& AssetFile::asset_from_link(AssetType type, const AssetLink& link) {
	Asset* asset = &root();
	auto [prefix, tags] = link.get();
	for(size_t i = 0; i < tags.size(); i++) {
		AssetType current_type = (i == tags.size() - 1)
			? type : PlaceholderAsset::ASSET_TYPE;
		asset = &asset->physical_child(current_type, tags[i]);
	}
	return *asset;
}

void AssetFile::read() {
	fs::path path = _relative_directory/_file_name;
	std::string path_str = path.string();
	std::string text = _bank.read_text_file(path);
	char* mutable_text = new char[text.size() + 1];
	memcpy(mutable_text, text.c_str(), text.size() + 1);
	text.clear();
	char* error_dest = nullptr;
	WtfNode* root_node = wtf_parse(mutable_text, &error_dest);
	verify(!error_dest, "syntax error in %s: %s", path_str.c_str(), error_dest);
	defer([&]() {
		wtf_free(root_node);
		delete[] mutable_text;
	});
	root().connect_precedence_pointers();
	root().read(root_node);
}

// *****************************************************************************

AssetBank::AssetBank(AssetForest& forest, bool is_writeable)
	: _forest(forest)
	, _is_writeable(is_writeable) {}

AssetBank::~AssetBank() {
	if(_unlocker) {
		_unlocker();
	}
}

std::string AssetBank::read_text_file(const FileReference& reference) const {
	return read_text_file(reference.owner->_relative_directory/reference.path);
}

bool AssetBank::is_writeable() const {
	return _is_writeable;
}

AssetFile& AssetBank::asset_file(fs::path path) {
	if(path.is_absolute()) {
		// Handle absolute paths generated by save dialogs.
		LooseAssetBank* bank = dynamic_cast<LooseAssetBank*>(this);
		verify(bank, "Tried to create an asset file from an absolute path in a non-loose bank.");
		path = fs::relative(path, bank->_directory);
	}
	path.replace_extension("asset");
	fs::path relative_directory = path.parent_path();
	fs::path file_name = path.filename();
	for(std::unique_ptr<AssetFile>& file : _asset_files) {
		if(file->_relative_directory == relative_directory && file->_file_name == file_name) {
			return *file.get();
		}
	}
	AssetFile& file = *_asset_files.emplace_back(std::make_unique<AssetFile>(_forest, *this, path)).get();
	file.root().connect_precedence_pointers();
	return file;
}

void AssetBank::write() {
	std::string game_info_str;
	write_game_info(game_info_str, game_info);
	write_text_file("gameinfo.txt", game_info_str.c_str());
	
	for(const std::unique_ptr<AssetFile>& file : _asset_files) {
		file->write();
	}
}

AssetBank* AssetBank::lower_precedence() { return _lower_precedence; }
AssetBank* AssetBank::higher_precedence() { return _higher_precedence; }

Asset* AssetBank::root() {
	if(_asset_files.size() >= 1) {
		return &_asset_files.back()->root().highest_precedence();
	} else {
		return nullptr;
	}
}

void AssetBank::read() {
	std::string game_info_txt = read_text_file("gameinfo.txt");
	if(!game_info_txt.empty()) {
		char* game_info_txt_mutable = new char[game_info_txt.size() + 1]; 
		defer([&]() { delete[] game_info_txt_mutable; });
		memcpy(game_info_txt_mutable, game_info_txt.c_str(), game_info_txt.size() + 1);
		game_info = read_game_info(game_info_txt_mutable);
	}
	
	std::vector<fs::path> asset_file_paths = enumerate_asset_files();
	std::sort(BEGIN_END(asset_file_paths));
	for(const fs::path& relative_path : asset_file_paths) {
		AssetFile& file = *_asset_files.emplace_back(std::make_unique<AssetFile>(_forest, *this, relative_path)).get();
		file.read();
	}
}

s32 AssetBank::check_lock() const { verify_fatal(0); return 0; }
void AssetBank::lock() { verify_fatal(0); }

// *****************************************************************************

Asset* AssetForest::any_root() {
	for(s32 i = (s32) _banks.size() - 1; i >= 0; i--) {
		Asset* root = _banks[i]->root();
		if(root) {
			return root;
		}
	}
	return nullptr;
}

const Asset* AssetForest::any_root() const {
	for(s32 i = (s32) _banks.size() - 1; i >= 0; i--) {
		Asset* root = _banks[i]->root();
		if(root) {
			return root;
		}
	}
	return nullptr;
}

Asset& AssetForest::lookup_asset(const AssetLink& link, Asset* context) {
	verify(_banks.size() >= 1 && _banks[0]->_asset_files.size() >= 1,
		"Asset lookup for '%s' failed because the asset forest is empty.",
		link.to_string().c_str());
	auto [prefix, tags] = link.get();
	bool matching_failed = false;
	Asset* asset = nullptr;
	if(prefix) {
		verify(context, "Tried to lookup a relative asset reference that can't be relative.");
		asset = context;
		while(asset && strcmp(prefix, asset_type_to_string(asset->logical_type())) != 0) {
			asset = asset->parent();
		}
	}
	if(!asset) {
		asset = &_banks[0]->_asset_files[0]->root();
		matching_failed = !!prefix;
	}
	try {
		for(const char* tag : tags) {
			asset = &asset->get_child(tag);
		}
	} catch(RuntimeError& e) {
		verify_not_reached("%s while looking up asset \"%s\"%s.",
			e.message.substr(0, e.message.size() - 1).c_str(),
			link.to_string().c_str(),
			matching_failed ? " where no ancestors matched the prefix" : "");
	}
	return *asset;
}

void AssetForest::unmount_last() {
	verify_fatal(_banks.size() >= 1);
	_banks.erase(_banks.end() - 1);
	_banks.back()->_higher_precedence = nullptr;
}

// *****************************************************************************

LooseAssetBank::LooseAssetBank(AssetForest& forest, fs::path directory, bool is_writeable)
	: AssetBank(forest, is_writeable)
	, _directory(directory) {
	if(is_writeable) {
		fs::create_directories(directory);
	}
}

std::unique_ptr<InputStream> LooseAssetBank::open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const {
	verify_fatal(path.is_relative());
	fs::path full_path = _directory/path;
	if(modified_time_dest) {
		*modified_time_dest = fs::last_write_time(full_path);
	}
	auto stream = std::make_unique<FileInputStream>();
	if(stream->open(full_path)) {
		return stream;
	} else {
		return nullptr;
	}
}

std::unique_ptr<OutputStream> LooseAssetBank::open_binary_file_for_writing(const fs::path& path) {
	verify_fatal(path.is_relative());
	verify_fatal(is_writeable());
	fs::path full_path = _directory/path;
	fs::create_directories(full_path.parent_path());
	auto stream = std::make_unique<FileOutputStream>();
	if(stream->open(full_path)) {
		return stream;
	} else {
		return nullptr;
	}
}

std::string LooseAssetBank::read_text_file(const fs::path& path) const {
	verify_fatal(path.is_relative());
	if(!fs::exists(_directory/path)) {
		return "";
	}
	auto bytes = read_file(_directory/path, "r");
	return std::string((char*) bytes.data(), bytes.size());
}

void LooseAssetBank::write_text_file(const fs::path& path, const char* contents) {
	verify_fatal(path.is_relative());
	verify_fatal(is_writeable());
	fs::create_directories((_directory/path).parent_path());
	write_file(_directory/path, Buffer((u8*) contents, (u8*) contents + strlen(contents)), "w");
}

bool LooseAssetBank::file_exists(const fs::path& path) const {
	verify_fatal(path.is_relative());
	return fs::exists(_directory/path);
}

std::vector<fs::path> LooseAssetBank::enumerate_asset_files() const {
	std::vector<fs::path> asset_files;
	for(auto& entry : fs::recursive_directory_iterator(_directory)) {
		if(entry.path().extension() == ".asset") {
			asset_files.emplace_back(fs::relative(entry.path(), _directory));
		}
	}
	return asset_files;
}

s32 LooseAssetBank::check_lock() const {
	if(fs::exists(_directory/"lock")) {
		std::string pid = read_text_file("lock");
		return atoi(pid.c_str());
	}
	return 0;
}

void LooseAssetBank::lock() {
	//s32 pid = getpid();
	//std::string pid_str = std::to_string(pid);
	//write_text_file("lock", pid_str.c_str());
	//fs::path dir = _directory;
	//_unlocker = [dir]() {
	//	fs::remove(dir/"lock");
	//};
}

// *****************************************************************************

MemoryAssetBank::MemoryAssetBank(AssetForest& forest)
	: AssetBank(forest, true) {}

std::unique_ptr<InputStream> MemoryAssetBank::open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const {
	return std::make_unique<MemoryInputStream>(_files.at(path));
}

std::unique_ptr<OutputStream> MemoryAssetBank::open_binary_file_for_writing(const fs::path& path) {
	return std::make_unique<MemoryOutputStream>(_files[path]);
}

std::string MemoryAssetBank::read_text_file(const fs::path& path) const {
	auto file = _files.find(path);
	if(file == _files.end()) {
		return "";
	} else {
		return std::string((char*) file->second.data(), file->second.size());
	}
}

void MemoryAssetBank::write_text_file(const fs::path& path, const char* contents) {
	_files[path] = std::vector<u8>(contents, contents + strlen(contents));
}

bool MemoryAssetBank::file_exists(const fs::path& path) const {
	return _files.find(path) == _files.end();
}

std::vector<fs::path> MemoryAssetBank::enumerate_asset_files() const {
	std::vector<fs::path> asset_files;
	for(auto& [path, contents] : _files) {
		if(path.extension() == ".asset") {
			asset_files.emplace_back(path);
		}
	}
	return asset_files;
}

s32 MemoryAssetBank::check_lock() const {
	return 0;
}

void MemoryAssetBank::lock() {}
