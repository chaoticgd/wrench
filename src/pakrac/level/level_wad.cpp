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

#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

packed_struct(Rac1LevelWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 unused_4;
	/* 0x008 */ s32 id;
	/* 0x00c */ s32 unused_c;
	/* 0x010 */ SectorRange data;
	/* 0x018 */ SectorRange gameplay_ntsc;
	/* 0x020 */ SectorRange gameplay_pal;
	/* 0x028 */ SectorRange occlusion;
)

packed_struct(ChunkWadHeader,
	/* 0x00 */ SectorRange chunks[3];
	/* 0x18 */ SectorRange sound_banks[3];
)

packed_struct(Rac23LevelWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s32 id;
	/* 0x0c */ s32 reverb;
	/* 0x10 */ SectorRange data;
	/* 0x18 */ SectorRange core_sound_bank;
	/* 0x20 */ SectorRange gameplay;
	/* 0x28 */ SectorRange occlusion;
	/* 0x30 */ ChunkWadHeader chunks;
)
static_assert(sizeof(Rac23LevelWadHeader) == 0x60);

packed_struct(Rac23LevelWadHeader68,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s32 id;
	/* 0x0c */ SectorRange data;
	/* 0x14 */ SectorRange core_sound_bank;
	/* 0x1c */ SectorRange gameplay_1;
	/* 0x24 */ SectorRange gameplay_2;
	/* 0x2c */ SectorRange occlusion;
	/* 0x34 */ SectorRange chunks[3];
	/* 0x4c */ s32 reverb;
	/* 0x50 */ SectorRange chunk_banks[3];
)
static_assert(sizeof(Rac23LevelWadHeader68) == 0x68);

packed_struct(MaxMissionSizes,
	/* 0x0 */ s32 max_instances_size;
	/* 0x4 */ s32 max_classes_size;
)

packed_struct(MissionWadHeader,
	/* 0x000 */ SectorRange instances[128];
	/* 0x400 */ SectorRange data[128];
	/* 0x800 */ SectorRange sound_banks[128];
)

packed_struct(DeadlockedLevelWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ s32 id;
	/* 0x00c */ s32 reverb;
	/* 0x010 */ MaxMissionSizes max_mission_sizes;
	/* 0x018 */ SectorRange data;
	/* 0x020 */ SectorRange core_sound_bank;
	/* 0x028 */ ChunkWadHeader chunks;
	/* 0x058 */ SectorRange gameplay_core;
	/* 0x060 */ MissionWadHeader missions;
	/* 0xc60 */ SectorRange art_instances;
)

static void unpack_rac1_level_wad(LevelWadAsset& dest, InputStream& src, Game game);
static void pack_rac1_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game);
static void unpack_rac23_level_wad(LevelWadAsset& dest, InputStream& src, Game game);
static void pack_rac23_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game);
static void unpack_dl_level_wad(LevelWadAsset& dest, InputStream& src, Game game);
static void pack_dl_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game);
static void unpack_chunks(CollectionAsset& dest, InputStream& file, const ChunkWadHeader& ranges, Game game);
static ChunkWadHeader pack_chunks(OutputStream& dest, CollectionAsset& chunks, Game game);
static void unpack_missions(CollectionAsset& dest, InputStream& file, const MissionWadHeader& ranges, Game game);
static std::pair<MissionWadHeader, MaxMissionSizes> pack_missions(OutputStream& dest, CollectionAsset& missions, Game game);

on_load(Level, []() {
	LevelWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<LevelWadAsset>(unpack_rac1_level_wad);
	LevelWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<LevelWadAsset>(unpack_rac23_level_wad);
	LevelWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<LevelWadAsset>(unpack_rac23_level_wad);
	LevelWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<LevelWadAsset>(unpack_dl_level_wad);
	
	LevelWadAsset::funcs.pack_rac1 = wrap_wad_packer_func<LevelWadAsset>(pack_rac1_level_wad);
	LevelWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<LevelWadAsset>(pack_rac23_level_wad);
	LevelWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<LevelWadAsset>(pack_rac23_level_wad);
	LevelWadAsset::funcs.pack_dl = wrap_wad_packer_func<LevelWadAsset>(pack_dl_level_wad);
})

void unpack_rac1_level_wad(LevelWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac1LevelWadHeader>(0);
	
	dest.set_id(header.id);
	
	unpack_asset(dest.data(), src, header.data, game);
	unpack_asset(dest.gameplay_core(), src, header.gameplay_ntsc, game);
}

static void pack_rac1_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game) {
	Rac1LevelWadHeader header = {0};
	header.header_size = sizeof(Rac1LevelWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.id = src.id();
	
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), game);
	header.gameplay_ntsc = pack_asset_sa<SectorRange>(dest, src.get_gameplay_core(), game);
	header.gameplay_pal = pack_asset_sa<SectorRange>(dest, src.get_gameplay_core(), game);
	// TODO: header.occlusion
	
	dest.write(0, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_rac23_level_wad(LevelWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac23LevelWadHeader>(0);
	
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	
	unpack_asset(dest.core_sound_bank(), src, header.core_sound_bank, game);
	unpack_asset(dest.data(), src, header.data, game);
	unpack_asset(dest.gameplay_core(), src, header.gameplay, game);
	unpack_chunks(dest.chunks(), src, header.chunks, game);
}

static void pack_rac23_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game) {
	Rac23LevelWadHeader header = {0};
	header.header_size = sizeof(Rac23LevelWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.id = src.id();
	header.reverb = src.reverb();
	
	header.core_sound_bank = pack_asset_sa<SectorRange>(dest, src.get_core_sound_bank(), game);
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), game);
	header.gameplay = pack_asset_sa<SectorRange>(dest, src.get_gameplay_core(), game);
	// TODO: header.occlusion
	header.chunks = pack_chunks(dest, src.get_chunks(), game);
	
	dest.write(0, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_dl_level_wad(LevelWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedLevelWadHeader>(0);
	
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	
	unpack_asset(dest.core_sound_bank(), src, header.core_sound_bank, game);
	unpack_asset(dest.data(), src, header.data, game);
	unpack_chunks(dest.chunks(), src, header.chunks, game);
	unpack_asset(dest.gameplay_core(), src, header.gameplay_core, game);
	unpack_missions(dest.missions(), src, header.missions, game);
	unpack_compressed_asset(dest.art_instances(), src, header.art_instances, game);
}

static void pack_dl_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& src, Game game) {
	DeadlockedLevelWadHeader header = {0};
	header.header_size = sizeof(DeadlockedLevelWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.id = src.id();
	header.reverb = src.reverb();
	
	header.core_sound_bank = pack_asset_sa<SectorRange>(dest, src.get_core_sound_bank(), game);
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), game);
	header.chunks = pack_chunks(dest, src.get_chunks(), game);
	header.gameplay_core = pack_asset_sa<SectorRange>(dest, src.get_gameplay_core(), game);
	std::tie(header.missions, header.max_mission_sizes) = pack_missions(dest, src.get_missions(), game);
	header.art_instances = pack_compressed_asset_sa<SectorRange>(dest, src.get_art_instances(), game, "art_insts");
	
	dest.write(0, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

packed_struct(ChunkHeader,
	/* 0x0 */ s32 tfrags;
	/* 0x4 */ s32 collision;
)

static void unpack_chunks(CollectionAsset& dest, InputStream& file, const ChunkWadHeader& ranges, Game game) {
	for(s32 i = 0; i < ARRAY_SIZE(ranges.chunks); i++) {
		ChunkHeader chunk_header = {0};
		if(!ranges.chunks[i].empty()) {
			chunk_header = file.read<ChunkHeader>(ranges.chunks[i].offset.bytes());
		}
		if(chunk_header.tfrags > 0 || chunk_header.collision > 0 || !ranges.sound_banks[i].empty()) {
			ChunkAsset& chunk = dest.switch_files(stringf("chunks/%d/chunk%d.asset", i, i)).child<ChunkAsset>(i);
			if(chunk_header.tfrags > 0) {
				s64 offset = ranges.chunks[i].offset.bytes() + chunk_header.tfrags;
				s64 size = ranges.chunks[i].size.bytes() - chunk_header.tfrags;
				ByteRange tfrags_range{(s32) offset, (s32) size};
				unpack_compressed_asset(chunk.tfrags(), file, tfrags_range, game);
			}
			if(chunk_header.collision > 0) {
				s64 offset = ranges.chunks[i].offset.bytes() + chunk_header.collision;
				s64 size = ranges.chunks[i].size.bytes() - chunk_header.collision;
				ByteRange collision_range{(s32) offset, (s32) size};
				unpack_compressed_asset(chunk.collision(), file, collision_range, game);
			}
			unpack_asset(chunk.sound_bank(), file, ranges.sound_banks[i], game);
		}
	}
}

static ChunkWadHeader pack_chunks(OutputStream& dest, CollectionAsset& chunks, Game game) {
	ChunkWadHeader header = {0};
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if(chunks.has_child(i)) {
			ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
			if(chunk.has_tfrags() || chunk.has_collision()) {
				dest.pad(SECTOR_SIZE, 0);
				s64 chunk_header_ofs = dest.tell();
				SubOutputStream chunk_dest(dest, chunk_header_ofs);
				ChunkHeader chunk_header = {-1, -1};
				static_cast<OutputStream&>(chunk_dest).write(chunk_header);
				if(chunk.has_tfrags()) {
					chunk_header.tfrags = pack_compressed_asset<ByteRange>(dest, chunk.get_tfrags(), game, 0x10, "chnktfrag").offset;
				}
				if(chunk.has_collision()) {
					chunk_header.collision = pack_compressed_asset<ByteRange>(dest, chunk.get_collision(), game, 0x10, "chunkcoll").offset;
				}
				dest.write(chunk_header_ofs, chunk_header);
				header.chunks[i] = SectorRange::from_bytes(chunk_header_ofs, dest.tell() - chunk_header_ofs);
			}
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if(chunks.has_child(i)) {
			ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
			if(chunk.has_sound_bank()) {
				header.sound_banks[i] = pack_asset_sa<SectorRange>(dest, chunk.get_sound_bank(), game);
			}
		}
	}
	return header;
}

// These offsets are relative to the beginning of the level file.
packed_struct(MissionHeader,
	/* 0x0 */ ByteRange instances;
	/* 0x8 */ ByteRange classes;
)

static void unpack_missions(CollectionAsset& dest, InputStream& file, const MissionWadHeader& ranges, Game game) {
	for(s32 i = 0; i < ARRAY_SIZE(ranges.data); i++) {
		MissionHeader header = {0};
		if(!ranges.data[i].empty()) {
			header = file.read<MissionHeader>(ranges.data[i].offset.bytes());
		}
		if(!header.instances.empty() || !header.classes.empty() || !ranges.sound_banks[i].empty()) {
			MissionAsset& mission = dest.switch_files(stringf("missions/%d/mission%d.asset", i, i)).child<MissionAsset>(i);
			unpack_compressed_asset(mission.instances(), file, header.instances, game);
			unpack_compressed_asset(mission.classes(), file, header.classes, game);
			unpack_asset(mission.sound_bank(), file, ranges.sound_banks[i], game);
		}
	}
}

static std::pair<MissionWadHeader, MaxMissionSizes> pack_missions(OutputStream& dest, CollectionAsset& missions, Game game) {
	MissionWadHeader header;
	MaxMissionSizes max_sizes;
	max_sizes.max_instances_size = 0;
	max_sizes.max_classes_size = 0;
	for(s32 i = 0; i < ARRAY_SIZE(header.instances); i++) {
		if(missions.has_child(i)) {
			MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if(mission.has_instances()) {
				header.instances[i] = pack_asset_sa<SectorRange>(dest, mission.get_instances(), game);
			}
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.data); i++) {
		if(missions.has_child(i)) {
			MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			MissionHeader mission_header = {0};
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			dest.write(mission_header);
			if(mission.has_instances()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_instances(), game, 0x10);
				
				max_sizes.max_instances_size = std::max(max_sizes.max_instances_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, "msinstncs", 8);
				
				dest.pad(0x10, 0);
				s64 begin = dest.tell();
				dest.write(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.instances = ByteRange::from_bytes(begin, end - begin);
			}
			if(mission.has_classes()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_classes(), game, 0x10);
				
				max_sizes.max_classes_size = std::max(max_sizes.max_classes_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, "msclasses", 8);
				
				dest.pad(0x10, 0);
				s64 begin = dest.tell();
				dest.write(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.classes = ByteRange::from_bytes(begin, end - begin);
			}
			dest.write(mission_header_ofs, mission_header);
			header.data[i] = SectorRange::from_bytes(mission_header_ofs, dest.tell() - mission_header_ofs);
		} else {
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			MissionHeader mission_header = {0};
			mission_header.instances.offset = -1;
			mission_header.classes.offset = -1;
			dest.write(mission_header);
			header.data[i] = SectorRange::from_bytes(mission_header_ofs, dest.tell() - mission_header_ofs);
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.sound_banks); i++) {
		if(missions.has_child(i)) {
			MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if(mission.has_sound_bank()) {
				header.sound_banks[i] = pack_asset_sa<SectorRange>(dest, mission.get_sound_bank(), game);
			}
		}
	}
	return {header, max_sizes};
}
