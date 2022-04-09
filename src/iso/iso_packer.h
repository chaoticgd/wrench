/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#ifndef ISO_PACKER_H
#define ISO_PACKER_H

#include <core/stream.h>
#include <editor/util.h>
#include <editor/fs_includes.h>
#include <editor/command_line.h>
#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <iso/iso_filesystem.h>
#include <iso/table_of_contents.h>

using AssetPackerFunc = std::function<
	void(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& asset, Game game, u32 hint)
>;

void pack_iso(OutputStream& dest, BuildAsset& build, Game game, const AssetPackerFunc& pack_wad_asset);

#endif
