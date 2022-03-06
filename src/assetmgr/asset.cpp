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

#include "asset_types.h"

Asset::Asset(AssetForest& forest, AssetPack& pack, AssetFile& file, Asset* parent, AssetType type, std::string tag)
	: _forest(forest)
	, _pack(pack)
	, _file(file)
	, _parent(parent)
	, _type(type)
	, _tag(tag) {
	connect_precedence_pointers();
}

Asset::~Asset() {
	disconnect_precedence_pointers();
}

AssetForest& Asset::forest() { return _forest; }
const AssetForest& Asset::forest() const { return _forest; }
AssetPack& Asset::pack() { return _pack; }
const AssetPack& Asset::pack() const { return _pack; }
AssetFile& Asset::file() { return _file; }
const AssetFile& Asset::file() const { return _file; }
Asset* Asset::parent() { return _parent; }
const Asset* Asset::parent() const { return _parent; }
AssetType Asset::type() const { return _type; }
const std::string& Asset::tag() const { return _tag; }
Asset* Asset::lower_precedence() { return _lower_precedence; }
Asset* Asset::higher_precedence() { return _higher_precedence; }

AssetReference Asset::absolute_reference() const {
	if(parent()) {
		AssetReference reference = parent()->absolute_reference();
		reference.fragments.push_back(AssetReferenceFragment{type(), tag()});
		return reference;
	} else {
		return AssetReference{false};
	}
}

AssetReference Asset::reference_relative_to(Asset& asset) const {
	AssetReferenceFragment fragment{type(), tag()};
	if(parent() == &asset || parent() == nullptr) {
		return AssetReference{parent() == &asset, {std::move(fragment)}};
	} else {
		AssetReference reference = parent()->reference_relative_to(asset);
		reference.fragments.emplace_back(std::move(fragment));
		return reference;
	}
}

Asset& Asset::child(AssetType type, const std::string& tag) {
	for(std::unique_ptr<Asset>& child : _children) {
		if(child->type() == type && child->tag() == tag) {
			return *child.get();
		}
	}
	return add_child(create_asset(type, forest(), pack(), file(), this, std::move(tag)));
}

Asset* Asset::find_child(AssetType type, const std::string& tag) {
	for(std::unique_ptr<Asset>& child : _children) {
		if(child->type() == type && child->tag() == tag) {
			return child.get();
		}
	}
	return nullptr;
}

bool Asset::remove_child(Asset& asset) {
	for(auto child = _children.begin(); child != _children.end(); child++) {
		if(child->get() == &asset) {
			_children.erase(child);
			return true;
		}
	}
	return false;
}

void Asset::read(WtfNode* node) {
	read_attributes(node);
	for(WtfNode* child = node->first_child; child != nullptr; child = child->next_sibling) {
		Asset& asset = add_child(create_asset(asset_string_to_type(child->type_name), forest(), pack(), file(), this, child->tag));
		asset.read(child);
	}
}

void Asset::write(WtfWriter* ctx) const {
	write_attributes(ctx);
	for_each_child([&](Asset& child) {
		wtf_begin_node(ctx, asset_type_to_string(child.type()), child.tag().c_str());
		child.write(ctx);
		wtf_end_node(ctx);
	});
}

void Asset::validate() const {
	validate_attributes();
	for_each_child([&](Asset& child) {
		child.validate();
	});
}

Asset* Asset::lookup_asset(AssetReference reference) {
	if(reference.is_relative) {
		AssetReference absolute = absolute_reference();
		auto begin = std::make_move_iterator(reference.fragments.begin());
		auto end = std::make_move_iterator(reference.fragments.end());
		absolute.fragments.insert(absolute.fragments.end(), begin, end);
		return _forest.lookup_asset(std::move(absolute));
	} else {
		return _forest.lookup_asset(std::move(reference));
	}
}

Asset& Asset::add_child(std::unique_ptr<Asset> child) {
	return *_children.emplace_back(std::move(child)).get();
}

Asset* Asset::lookup_local_asset(const AssetReferenceFragment* begin, const AssetReferenceFragment* end) {
	for(const std::unique_ptr<Asset>& child : _children) {
		if(child->type() == begin->type && child->tag() == begin->tag) {
			if(begin + 1 < end) {
				return child->lookup_local_asset(begin + 1, end);
			} else {
				return child.get();
			}
		}
	}
	return nullptr;
}

void Asset::connect_precedence_pointers() {
	// Connect asset nodes from adjacent trees so that if a given node doesn't
	// contain a given attribute we can check lower precedence nodes.
	if(parent()) {
		// Check for a lower precedence node first. This should be the more
		// common case while editing.
		for(Asset* lower_parent = parent()->lower_precedence(); lower_parent != nullptr; lower_parent = lower_parent->lower_precedence()) {
			if(Asset* lower = lower_parent->find_child(type(), tag())) {
				Asset* higher = lower->higher_precedence();
				_lower_precedence = lower;
				_higher_precedence = higher;
				lower->_higher_precedence = this;
				if(higher) {
					higher->_lower_precedence = this;
				}
				return;
			}
		}
		// There was no lower precedence node, so now check if there's is a
		// higher precedence node.
		for(Asset* higher_parent = parent()->higher_precedence(); higher_parent != nullptr; higher_parent = higher_parent->higher_precedence()) {
			if(Asset* higher = higher_parent->find_child(type(), tag())) {
				Asset* lower = higher->lower_precedence();
				_lower_precedence = lower;
				_higher_precedence = higher;
				if(lower) {
					lower->_higher_precedence = this;
				}
				higher->_lower_precedence = this;
				return;
			}
		}
	} else {
		if(file().lower_precedence()) {
			_lower_precedence = &file().lower_precedence()->root();
		}
		if(file().higher_precedence()) {
			_higher_precedence = &file().higher_precedence()->root();
		}
	}
}

void Asset::disconnect_precedence_pointers() {
	if(lower_precedence()) {
		_lower_precedence->_higher_precedence = higher_precedence();
	}
	if(higher_precedence()) {
		_higher_precedence->_lower_precedence = lower_precedence();
	}
}

// *****************************************************************************

AssetFile::AssetFile(AssetForest& forest, AssetPack& pack, const fs::path& relative_path)
	: _pack(pack)
	, _relative_directory(relative_path.parent_path())
	, _file_name(relative_path.filename())
	, _root(std::make_unique<RootAsset>(forest, pack, *this, nullptr, "")) {}

Asset& AssetFile::root() {
	return *_root.get();
}

void AssetFile::write() const {
	FILE* fh = _pack.open_asset_write_handle(_relative_directory/_file_name);
	WtfWriter* ctx = wtf_begin_file(fh);
	defer([&]() {
		wtf_end_file(ctx);
		fclose(fh);
	});
	_root->write(ctx);
}

FileReference AssetFile::write_text_file(const fs::path& path, const char* contents) const {
	_pack.write_text_file(_relative_directory/path, contents);
	return FileReference(*this, path);
}

FileReference AssetFile::write_binary_file(const fs::path& path, Buffer contents) const {
	_pack.write_binary_file(_relative_directory/path, contents);
	return FileReference(*this, path);
}

AssetFile* AssetFile::lower_precedence() {
	for(size_t i = 0; i < _pack._asset_files.size(); i++) {
		if(_pack._asset_files[i].get() == this) {
			if(i - 1 >= 0) {
				return _pack._asset_files[i - 1].get();
			} else {
				break;
			}
		}
	}
	for(AssetPack* lower = _pack.lower_precedence(); lower != nullptr; lower = lower->lower_precedence()) {
		if(lower->_asset_files.size() >= 1) {
			return lower->_asset_files.back().get();
		}
	}
	return nullptr;
}

AssetFile* AssetFile::higher_precedence() {
	for(size_t i = 0; i < _pack._asset_files.size(); i++) {
		if(_pack._asset_files[i].get() == this) {
			if(i + 1 < _pack._asset_files.size()) {
				return _pack._asset_files[i + 1].get();
			} else {
				break;
			}
		}
	}
	for(AssetPack* higher = _pack.higher_precedence(); higher != nullptr; higher = higher->higher_precedence()) {
		if(higher->_asset_files.size() >= 1) {
			return higher->_asset_files.front().get();
		}
	}
	return nullptr;
}

void AssetFile::read() {
	fs::path path = _relative_directory/_file_name;
	std::string path_str = path.string();
	std::string text = _pack.read_text_file(path);
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
	root().read(root_node);
}

// *****************************************************************************

AssetPack::AssetPack(AssetForest& forest, bool is_writeable)
	: _forest(forest)
	, _is_writeable(is_writeable) {}

std::string AssetPack::read_text_file(const FileReference& reference) const {
	return read_text_file(reference.owner->_relative_directory/reference.path);
}

std::vector<u8> AssetPack::read_binary_file(const FileReference& reference) const {
	return read_binary_file(reference.owner->_relative_directory/reference.path);
}

bool AssetPack::is_writeable() const {
	return _is_writeable;
}

AssetFile& AssetPack::asset_file(fs::path relative_path) {
	relative_path.replace_extension("asset");
	fs::path relative_directory = relative_path.parent_path();
	fs::path file_name = relative_path.filename();
	for(std::unique_ptr<AssetFile>& file : _asset_files) {
		if(file->_relative_directory == relative_directory && file->_file_name == file_name) {
			return *file.get();
		}
	}
	return *_asset_files.emplace_back(std::make_unique<AssetFile>(_forest, *this, relative_path)).get();
}

void AssetPack::write() const {
	for(const std::unique_ptr<AssetFile>& file : _asset_files) {
		file->write();
	}
}

AssetPack* AssetPack::lower_precedence() { return _lower_precedence; }
AssetPack* AssetPack::higher_precedence() { return _higher_precedence; }

void AssetPack::read_asset_files() {
	std::vector<fs::path> asset_file_paths = enumerate_asset_files();
	std::sort(BEGIN_END(asset_file_paths));
	for(const fs::path& relative_path : asset_file_paths) {
		AssetFile& file = *_asset_files.emplace_back(std::make_unique<AssetFile>(_forest, *this, relative_path)).get();
		file.read();
	}
}

Asset* AssetPack::lookup_local_asset(const AssetReference& absolute_reference) {
	const AssetReferenceFragment* first_fragment = absolute_reference.fragments.data();
	const AssetReferenceFragment* fragment_past_end = first_fragment + absolute_reference.fragments.size();
	for(auto file = _asset_files.rbegin(); file != _asset_files.rend(); file++) {
		Asset* asset = (*file)->root().lookup_local_asset(first_fragment, fragment_past_end);
		if(asset) {
			return asset;
		}
	}
	return nullptr;
}

// *****************************************************************************

Asset* AssetForest::lookup_asset(const AssetReference& absolute_reference) {
	for(auto pack = _packs.rbegin(); pack != _packs.rend(); pack++) {
		Asset* asset = (*pack)->lookup_local_asset(absolute_reference);
		if(asset) {
			return asset;
		}
	}
	return nullptr;
}

// *****************************************************************************

LooseAssetPack::LooseAssetPack(AssetForest& forest, fs::path directory)
	: AssetPack(forest, true)
	, _directory(directory) {}

std::string LooseAssetPack::read_text_file(const fs::path& path) const {
	auto bytes = read_file(_directory/path, "r");
	return std::string((char*) bytes.data(), bytes.size());
}

std::vector<u8> LooseAssetPack::read_binary_file(const fs::path& path) const {
	return read_file(_directory/path, "rb");
}

void LooseAssetPack::write_text_file(const fs::path& path, const char* contents) const {
	fs::create_directories((_directory/path).parent_path());
	write_file(_directory/path, Buffer((u8*) contents, (u8*) contents + strlen(contents)), "w");
}

void LooseAssetPack::write_binary_file(const fs::path& path, Buffer contents) const {
	fs::create_directories((_directory/path).parent_path());
	write_file(_directory/path, contents, "wb");
}

std::vector<fs::path> LooseAssetPack::enumerate_asset_files() const {
	std::vector<fs::path> asset_files;
	for(auto& entry : fs::recursive_directory_iterator(_directory)) {
		if(entry.path().extension() == ".asset") {
			asset_files.emplace_back(fs::relative(entry.path(), _directory));
		}
	}
	return asset_files;
}

FILE* LooseAssetPack::open_asset_write_handle(const fs::path& path) const {
	fs::create_directories((_directory/path).parent_path());
	std::string string = (_directory/path).string();
	return fopen(string.c_str(), "w");
}
