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

AssetReference parse_asset_reference(const char* ptr) {
	std::string string(ptr);
	std::vector<AssetReferenceFragment> fragments;
	
	size_t begin = string[0] == '/' ? 1 : 0;
	size_t type_name_begin = begin;
	size_t tag_begin = begin;
	
	for(size_t i = begin; i < string.size(); i++) {
		if(string[i] == ':') {
			tag_begin = i + 1;
		}
		if(string[i] == '/') {
			std::string type_name = string.substr(type_name_begin, tag_begin - 1);
			AssetType type = asset_type_name_to_type(type_name.c_str());
			std::string tag = string.substr(tag_begin, i);
			fragments.push_back(AssetReferenceFragment{type, std::move(tag)});
			type_name_begin = i + 1;
		}
	}
	
	std::string type_name = string.substr(type_name_begin, tag_begin - 1);
	AssetType type = asset_type_name_to_type(type_name.c_str());
	std::string tag = string.substr(tag_begin, string.size());
	fragments.push_back(AssetReferenceFragment{type, std::move(tag)});
	
	return {string[0] != '/', fragments};
}

std::string asset_reference_to_string(const AssetReference& reference) {
	std::string string;
	bool first = true;
	for(const AssetReferenceFragment& fragment : reference.fragments) {
		if(!first || !reference.is_relative) {
			string += '/';
		}
		string += fragment.type.string;
		string += ':';
		string += fragment.tag;
		first = false;
	}
	return string;
}

// *****************************************************************************

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

static std::vector<AssetReferenceFragment> append_fragments(std::vector<AssetReferenceFragment> fragments, AssetReferenceFragment fragment) {
	fragments.emplace_back(std::move(fragment));
	return fragments;
}

AssetReference Asset::absolute_reference() const {
	if(parent()) {
		AssetReference reference = parent()->absolute_reference();
		reference.fragments.push_back(AssetReferenceFragment{type(), tag()});
		return reference;
	} else {
		return AssetReference();
	}
}

Asset& Asset::add_child(std::unique_ptr<Asset> child) {
	return *_children.emplace_back(std::move(child)).get();
}

void Asset::read(WtfNode* node) {
	read_attributes(node);
	for(WtfNode* child = node->first_child; child != nullptr; child = child->next_sibling) {
		Asset& asset = add_child(create_asset(node->type_name, forest(), pack(), file(), this));
		asset.read(child);
	}
}

void Asset::write(WtfWriter* ctx) const {
	write_attributes(ctx);
	for_each_child([&](Asset& child) {
		child.write(ctx);
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

Asset::Asset(AssetForest& forest, AssetPack& pack, AssetFile& file, Asset* parent, AssetType type)
	: _forest(forest), _pack(pack), _file(file), _parent(parent), _type(type) {}

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

// *****************************************************************************

AssetFile::AssetFile(AssetForest& forest, AssetPack& pack, const fs::path& relative_path)
	: _pack(pack)
	, _relative_directory(relative_path.parent_path())
	, _file_name(relative_path.filename())
	, _root(std::make_unique<RootAsset>(forest, pack, *this, nullptr)) {}

Asset& AssetFile::root() {
	return *_root.get();
}

void AssetFile::write() {
	FILE* fh = _pack.open_asset_write_handle(_relative_directory/_file_name);
	WtfWriter* ctx = wtf_begin_file(fh);
	defer([&]() {
		wtf_end_file(ctx);
		fclose(fh);
	});
	_root->write(ctx);
}

std::string AssetFile::read_relative_text_file(const FileReference& file) const {
	return _pack.read_text_file(_relative_directory/file.path);
}

std::vector<u8> AssetFile::read_relative_binary_file(const FileReference& file) const {
	return _pack.read_binary_file(_relative_directory/file.path);
}

// *****************************************************************************

AssetPack::AssetPack(AssetForest& forest, bool is_writeable)
	: _forest(forest)
	, _is_writeable(is_writeable) {}

bool AssetPack::is_writeable() const {
	return _is_writeable;
}

AssetFile& AssetPack::create_asset_file(const fs::path& relative_path) {
	return *_asset_files.emplace_back(std::make_unique<AssetFile>(_forest, *this, relative_path)).get();
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
	return std::string(BEGIN_END(bytes));
}

std::vector<u8> LooseAssetPack::read_binary_file(const fs::path& path) const {
	return read_file(_directory/path, "rb");
}

FILE* LooseAssetPack::open_asset_write_handle(const fs::path& path) const {
	assert(is_writeable());
	std::string string = (_directory/path).string();
	return fopen(string.c_str(), "w");
}
