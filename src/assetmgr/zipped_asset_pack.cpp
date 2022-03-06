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

#include "zipped_asset_pack.h"

ZippedAssetPack::ZippedAssetPack(AssetForest& forest, fs::path path_to_zip)
	: AssetPack(forest, false)
	, _path_to_zip(path_to_zip) {}
	
std::string ZippedAssetPack::read_text_file(const fs::path& path) const {
	assert(0);
}

std::vector<u8> ZippedAssetPack::read_binary_file(const fs::path& path) const {
	assert(0);
}

void ZippedAssetPack::write_text_file(const fs::path& path, const char* contents) const {
	assert(0);
}

void ZippedAssetPack::write_binary_file(const fs::path& path, Buffer contents) const {
	assert(0);
}

std::vector<fs::path> ZippedAssetPack::enumerate_asset_files() const {
	assert(0);
}

FILE* ZippedAssetPack::open_asset_write_handle(const fs::path& write_path) const {
	std::string path_to_zip = _path_to_zip.string();
	std::string write_path_str = write_path.string();
	fprintf(stderr, "Tried to write to zipped asset pack! This should never happen!\n");
	fprintf(stderr, "\tpath_to_zip=%s, write_path=%s\n", path_to_zip.c_str(), write_path_str.c_str());
	abort();
}
