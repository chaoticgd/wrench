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

#ifndef WRENCHBUILD_LEVEL_CHUNKS_H
#define WRENCHBUILD_LEVEL_CHUNKS_H

#include <core/mesh.h>
#include <core/stream.h>
#include <assetmgr/asset_types.h>
#include <instancemgr/gameplay.h>

packed_struct(ChunkWadHeader,
	/* 0x00 */ SectorRange chunks[3];
	/* 0x18 */ SectorRange sound_banks[3];
)

struct LevelChunk
{
	std::vector<u8> tfrags;
	std::vector<Mesh> tfrag_meshes;
	std::vector<u8> collision;
	std::vector<u8> sound_bank;
};

void unpack_level_chunks(
	CollectionAsset& dest, InputStream& file, const ChunkWadHeader& ranges, BuildConfig config);
std::vector<LevelChunk> load_level_chunks(
	const LevelWadAsset& level_wad, const Gameplay& gameplay, BuildConfig config);
ChunkWadHeader write_level_chunks(OutputStream& dest, const std::vector<LevelChunk>& chunks);

#endif
