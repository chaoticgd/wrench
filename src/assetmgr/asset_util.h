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

#ifndef ASSETMGR_ASSET_UTIL_H
#define ASSETMGR_ASSET_UTIL_H

#include <any>

#include <core/wtf.h>
#include <core/util.h>
#include <core/stream.h>
#include <core/filesystem.h>

struct AssetType {
	s32 id = -1;
	
	bool operator==(const AssetType& rhs) const { return id == rhs.id; }
	bool operator!=(const AssetType& rhs) const { return id != rhs.id; }
};

#define NULL_ASSET_TYPE AssetType()

using AssetVisitorCallback = std::function<void(const char* key, std::any value, std::function<void(std::any)> setter)>;
using ConstAssetVisitorCallback = std::function<void(const char* key, std::any value)>;

struct AssetReferenceFragment {
	std::string tag;
	AssetType type = {-1}; // Not populated by the parser.
};

struct AssetReference {
	bool is_relative = true;
	std::vector<AssetReferenceFragment> fragments;
	std::string pack;
};

AssetReference parse_asset_reference(const char* ptr);
std::string asset_reference_to_string(const AssetReference& reference);

class AssetForest;
class AssetPack;
class AssetFile;

struct FileReference {
	FileReference() {}
	FileReference(const AssetFile& owner_, fs::path path_)
		: owner(&owner_)
		, path(path_) {}
	
	const AssetFile* owner = nullptr;
	fs::path path;
};

// *****************************************************************************

enum AssetPackType {
	WADS, // Built WAD files extracted from a base game ISO.
	BINS, // Built loose files extracted from a base game ISO.
	SOURCE, // Source assets unpacked from files extracted from a base game ISO.
	LIBRARY, // Additional assets to be used by mods.
	MOD // A mod.
};

struct GameInfo {
	AssetPackType type;
	std::vector<AssetReference> builds; // List of builds included with this asset pack.
	std::vector<std::string> dependencies; // Library packs to be mounted before the mod but after the base game.
};

GameInfo read_game_info(char* input);
void write_game_info(std::string& dest, const GameInfo& info);

// *****************************************************************************

class Asset;

using AssetPackerFunc = std::function<void((OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, u32 hint))>;

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, u32 hint) {
		func(dest, static_cast<ThisAsset&>(src), game);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_wad_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, u32 hint) {
		func(dest, header_dest, static_cast<ThisAsset&>(src), game);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_bin_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, u32 hint) {
		func(dest, header_dest, time_dest, static_cast<ThisAsset&>(src));
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_iso_packer_func(PackerFunc func, AssetPackerFunc pack) {
	return new AssetPackerFunc([func, pack](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, u32 hint) {
		func(dest, static_cast<ThisAsset&>(src), game, pack);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

// *****************************************************************************

struct AssetError : std::exception {};

struct InvalidAssetAttributeType : AssetError {
	InvalidAssetAttributeType(const WtfNode* node, const WtfAttribute* attribute) {}
};

struct MissingAssetAttribute : AssetError {
	MissingAssetAttribute() {}
};

struct MissingChildAsset : AssetError {
	MissingChildAsset() {}
};

struct AssetLookupFailed : AssetError {
	AssetLookupFailed(std::string a) : asset(a) {}
	const char* what() const noexcept override { return asset.c_str(); }
	std::string asset;
};

#endif
