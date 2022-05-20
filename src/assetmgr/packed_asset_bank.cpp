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

#include "packed_asset_bank.h"

PackedAssetBank::PackedAssetBank(AssetForest& forest, fs::path path_to_zip)
	: AssetBank(forest, false)
	, _path_to_zip(path_to_zip) {}

std::string PackedAssetBank::read_text_file(const fs::path& path) const {
	assert(0);
}

void PackedAssetBank::write_text_file(const fs::path& path, const char* contents) {
	assert(0);
}

std::vector<fs::path> PackedAssetBank::enumerate_asset_files() const {
	assert(0);
}
