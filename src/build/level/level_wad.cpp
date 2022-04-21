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

#include <build/asset_packer.h>

static void pack_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game);

on_load([]() {
	LevelWadAsset::pack_func = wrap_wad_packer_func<LevelWadAsset>(pack_level_wad);
})

packed_struct(LevelWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ s32 id;
	/* 0x00c */ s32 reverb;
	/* 0x010 */ s32 max_mission_instances_size;
	/* 0x014 */ s32 max_mission_classes_size;
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
	
	header.core_sound_bank = pack_asset_sa<SectorRange>(dest, src.get_core_sound_bank(), game, base);
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), game, base);
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
				header.chunks[i] = SectorRange::from_bytes(chunk_header_ofs - base, dest.tell() - chunk_header_ofs);
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
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_instances(), game, base);
				
				header.max_mission_instances_size = std::max(header.max_mission_instances_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, 8);
				
				s64 begin = dest.tell();
				dest.write(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.instances = ByteRange::from_bytes(begin - base, end - begin);
			}
			if(mission.has_classes()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_classes(), game, base);
				
				header.max_mission_classes_size = std::max(header.max_mission_classes_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, 8);
				
				s64 begin = dest.tell();
				dest.write(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.classes = ByteRange::from_bytes(begin - base, end - begin);
			}
			dest.write(mission_header_ofs, mission_header);
			header.mission_data[i] = SectorRange::from_bytes(mission_header_ofs - base, dest.tell() - mission_header_ofs);
		} else {
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			MissionHeader mission_header = {0};
			mission_header.instances.offset = -1;
			mission_header.classes.offset = -1;
			dest.write(mission_header);
			header.mission_data[i] = SectorRange::from_bytes(mission_header_ofs - base, dest.tell() - mission_header_ofs);
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.mission_sound_banks); i++) {
		if(missions.has_child(i)) {
			MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if(mission.has_sound_bank()) {
				header.mission_sound_banks[i] = pack_asset_sa<SectorRange>(dest, mission.get_sound_bank(), game, base);
			}
		}
	}
	header.art_instances = pack_compressed_asset_sa<SectorRange>(dest, src.get_art_instances(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_level_wad(LevelWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<LevelWadHeaderDL>(src);
	
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	
	unpack_binary(dest.core_sound_bank(), *file, header.core_sound_bank, "core_sound_bank.bin");
	unpack_binary(dest.data(), *file, header.data, "data.bin");
	CollectionAsset& chunks = dest.chunks();
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		ChunkHeader chunk_header = {0};
		if(!header.chunks[i].empty()) {
			chunk_header = file->read<ChunkHeader>(header.chunks[i].offset.bytes());
		}
		if(chunk_header.tfrags > 0 || chunk_header.collision > 0 || !header.chunk_sound_banks[i].empty()) {
			ChunkAsset& chunk = chunks.switch_files(stringf("chunks/%d/chunk%d.asset", i, i)).child<ChunkAsset>(i);
			if(chunk_header.tfrags > 0) {
				s64 offset = header.chunks[i].offset.bytes() + chunk_header.tfrags;
				s64 size = header.chunks[i].size.bytes() - chunk_header.tfrags;
				ByteRange tfrags_range{(s32) offset, (s32) size};
				unpack_compressed_binary(chunk.tfrags(), *file, tfrags_range, "tfrags.bin");
			}
			if(chunk_header.collision > 0) {
				s64 offset = header.chunks[i].offset.bytes() + chunk_header.collision;
				s64 size = header.chunks[i].size.bytes() - chunk_header.collision;
				ByteRange collision_range{(s32) offset, (s32) size};
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
			unpack_binary(mission.sound_bank(), *file, header.mission_sound_banks[i], "sound_bank.bin");
		}
	}
	unpack_compressed_binary(dest.art_instances(), *file, header.art_instances, "art_instances.bin");
}
