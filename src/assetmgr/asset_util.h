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

#include <wtf/wtf.h>
#include <core/util.h>
#include <core/stream.h>
#include <core/filesystem.h>

struct AssetType {
	s16 id = -1;
	
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
	std::vector<AssetReferenceFragment> fragments;
	std::string pack;
};

AssetReference parse_asset_reference(const char* ptr);
std::string asset_reference_to_string(const AssetReference& reference);

class AssetForest;
class AssetBank;
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

enum class AssetBankType {
	UNPACKED, // Assets unpacked from a base game ISO.
	MOD, // A mod.
	TEST // A set of binary assets to be used for running tests on.
};

struct GameInfo {
	std::string name;
	s32 format_version;
	AssetBankType type;
	std::vector<AssetReference> builds; // List of builds included with this asset pack.
};

GameInfo read_game_info(char* input);
void write_game_info(std::string& dest, const GameInfo& info);

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
