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

#ifndef ASSETMGR_ZIPPED_ASSET_PACK_H
#define ASSETMGR_ZIPPED_ASSET_PACK_H

#include "asset.h"

class ZippedAssetPack : public AssetPack {
private:
	ZippedAssetPack(AssetForest& forest, fs::path path_to_zip);
	
	std::string read_text_file(const fs::path& path) const override;
	std::vector<u8> read_binary_file(const fs::path& path) const override;
	FILE* open_asset_write_handle(const fs::path& write_path) const override;
	
	fs::path _path_to_zip;
};

#endif
