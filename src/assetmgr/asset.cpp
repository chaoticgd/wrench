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

AssetPack& Asset::pack() { return _pack; }
const AssetPack& Asset::pack() const { return _pack; }
Asset* Asset::parent() { return _parent; }
const Asset* Asset::parent() const { return _parent; }
AssetType Asset::type() const { return _type; }
const std::string& Asset::tag() const { return _tag; }

const fs::path Asset::file_path() const {
	return directory_path()/_file_name;
}

const fs::path& Asset::directory_path() const {
	if(_dir.empty()) {
		assert(parent());
		return parent()->directory_path();
	}
	return _dir;
}

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

void Asset::read(WtfNode* node) {
	read_attributes(node);
	
}

static void begin_enclosing_nodes(WtfWriter* ctx, const Asset& child) {
	const Asset* parent = child.parent();
	if(parent) {
		begin_enclosing_nodes(ctx, *parent);
		const char* tag = parent->tag().c_str();
		wtf_begin_node(ctx, parent->type().string, tag);
	}
}

static void end_enclosing_nodes(WtfWriter* ctx, const Asset& child) {
	const Asset* parent = child.parent();
	if(parent) {
		wtf_end_node(ctx);
		end_enclosing_nodes(ctx, *parent);
	}
}

void Asset::write(WtfWriter* ctx) const {
	if(!_file_name.empty() || ctx == nullptr) {
		// This asset is the root of a new file.
		assert(!(!_file_name.empty() && ctx == nullptr));
		FILE* file = pack().open_asset_write_handle(file_path());
		WtfWriter* child_ctx = wtf_begin_file(file);
		defer([&]() {
			wtf_end_file(child_ctx);
			fclose(file);
		});
		begin_enclosing_nodes(child_ctx, *this);
		write_attributes(child_ctx);
		for_each_child([&](Asset& child) {
			child.write(child_ctx);
		});
		end_enclosing_nodes(child_ctx, *this);
	} else {
		// This asset is being written to the already open file.
		write_attributes(ctx);
		for_each_child([&](Asset& child) {
			child.write(ctx);
		});
	}
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
		return _manager.lookup_asset(std::move(absolute));
	} else {
		return _manager.lookup_asset(std::move(reference));
	}
}
	
std::string Asset::read_text_file(const FileReference& file) const {
	return _pack.read_text_file(_dir/file.path);
}

std::vector<u8> Asset::read_binary_file(const FileReference& file) const {
	return _pack.read_binary_file(_dir/file.path);
}

Asset* Asset::lookup_local_asset(const AssetReferenceFragment* begin, const AssetReferenceFragment* end) {
	if(begin < end) {
		for(const std::unique_ptr<Asset>& child : _children) {
			if(child->type() == begin->type && child->tag() == begin->tag) {
				return child->lookup_local_asset(begin + 1, end);
			}
		}
	}
	return nullptr;
}

// *****************************************************************************

AssetPack::AssetPack(AssetManager& manager, bool is_writeable)
	: _root(std::make_unique<RootAsset>(manager, *this, nullptr))
	, _is_writeable(is_writeable) {}

Asset* AssetPack::root() {
	return _root.get();
}

bool AssetPack::is_writeable() const {
	return _is_writeable;
}

// *****************************************************************************

LooseAssetPack::LooseAssetPack(AssetManager& manager, fs::path directory)
	: AssetPack(manager, true)
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

// *****************************************************************************

Asset* AssetManager::lookup_asset(AssetReference absolute_reference) {
	for(auto pack = _packs.rbegin(); pack != _packs.rend(); pack++) {
		AssetReferenceFragment* first_fragment = absolute_reference.fragments.data();
		AssetReferenceFragment* fragment_past_end = first_fragment + absolute_reference.fragments.size();
		Asset* asset = (*pack)->root()->lookup_local_asset(first_fragment, fragment_past_end);
		if(asset) {
			return asset;
		}
	}
	return nullptr;
}
