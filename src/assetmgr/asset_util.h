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

#include <core/util.h>
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
	AssetType type;
	std::string tag;
};

struct AssetReference {
	bool is_relative = true;
	std::vector<AssetReferenceFragment> fragments;
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

#endif
