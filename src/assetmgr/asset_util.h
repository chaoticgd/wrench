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
	
	bool operator==(const AssetReferenceFragment& rhs) const { return tag == rhs.tag; }
};

struct AssetReference {
	std::vector<AssetReferenceFragment> fragments;
	std::string pack;
	
	bool operator==(const AssetReference& rhs) const { return fragments == rhs.fragments && pack == rhs.pack; }
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

enum AssetAccessorMode {
	DO_NOT_SWITCH_FILES,
	SWITCH_FILES
};

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

// *****************************************************************************

// Takes a string in the form of "firsttoken,secondtoken,etc", copies firsttoken
// into a temporary static buffer, and sets the hint pointer to point to
// secondtoken. This is used to consume hint strings passed to asset packers
// and unpackers.
const char* next_hint(const char** hint);

#endif
