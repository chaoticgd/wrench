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

#include "level_chunks.h"

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/tfrags_asset.h>
#include <wrenchbuild/level/collision_asset.h>

packed_struct(ChunkHeader,
	/* 0x0 */ s32 tfrags;
	/* 0x4 */ s32 collision;
)

void unpack_level_chunks(
	CollectionAsset& dest, InputStream& file, const ChunkWadHeader& ranges, BuildConfig config)
{
	for (s32 i = 0; i < ARRAY_SIZE(ranges.chunks); i++) {
		ChunkHeader chunk_header = {};
		if (!ranges.chunks[i].empty()) {
			chunk_header = file.read<ChunkHeader>(ranges.chunks[i].offset.bytes());
		}
		if (chunk_header.tfrags > 0 || chunk_header.collision > 0 || !ranges.sound_banks[i].empty()) {
			ChunkAsset& chunk = dest.foreign_child<ChunkAsset>(stringf("chunks/%d/chunk_%d.asset", i, i), false, i);
			if (chunk_header.tfrags > 0 && !chunk.has_tfrags()) {
				s64 offset = ranges.chunks[i].offset.bytes() + chunk_header.tfrags;
				s64 size = ranges.chunks[i].size.bytes() - chunk_header.tfrags;
				ByteRange tfrags_range{(s32) offset, (s32) size};
				unpack_compressed_asset(chunk.tfrags(SWITCH_FILES), file, tfrags_range, config);
			}
			if (chunk_header.collision > 0 && !chunk.has_collision()) {
				s64 offset = ranges.chunks[i].offset.bytes() + chunk_header.collision;
				s64 size = ranges.chunks[i].size.bytes() - chunk_header.collision;
				ByteRange collision_range{(s32) offset, (s32) size};
				unpack_compressed_asset(chunk.collision<CollisionAsset>(SWITCH_FILES), file, collision_range, config);
			}
			unpack_asset(chunk.sound_bank(), file, ranges.sound_banks[i], config);
		}
	}
}

std::vector<LevelChunk> load_level_chunks(
	const LevelWadAsset& level_wad, const Gameplay& gameplay, BuildConfig config)
{
	const CollectionAsset& collection = level_wad.get_chunks();
	std::vector<LevelChunk> chunks(3);
	u16 next_occlusion_index = 0;
	for (s32 i = 0; i < 3; i++) {
		if (collection.has_child(i)) {
			const ChunkAsset& asset = collection.get_child(i).as<ChunkAsset>();
			if (asset.has_tfrags()) {
				MemoryOutputStream stream(chunks[i].tfrags);
				const TfragsAsset& tfrags_asset = asset.get_tfrags();
				pack_tfrags(stream, &chunks[i].tfrag_meshes, tfrags_asset, &next_occlusion_index, config);
			}
			if (asset.has_collision()) {
				MemoryOutputStream stream(chunks[i].collision);
				const Asset& collision_asset = asset.get_collision();
				if (const CollisionAsset* collision = collision_asset.maybe_as<CollisionAsset>()) {
					pack_level_collision(stream, *collision, &level_wad, &gameplay, i);
				} else {
					pack_asset<ByteRange>(stream, collision_asset, config, 0x10);
				}
			}
			if (asset.has_sound_bank()) {
				MemoryOutputStream stream(chunks[i].sound_bank);
				pack_asset<ByteRange>(stream, asset.get_sound_bank(), config, 0x10);
			}
		}
	}
	return chunks;
}

ChunkWadHeader write_level_chunks(OutputStream& dest, const std::vector<LevelChunk>& chunks)
{
	ChunkWadHeader header = {};
	for (s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if (i < chunks.size()) {
			const LevelChunk& chunk = chunks[i];
			if (!chunk.tfrags.empty() || !chunk.collision.empty()) {
				dest.pad(SECTOR_SIZE, 0);
				s64 chunk_header_ofs = dest.alloc<ChunkHeader>();
				ChunkHeader chunk_header;
				if (!chunk.tfrags.empty()) {
					dest.pad(0x10, 0);
					chunk_header.tfrags = (s32) dest.tell() - chunk_header_ofs;
					std::vector<u8> compressed_tfrags;
					compress_wad(compressed_tfrags, chunks[i].tfrags, "chnktfrag", 8);
					dest.write_v(compressed_tfrags);
				}
				if (!chunk.collision.empty()) {
					dest.pad(0x10, 0);
					chunk_header.collision = (s32) dest.tell() - chunk_header_ofs;
					std::vector<u8> compressed_collision;
					compress_wad(compressed_collision, chunks[i].collision, "chunkcoll", 8);
					dest.write_v(compressed_collision);
				}
				dest.write(chunk_header_ofs, chunk_header);
				header.chunks[i] = SectorRange::from_bytes(chunk_header_ofs, dest.tell() - chunk_header_ofs);
			}
		}
	}
	for (s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if (i < chunks.size()) {
			const LevelChunk& chunk = chunks[i];
			if (!chunk.sound_bank.empty()) {
				dest.pad(SECTOR_SIZE, 0);
				s32 ofs = (s32) dest.tell();
				dest.write_v(chunk.sound_bank);
				s32 end_ofs = (s32) dest.tell();
				header.sound_banks[i] = {ofs / SECTOR_SIZE, Sector32::size_from_bytes(end_ofs - ofs)};
			}
		}
	}
	return header;
}
