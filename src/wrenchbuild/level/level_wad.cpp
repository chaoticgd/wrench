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

#include <iso/table_of_contents.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/level_data_wad.h>

packed_struct(ChunkWadHeader,
	/* 0x00 */ SectorRange chunks[3];
	/* 0x18 */ SectorRange sound_banks[3];
)

packed_struct(GcUyaLevelWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s32 id;
	/* 0x0c */ s32 reverb;
	/* 0x10 */ SectorRange data;
	/* 0x18 */ SectorRange sound_bank;
	/* 0x20 */ SectorRange gameplay;
	/* 0x28 */ SectorRange occlusion;
	/* 0x30 */ ChunkWadHeader chunks;
)
static_assert(sizeof(GcUyaLevelWadHeader) == 0x60);

packed_struct(GcLevelWadHeader68,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s32 id;
	/* 0x0c */ SectorRange data;
	/* 0x14 */ SectorRange sound_bank;
	/* 0x1c */ SectorRange gameplay_1;
	/* 0x24 */ SectorRange gameplay_2;
	/* 0x2c */ SectorRange occlusion;
	/* 0x34 */ SectorRange chunks[3];
	/* 0x4c */ s32 reverb;
	/* 0x50 */ SectorRange chunk_banks[3];
)
static_assert(sizeof(GcLevelWadHeader68) == 0x68);

packed_struct(MaxMissionSizes,
	/* 0x0 */ s32 max_instances_size;
	/* 0x4 */ s32 max_classes_size;
)

packed_struct(MissionWadHeader,
	/* 0x000 */ SectorRange instances[128];
	/* 0x400 */ SectorRange data[128];
	/* 0x800 */ SectorRange sound_banks[128];
)

packed_struct(DlLevelWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ s32 id;
	/* 0x00c */ s32 reverb;
	/* 0x010 */ MaxMissionSizes max_mission_sizes;
	/* 0x018 */ SectorRange data;
	/* 0x020 */ SectorRange sound_bank;
	/* 0x028 */ ChunkWadHeader chunks;
	/* 0x058 */ SectorRange gameplay;
	/* 0x060 */ MissionWadHeader missions;
	/* 0xc60 */ SectorRange art_instances;
)

static void unpack_rac_level_wad(LevelWadAsset& dest, const RacLevelWadHeader& header, InputStream& src, BuildConfig config);
static void pack_rac_level_wad(OutputStream& dest, RacLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config);
static void unpack_gc_uya_level_wad(LevelWadAsset& dest, const GcUyaLevelWadHeader& header, InputStream& src, BuildConfig config);
static void pack_gc_uya_level_wad(OutputStream& dest, GcUyaLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config);
static void unpack_dl_level_wad(LevelWadAsset& dest, const DlLevelWadHeader& header, InputStream& src, BuildConfig config);
static void pack_dl_level_wad(OutputStream& dest, DlLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config);
static void unpack_chunks(CollectionAsset& dest, InputStream& file, const ChunkWadHeader& ranges, BuildConfig config);
static ChunkWadHeader pack_chunks(OutputStream& dest, const CollectionAsset& chunks, BuildConfig config);
static void unpack_missions(CollectionAsset& dest, InputStream& file, const MissionWadHeader& ranges, BuildConfig config);
static std::pair<MissionWadHeader, MaxMissionSizes> pack_missions(OutputStream& dest, const CollectionAsset& missions, BuildConfig config);
template <typename PackerFunc>
static SectorRange pack_data_wad(OutputStream& dest, const LevelWadAsset& src, BuildConfig config, PackerFunc packer);
static SectorRange pack_gameplay(OutputStream& dest, std::vector<u8>& gameplay, const BinaryAsset& asset, BuildConfig config);
static SectorRange write_occlusion_copy(OutputStream& dest, const std::vector<u8>& gameplay, s32 field);
static SectorRange write_section(OutputStream& dest, const u8* src, s64 size);

on_load(Level, []() {
	LevelWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<LevelWadAsset, RacLevelWadHeader>(unpack_rac_level_wad);
	LevelWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<LevelWadAsset, GcUyaLevelWadHeader>(unpack_gc_uya_level_wad);
	LevelWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<LevelWadAsset, GcUyaLevelWadHeader>(unpack_gc_uya_level_wad);
	LevelWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<LevelWadAsset, DlLevelWadHeader>(unpack_dl_level_wad);
	
	LevelWadAsset::funcs.pack_rac1 = wrap_wad_packer_func<LevelWadAsset, RacLevelWadHeader>(pack_rac_level_wad);
	LevelWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<LevelWadAsset, GcUyaLevelWadHeader>(pack_gc_uya_level_wad);
	LevelWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<LevelWadAsset, GcUyaLevelWadHeader>(pack_gc_uya_level_wad);
	LevelWadAsset::funcs.pack_dl = wrap_wad_packer_func<LevelWadAsset, DlLevelWadHeader>(pack_dl_level_wad);
})

static void unpack_rac_level_wad(LevelWadAsset& dest, const RacLevelWadHeader& header, InputStream& src, BuildConfig config) {
	dest.set_id(header.id);
	g_asset_unpacker.current_level_id = header.id;
	
	SubInputStream data(src, header.data.bytes());
	unpack_rac_level_data_wad(dest, data, config);
	unpack_compressed_asset(dest.gameplay(), src, header.gameplay_ntsc, config);
}

static void pack_rac_level_wad(OutputStream& dest, RacLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config) {
	header.id = src.id();
	g_asset_packer_current_level_id = src.id();
	
	header.data = pack_data_wad(dest, src, config, pack_rac_level_data_wad);
	std::vector<u8> gameplay;
	header.gameplay_ntsc = pack_gameplay(dest, gameplay, src.get_gameplay(), config);
	header.gameplay_pal = header.gameplay_ntsc; // TODO: Pack and write out separate files.
	header.occlusion = write_occlusion_copy(dest, gameplay, 0x8c);
}

static void unpack_gc_uya_level_wad(LevelWadAsset& dest, const GcUyaLevelWadHeader& header, InputStream& src, BuildConfig config) {
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	g_asset_unpacker.current_level_id = header.id;
	
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	SubInputStream data(src, header.data.bytes());
	unpack_gc_uya_level_data_wad(dest, data, config);
	unpack_compressed_asset(dest.gameplay(), src, header.gameplay, config);
	unpack_chunks(dest.chunks(), src, header.chunks, config);
}

static void pack_gc_uya_level_wad(OutputStream& dest, GcUyaLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config) {
	header.id = src.id();
	header.reverb = src.reverb();
	g_asset_packer_current_level_id = src.id();
	
	header.sound_bank = pack_asset_sa<SectorRange>(dest, src.get_sound_bank(), config);
	header.data = pack_data_wad(dest, src, config, pack_gc_uya_level_data_wad);
	std::vector<u8> gameplay;
	header.gameplay = pack_gameplay(dest, gameplay, src.get_gameplay(), config);
	header.occlusion = write_occlusion_copy(dest, gameplay, 0x90);
	header.chunks = pack_chunks(dest, src.get_chunks(), config);
}

static void unpack_dl_level_wad(LevelWadAsset& dest, const DlLevelWadHeader& header, InputStream& src, BuildConfig config) {
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	g_asset_unpacker.current_level_id = header.id;
	
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	SubInputStream data(src, header.data.bytes());
	unpack_dl_level_data_wad(dest, data, config);
	unpack_chunks(dest.chunks(), src, header.chunks, config);
	unpack_missions(dest.missions(), src, header.missions, config);
}

static void pack_dl_level_wad(OutputStream& dest, DlLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config) {
	header.id = src.id();
	header.reverb = src.reverb();
	g_asset_packer_current_level_id = src.id();
	
	header.sound_bank = pack_asset_sa<SectorRange>(dest, src.get_sound_bank(), config);
	std::vector<u8> compressed_art_instances, compressed_gameplay;
	header.data = pack_data_wad(dest, src, config, [&](OutputStream& data_dest, const LevelWadAsset& data_src, BuildConfig data_config) {
		pack_dl_level_data_wad(data_dest, compressed_art_instances, compressed_gameplay, data_src, data_config);
	});
	assert(!compressed_gameplay.empty() && !compressed_art_instances.empty());
	header.chunks = pack_chunks(dest, src.get_chunks(), config);
	header.gameplay = write_section(dest, compressed_gameplay.data(), compressed_gameplay.size());
	std::tie(header.missions, header.max_mission_sizes) = pack_missions(dest, src.get_missions(), config);
	header.art_instances = write_section(dest, compressed_art_instances.data(), compressed_art_instances.size());
}

packed_struct(ChunkHeader,
	/* 0x0 */ s32 tfrags;
	/* 0x4 */ s32 collision;
)

static void unpack_chunks(CollectionAsset& dest, InputStream& file, const ChunkWadHeader& ranges, BuildConfig config) {
	for(s32 i = 0; i < ARRAY_SIZE(ranges.chunks); i++) {
		ChunkHeader chunk_header = {};
		if(!ranges.chunks[i].empty()) {
			chunk_header = file.read<ChunkHeader>(ranges.chunks[i].offset.bytes());
		}
		if(chunk_header.tfrags > 0 || chunk_header.collision > 0 || !ranges.sound_banks[i].empty()) {
			ChunkAsset& chunk = dest.foreign_child<ChunkAsset>(stringf("chunks/%d/chunk%d.asset", i, i), i);
			if(chunk_header.tfrags > 0) {
				s64 offset = ranges.chunks[i].offset.bytes() + chunk_header.tfrags;
				s64 size = ranges.chunks[i].size.bytes() - chunk_header.tfrags;
				ByteRange tfrags_range{(s32) offset, (s32) size};
				unpack_compressed_asset(chunk.tfrags(), file, tfrags_range, config);
			}
			if(chunk_header.collision > 0) {
				s64 offset = ranges.chunks[i].offset.bytes() + chunk_header.collision;
				s64 size = ranges.chunks[i].size.bytes() - chunk_header.collision;
				ByteRange collision_range{(s32) offset, (s32) size};
				unpack_compressed_asset(chunk.collision<CollisionAsset>(SWITCH_FILES), file, collision_range, config);
			}
			unpack_asset(chunk.sound_bank(), file, ranges.sound_banks[i], config);
		}
	}
}

static ChunkWadHeader pack_chunks(OutputStream& dest, const CollectionAsset& chunks, BuildConfig config) {
	ChunkWadHeader header = {};
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if(chunks.has_child(i)) {
			const ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
			if(chunk.has_tfrags() || chunk.has_collision()) {
				dest.pad(SECTOR_SIZE, 0);
				s64 chunk_header_ofs = dest.tell();
				SubOutputStream chunk_dest(dest, chunk_header_ofs);
				ChunkHeader chunk_header = {-1, -1};
				static_cast<OutputStream&>(chunk_dest).write(chunk_header);
				if(chunk.has_tfrags()) {
					chunk_header.tfrags = pack_compressed_asset<ByteRange>(dest, chunk.get_tfrags(), config, 0x10, "chnktfrag").offset;
				}
				if(chunk.has_collision()) {
					chunk_header.collision = pack_compressed_asset<ByteRange>(dest, chunk.get_collision(), config, 0x10, "chunkcoll").offset;
				}
				dest.write(chunk_header_ofs, chunk_header);
				header.chunks[i] = SectorRange::from_bytes(chunk_header_ofs, dest.tell() - chunk_header_ofs);
			}
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.chunks); i++) {
		if(chunks.has_child(i)) {
			const ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
			if(chunk.has_sound_bank()) {
				header.sound_banks[i] = pack_asset_sa<SectorRange>(dest, chunk.get_sound_bank(), config);
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

static void unpack_missions(CollectionAsset& dest, InputStream& file, const MissionWadHeader& ranges, BuildConfig config) {
	for(s32 i = 0; i < ARRAY_SIZE(ranges.data); i++) {
		MissionHeader header = {};
		if(!ranges.data[i].empty()) {
			header = file.read<MissionHeader>(ranges.data[i].offset.bytes());
		}
		if(!header.instances.empty() || !header.classes.empty() || !ranges.sound_banks[i].empty()) {
			std::string path = stringf("missions/%d/mission%d.asset", i, i);
			MissionAsset& mission = dest.foreign_child<MissionAsset>(path, i);
			unpack_compressed_asset(mission.instances(), file, header.instances, config);
			unpack_compressed_asset(mission.classes<CollectionAsset>(), file, header.classes, config, FMT_COLLECTION_MISSION_CLASSES);
			unpack_asset(mission.sound_bank(), file, ranges.sound_banks[i], config);
		}
	}
}

static std::pair<MissionWadHeader, MaxMissionSizes> pack_missions(OutputStream& dest, const CollectionAsset& missions, BuildConfig config) {
	MissionWadHeader header;
	MaxMissionSizes max_sizes;
	max_sizes.max_instances_size = 0;
	max_sizes.max_classes_size = 0;
	for(s32 i = 0; i < ARRAY_SIZE(header.instances); i++) {
		if(missions.has_child(i)) {
			const MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if(mission.has_instances()) {
				header.instances[i] = pack_asset_sa<SectorRange>(dest, mission.get_instances(), config);
			}
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.data); i++) {
		if(missions.has_child(i)) {
			const MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			MissionHeader mission_header = {};
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			dest.write(mission_header);
			if(mission.has_instances()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_instances(), config, 0x10);
				
				max_sizes.max_instances_size = std::max(max_sizes.max_instances_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, "msinstncs", 8);
				
				dest.pad(0x10, 0);
				s64 begin = dest.tell();
				dest.write_n(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.instances = ByteRange::from_bytes(begin, end - begin);
			}
			if(mission.has_classes()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_classes(), config, 0x10, FMT_COLLECTION_MISSION_CLASSES);
				
				max_sizes.max_classes_size = std::max(max_sizes.max_classes_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, "msclasses", 8);
				
				dest.pad(0x10, 0);
				s64 begin = dest.tell();
				dest.write_n(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.classes = ByteRange::from_bytes(begin, end - begin);
			}
			dest.write(mission_header_ofs, mission_header);
			header.data[i] = SectorRange::from_bytes(mission_header_ofs, dest.tell() - mission_header_ofs);
		} else {
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			MissionHeader mission_header = {};
			mission_header.instances.offset = -1;
			mission_header.classes.offset = -1;
			dest.write(mission_header);
			header.data[i] = SectorRange::from_bytes(mission_header_ofs, dest.tell() - mission_header_ofs);
		}
	}
	for(s32 i = 0; i < ARRAY_SIZE(header.sound_banks); i++) {
		if(missions.has_child(i)) {
			const MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if(mission.has_sound_bank()) {
				header.sound_banks[i] = pack_asset_sa<SectorRange>(dest, mission.get_sound_bank(), config);
			}
		}
	}
	return {header, max_sizes};
}

template <typename PackerFunc>
static SectorRange pack_data_wad(OutputStream& dest, const LevelWadAsset& src, BuildConfig config, PackerFunc packer) {
	SectorRange range;
	dest.pad(SECTOR_SIZE, 0);
	range.offset.sectors = dest.tell() / SECTOR_SIZE;
	SubOutputStream data_dest(dest, dest.tell());
	packer(data_dest, src, config);
	range.size = Sector32::size_from_bytes(data_dest.tell());
	return range;
}

static SectorRange pack_gameplay(OutputStream& dest, std::vector<u8>& gameplay, const BinaryAsset& asset, BuildConfig config) {
	MemoryOutputStream gameplay_stream(gameplay);
	pack_asset<ByteRange>(gameplay_stream, asset, config, 0x10);
	std::vector<u8> gameplay_compressed;
	compress_wad(gameplay_compressed, gameplay, "gameplay", 8);
	dest.pad(SECTOR_SIZE, 0);
	return write_section(dest, gameplay_compressed.data(), gameplay_compressed.size());
}

static SectorRange write_occlusion_copy(OutputStream& dest, const std::vector<u8>& gameplay, s32 field) {
	if(g_asset_packer_dry_run) {
		return {0, 0};
	}
	s32 occlusion_ofs = Buffer(gameplay).read<s32>(field, "occlusion offset");
	verify(occlusion_ofs < gameplay.size(), "Failed to retrieve occlusion data from gameplay file.");
	return write_section(dest, gameplay.data() + occlusion_ofs, gameplay.size() - occlusion_ofs);
}

static SectorRange write_section(OutputStream& dest, const u8* src, s64 size) {
	SectorRange range;
	dest.pad(SECTOR_SIZE, 0);
	range.offset.sectors = dest.tell() / SECTOR_SIZE;
	dest.write_n(src, size);
	range.size = Sector32::size_from_bytes(dest.tell() - range.offset.bytes());
	return range;
}
