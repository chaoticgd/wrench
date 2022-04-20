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

#include "level_wad.h"

#include <spanner/asset_packer.h>

static void pack_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game);

on_load([]() {
	LevelWadAsset::pack_func = wrap_wad_packer_func<LevelWadAsset>(pack_level_wad);
})

packed_struct(LevelWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ s32 id;
	/* 0x00c */ s32 reverb;
	/* 0x010 */ s32 max_mission_sizes[2];
	/* 0x018 */ SectorRange data;
	/* 0x020 */ SectorRange core_sound_bank;
	/* 0x028 */ SectorRange chunks[3];
	/* 0x040 */ SectorRange chunk_sound_banks[3];
	/* 0x058 */ SectorRange gameplay_core;
	/* 0x060 */ SectorRange mission_instances[128];
	/* 0x460 */ SectorRange mission_data[128];
	/* 0x860 */ SectorRange mission_sound_banks[128];
	/* 0xc60 */ SectorRange art_instances;
)

packed_struct(ChunkHeader,
	/* 0x0 */ s32 tfrags;
	/* 0x4 */ s32 collision;
)

// These offsets are relative to the beginning of the level file.
packed_struct(MissionHeader,
	/* 0x0 */ ByteRange instances;
	/* 0x8 */ ByteRange classes;
)

static void pack_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	LevelWadHeaderDL header = {0};
	header.header_size = sizeof(LevelWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.id = src.id();
	header.reverb = src.reverb();
	
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), game, base);
	header.core_sound_bank = pack_asset_sa<SectorRange>(dest, src.get_core_sound_bank(), game, base);
	CollectionAsset& chunks = src.get_chunks();
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if(chunks.has_child(i)) {
			ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
			if(chunk.has_tfrags() && chunk.has_collision()) {
				ChunkHeader chunk_header;
				dest.pad(SECTOR_SIZE, 0);
				s64 chunk_header_ofs = dest.tell();
				dest.write(chunk_header);
				chunk_header.tfrags = pack_compressed_asset_aligned<ByteRange>(dest, chunk.get_tfrags(), game, chunk_header_ofs, 0x10).offset;
				chunk_header.collision = pack_compressed_asset_aligned<ByteRange>(dest, chunk.get_collision(), game, chunk_header_ofs, 0x10).offset;
				dest.write(chunk_header_ofs, chunk_header);
			}
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if(chunks.has_child(i)) {
			ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
			if(chunk.has_sound_bank()) {
				header.chunk_sound_banks[i] = pack_asset_sa<SectorRange>(dest, chunk.get_sound_bank(), game, base);
			}
		}
	}
	header.gameplay_core = pack_asset_sa<SectorRange>(dest, src.get_gameplay_core(), game, base);
	CollectionAsset& missions = src.get_missions();
	for(s32 i = 0; i < ARRAY_SIZE(header.mission_instances); i++) {
		if(missions.has_child(i)) {
			MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if(mission.has_instances()) {
				header.mission_instances[i] = pack_asset_sa<SectorRange>(dest, mission.get_instances(), game, base);
			}
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.mission_data); i++) {
		if(missions.has_child(i)) {
			MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			MissionHeader mission_header = {0};
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			dest.write(mission_header);
			if(mission.has_instances()) {
				mission_header.instances = pack_compressed_asset_aligned<ByteRange>(dest, mission.get_instances(), game, base, 0x40);
			}
			if(mission.has_classes()) {
				mission_header.classes = pack_compressed_asset_aligned<ByteRange>(dest, mission.get_classes(), game, base, 0x40);
			}
			dest.write(mission_header_ofs, mission_header);
		}
	}
	header.art_instances = pack_asset_sa<SectorRange>(dest, src.get_art_instances(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_level_wad(LevelWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<LevelWadHeaderDL>(src);
	
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	
	unpack_binary(dest.data(), *file, header.data, "data.bin");
	unpack_binary(dest.core_sound_bank(), *file, header.core_sound_bank, "core_sound_bank.bin");
	CollectionAsset& chunks = dest.chunks();
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		ChunkHeader chunk_header = file->read<ChunkHeader>(header.chunks[i].offset.bytes());
		if(chunk_header.tfrags > 0 || chunk_header.collision > 0 || !header.chunk_sound_banks[i].empty()) {
			ChunkAsset& chunk = chunks.switch_files(stringf("chunks/%d/chunk%d.asset", i, i)).child<ChunkAsset>(i);
			if(chunk_header.tfrags > 0) {
				ByteRange tfrags_range{chunk_header.tfrags, (s32) (header.chunks[i].size.bytes() - chunk_header.tfrags)};
				unpack_compressed_binary(chunk.tfrags(), *file, tfrags_range, "tfrags.bin");
			}
			if(chunk_header.collision > 0) {
				ByteRange collision_range{chunk_header.collision, (s32) (header.chunks[i].size.bytes() - chunk_header.collision)};
				unpack_compressed_binary(chunk.collision(), *file, collision_range, "collision.bin");
			}
			unpack_binary(chunk.sound_bank(), *file, header.chunk_sound_banks[i], "sound_bank.bin");
		}
	}
	unpack_binary(dest.gameplay_core(), *file, header.gameplay_core, "gameplay_core.bin");
	CollectionAsset& missions = dest.missions();
	for(s32 i = 0; i < ARRAY_SIZE(header.mission_data); i++) {
		MissionHeader mission_header = file->read<MissionHeader>(header.mission_data[i].offset.bytes());
		if(!mission_header.instances.empty() || !mission_header.classes.empty()) {
			MissionAsset& mission = missions.switch_files(stringf("missions/%d/mission%d.asset", i, i)).child<MissionAsset>(i);
			unpack_compressed_binary(mission.instances(), *file, mission_header.instances, "instances.bin");
			unpack_compressed_binary(mission.classes(), *file, mission_header.classes, "classes.bin");
		}
	}
	unpack_binary(dest.art_instances(), *file, header.art_instances, "art_instances.bin");
}
