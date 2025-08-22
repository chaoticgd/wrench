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

#ifndef WRENCHBUILD_LEVEL_CLASSES_H
#define WRENCHBUILD_LEVEL_CLASSES_H

#include <set>
#include <assetmgr/asset_types.h>
#include <wrenchbuild/level/level_textures.h>

struct LevelCoreHeader;

void unpack_moby_classes(
	CollectionAsset& data_dest,
	CollectionAsset& refs_dest,
	const LevelCoreHeader& header,
	InputStream& index,
	InputStream& data,
	const std::vector<GsRamEntry>& gs_table,
	InputStream& gs_ram,
	const std::vector<s64>& block_bounds,
	BuildConfig config,
	s32 moby_stash_addr,
	const std::set<s32>& moby_stash);
void pack_moby_classes(
	OutputStream& index,
	OutputStream& core,
	const CollectionAsset& classes,
	const std::vector<LevelTexture>& textures,
	s32 table,
	s32 texture_index,
	BuildConfig config);

void unpack_tie_classes(
	CollectionAsset& data_dest,
	CollectionAsset& refs_dest,
	const LevelCoreHeader& header,
	InputStream& index,
	InputStream& data,
	InputStream& gs_ram,
	const std::vector<s64>& block_bounds,
	BuildConfig config);
void pack_tie_classes(
	OutputStream& index,
	OutputStream& core,
	const CollectionAsset& classes,
	const std::vector<LevelTexture>& textures,
	s32 table,
	s32 texture_index,
	BuildConfig config);

void unpack_shrub_classes(
	CollectionAsset& data_dest,
	CollectionAsset& refs_dest,
	const LevelCoreHeader& header,
	InputStream& index,
	InputStream& data,
	InputStream& gs_ram,
	const std::vector<s64>& block_bounds,
	BuildConfig config);
void pack_shrub_classes(
	OutputStream& index,
	OutputStream& core,
	const CollectionAsset& classes,
	const std::vector<LevelTexture>& textures,
	s32 table,
	s32 texture_index,
	BuildConfig config);

std::array<ArrayRange, 3> allocate_class_tables(
	OutputStream& index,
	const CollectionAsset& mobies,
	const CollectionAsset& ties,
	const CollectionAsset& shrubs);


#endif
