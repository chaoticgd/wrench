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
#include <functional>
#include <glm/glm.hpp>

#include <wtf/wtf.h>
#include <core/gltf.h>
#include <core/util.h>
#include <core/stream.h>
#include <core/collada.h>
#include <core/filesystem.h>

struct AssetType
{
	s16 id = -1;
	
	bool operator==(const AssetType& rhs) const { return id == rhs.id; }
	bool operator!=(const AssetType& rhs) const { return id != rhs.id; }
};

#define NULL_ASSET_TYPE AssetType()

using AssetVisitorCallback = std::function<void(const char* key, std::any value, std::function<void(std::any)> setter)>;
using ConstAssetVisitorCallback = std::function<void(const char* key, std::any value)>;

struct AssetLinkPointers
{
	const char* prefix = nullptr;
	std::vector<const char*> tags;
};

// Stores a link to an asset e.g. "gc.levels.0" as a single string in memory
// with the seperators replaced with nulls so pointers to each section can be
// used as strings directly.
class AssetLink
{
public:
	AssetLink();
	AssetLink(const char* src);
	
	AssetLinkPointers get() const;
	void set(const char* src);
	void add_prefix(const char* str);
	void add_tag(const char* tag);
	std::string to_string() const;
private:
	bool m_prefix = false;
	s16 m_tags = 0;
	std::vector<char> m_data; // = [prefix \0] fragment(0) \0 ... \0 fragment(fragments-1)
};

class AssetForest;
class AssetBank;
class AssetFile;

struct FileReference
{
	FileReference() {}
	FileReference(const AssetFile& owner_, fs::path path_)
		: owner(&owner_)
		, path(path_) {}
	
	std::unique_ptr<InputStream> open_binary_file_for_reading(fs::file_time_type* modified_time_dest = nullptr) const;
	std::string read_text_file() const;
	
	const AssetFile* owner = nullptr;
	fs::path path;
};

std::vector<ColladaScene*> read_collada_files(std::vector<std::unique_ptr<ColladaScene>>& owners, std::vector<FileReference> refs);
std::vector<GLTF::ModelFile*> read_glb_files(std::vector<std::unique_ptr<GLTF::ModelFile>>& owners, std::vector<FileReference> refs);

enum AssetAccessorMode
{
	DO_NOT_SWITCH_FILES,
	SWITCH_FILES
};

// Takes a string in the form of "firsttoken,secondtoken,etc", copies firsttoken
// into a temporary static buffer, and sets the hint pointer to point to
// secondtoken. This is used to consume hint strings passed to asset packers
// and unpackers.
const char* next_hint(const char** hint);

#endif
