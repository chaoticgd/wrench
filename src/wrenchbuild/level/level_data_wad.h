/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef WRENCHBUILD_LEVEL_DATA_WAD_H
#define WRENCHBUILD_LEVEL_DATA_WAD_H

#include <assetmgr/asset_types.h>
#include <instancemgr/gameplay.h>
#include <wrenchbuild/level/level_chunks.h>

void unpack_rac_level_data_wad(LevelWadAsset& dest, InputStream& src, BuildConfig config);
void pack_rac_level_data_wad(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config);

void unpack_gc_uya_level_data_wad(LevelWadAsset& dest, InputStream& src, BuildConfig config);
void pack_gc_uya_level_data_wad(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config);

s32 unpack_dl_level_data_wad(LevelWadAsset& dest, InputStream& src, BuildConfig config);
void pack_dl_level_data_wad(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	std::vector<u8>& art_instances,
	std::vector<u8>& gameplay,
	const LevelWadAsset& src,
	BuildConfig config);

#endif
