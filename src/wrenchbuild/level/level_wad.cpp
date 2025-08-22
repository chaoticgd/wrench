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

#include <instancemgr/gameplay.h>
#include <iso/table_of_contents.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/level_chunks.h>
#include <wrenchbuild/level/level_data_wad.h>
#include <wrenchbuild/level/instances_asset.h>

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
	/* 0x1c */ SectorRange gameplay_ntsc;
	/* 0x24 */ SectorRange gameplay_pal;
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

static void unpack_rac_level_wad(
	LevelWadAsset& dest, const RacLevelWadHeader& header, InputStream& src, BuildConfig config);
static void pack_rac_level_wad(
	OutputStream& dest, RacLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config);
static void unpack_gc_uya_level_wad(
	LevelWadAsset& dest, const GcUyaLevelWadHeader& header, InputStream& src, BuildConfig config);
static void pack_gc_uya_level_wad(
	OutputStream& dest, GcUyaLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config);
static void unpack_dl_level_wad(
	LevelWadAsset& dest, const DlLevelWadHeader& header, InputStream& src, BuildConfig config);
static void pack_dl_level_wad(
	OutputStream& dest, DlLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config);
static void unpack_missions(
	LevelWadAsset& dest,
	InputStream& file,
	const MissionWadHeader& ranges,
	s32 core_moby_count,
	BuildConfig config);
static std::pair<MissionWadHeader, MaxMissionSizes> pack_missions(
	OutputStream& dest, const CollectionAsset& missions, BuildConfig config);
template <typename PackerFunc>
static SectorRange pack_data_wad_outer(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config,
	PackerFunc packer);
static SectorRange pack_dl_data_wad_outer(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	std::vector<u8>& art_instances,
	std::vector<u8>& gameplay,
	const LevelWadAsset& src,
	BuildConfig config);
static SectorRange write_gameplay_section(
	OutputStream& dest, const Gameplay& gameplay, BuildConfig config);
static SectorRange write_occlusion_copy(OutputStream& dest, const OcclusionAsset& occlusion, Game game);
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

static void unpack_rac_level_wad(
	LevelWadAsset& dest, const RacLevelWadHeader& header, InputStream& src, BuildConfig config)
{
	dest.set_id(header.id);
	g_asset_unpacker.current_level_id = header.id;
	
	SubInputStream data(src, header.data.bytes());
	unpack_rac_level_data_wad(dest, data, config);
	ByteRange64 gameplay_range = header.gameplay_ntsc.bytes();
	std::vector<u8> gameplay = src.read_multiple<u8>(gameplay_range.offset, gameplay_range.size);
	unpack_instances(dest.gameplay<InstancesAsset>(), &dest, gameplay, nullptr, config, FMT_INSTANCES_GAMEPLAY);
}

static void pack_rac_level_wad(
	OutputStream& dest, RacLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config)
{
	header.id = src.id();
	g_asset_packer_current_level_id = src.id();
	
	Gameplay gameplay = load_gameplay(src.get_gameplay(), &src, src.forest().types(), config, FMT_INSTANCES_GAMEPLAY);
	std::vector<LevelChunk> chunks = load_level_chunks(src, gameplay, config);
	
	header.data = pack_data_wad_outer(dest, chunks, src, config, pack_rac_level_data_wad);
	header.gameplay_ntsc = write_gameplay_section(dest, gameplay, config);
	header.gameplay_pal = header.gameplay_ntsc; // TODO: Pack and write out separate files.
	if (src.has_occlusion()) {
		header.occlusion = write_occlusion_copy(dest, src.get_occlusion(), config.game());
	}
}

static void unpack_gc_68_level_wad(
	LevelWadAsset& dest, const GcLevelWadHeader68& header, InputStream& src, BuildConfig config)
{
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	g_asset_unpacker.current_level_id = header.id;
	
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	SubInputStream data(src, header.data.bytes());
	unpack_gc_uya_level_data_wad(dest, data, config);
	
	ByteRange64 gameplay_range = header.gameplay_ntsc.bytes();
	std::vector<u8> gameplay = src.read_multiple<u8>(gameplay_range.offset, gameplay_range.size);
	unpack_instances(dest.gameplay<InstancesAsset>(), &dest, gameplay, nullptr, config, FMT_INSTANCES_GAMEPLAY);
	
	ChunkWadHeader chunks;
	memcpy(&chunks.chunks, header.chunks, sizeof(chunks.chunks));
	memcpy(&chunks.sound_banks, header.chunk_banks, sizeof(chunks.sound_banks));
	unpack_level_chunks(dest.chunks(), src, chunks, config);
}

static void unpack_gc_uya_level_wad(
	LevelWadAsset& dest, const GcUyaLevelWadHeader& header, InputStream& src, BuildConfig config)
{
	if (header.header_size == 0x68) {
		unpack_gc_68_level_wad(dest, src.read<GcLevelWadHeader68>(0), src, config);
		return;
	}
	
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	g_asset_unpacker.current_level_id = header.id;
	
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	SubInputStream data(src, header.data.bytes());
	unpack_gc_uya_level_data_wad(dest, data, config);
	
	ByteRange64 gameplay_range = header.gameplay.bytes();
	fflush(stdout);
	std::vector<u8> gameplay = src.read_multiple<u8>(gameplay_range.offset, gameplay_range.size);
	unpack_instances(dest.gameplay<InstancesAsset>(), &dest, gameplay, nullptr, config, FMT_INSTANCES_GAMEPLAY);
	
	unpack_level_chunks(dest.chunks(), src, header.chunks, config);
}

static void pack_gc_uya_level_wad(
	OutputStream& dest, GcUyaLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config)
{
	header.id = src.id();
	header.reverb = src.reverb();
	g_asset_packer_current_level_id = src.id();
	
	Gameplay gameplay = load_gameplay(src.get_gameplay(), &src, src.forest().types(), config, FMT_INSTANCES_GAMEPLAY);
	std::vector<LevelChunk> chunks = load_level_chunks(src, gameplay, config);
	
	header.sound_bank = pack_asset_sa<SectorRange>(dest, src.get_sound_bank(), config);
	header.data = pack_data_wad_outer(dest, chunks, src, config, pack_gc_uya_level_data_wad);
	header.gameplay = write_gameplay_section(dest, gameplay, config);
	if (src.has_occlusion()) {
		header.occlusion = write_occlusion_copy(dest, src.get_occlusion(), config.game());
	}
	header.chunks = write_level_chunks(dest, chunks);
}

static void unpack_dl_level_wad(
	LevelWadAsset& dest, const DlLevelWadHeader& header, InputStream& src, BuildConfig config)
{
	dest.set_id(header.id);
	dest.set_reverb(header.reverb);
	g_asset_unpacker.current_level_id = header.id;
	
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	SubInputStream data(src, header.data.bytes());
	s32 core_moby_count = unpack_dl_level_data_wad(dest, data, config);
	unpack_level_chunks(dest.chunks(), src, header.chunks, config);
	unpack_missions(dest, src, header.missions, core_moby_count, config);
}

static void pack_dl_level_wad(
	OutputStream& dest, DlLevelWadHeader& header, const LevelWadAsset& src, BuildConfig config)
{
	header.id = src.id();
	header.reverb = src.reverb();
	g_asset_packer_current_level_id = src.id();
	
	Gameplay gameplay = load_gameplay(src.get_gameplay(), &src, src.forest().types(), config, FMT_INSTANCES_GAMEPLAY);
	std::vector<LevelChunk> chunks = load_level_chunks(src, gameplay, config);
	
	std::vector<u8> compressed_art_instances, compressed_gameplay;
	if (!g_asset_packer_dry_run) {
		std::vector<u8> art_instances_buffer = write_gameplay(gameplay, config.game(), DL_ART_INSTANCE_BLOCKS);
		compress_wad(compressed_art_instances, art_instances_buffer, "artinsts", 8);
		
		std::vector<u8> gameplay_buffer = write_gameplay(gameplay, config.game(), DL_GAMEPLAY_CORE_BLOCKS);
		compress_wad(compressed_gameplay, gameplay_buffer, "artinsts", 8);
	}
	
	header.sound_bank = pack_asset_sa<SectorRange>(dest, src.get_sound_bank(), config);
	header.data = pack_dl_data_wad_outer(dest, chunks, compressed_art_instances, compressed_gameplay, src, config);
	verify_fatal(g_asset_packer_dry_run || (!compressed_gameplay.empty() && !compressed_art_instances.empty()));
	header.chunks = write_level_chunks(dest, chunks);
	header.gameplay = write_section(dest, compressed_gameplay.data(), compressed_gameplay.size());
	std::tie(header.missions, header.max_mission_sizes) = pack_missions(dest, src.get_missions(), config);
	header.art_instances = write_section(dest, compressed_art_instances.data(), compressed_art_instances.size());
}

// These offsets are relative to the beginning of the level file.
packed_struct(MissionHeader,
	/* 0x0 */ ByteRange instances;
	/* 0x8 */ ByteRange classes;
)

static void unpack_missions(
	LevelWadAsset& dest,
	InputStream& file,
	const MissionWadHeader& ranges,
	s32 core_moby_count,
	BuildConfig config)
{
	CollectionAsset& collection = dest.missions();
	for (s32 i = 0; i < ARRAY_SIZE(ranges.data); i++) {
		MissionHeader header = {};
		if (!ranges.data[i].empty()) {
			header = file.read<MissionHeader>(ranges.data[i].offset.bytes());
		}
		if (!header.instances.empty() || !header.classes.empty() || !ranges.sound_banks[i].empty()) {
			std::string path = stringf("missions/%d/mission%d.asset", i, i);
			MissionAsset& mission = collection.foreign_child<MissionAsset>(path, false, i);
			
			// Horrible hack: There are some mission instance files that look
			// more like a gameplay core with empty help message sections, for
			// example level 4 (sarathos), mission 44. I'm not sure what these
			// files are exactly.
			file.seek(header.instances.bytes().offset);
			std::vector<u8> compressed_instances = file.read_multiple<u8>(header.instances.bytes().size);
			std::vector<u8> instances;
			decompress_wad(instances, compressed_instances);
			verify(instances.size() >= 4, "Bad mission instances file.");
			if (*(s32*) instances.data() != 0x90) {
				MemoryInputStream compressed_instances_stream(compressed_instances);
				InstancesAsset& instances_asset = mission.instances<InstancesAsset>();
				std::string hint = stringf("mission,%d", core_moby_count);
				unpack_asset_impl(instances_asset, compressed_instances_stream, nullptr, config, hint.c_str());
				ReferenceAsset& reference = instances_asset.child<ReferenceAsset>("core");
				reference.set_asset(dest.get_gameplay().link_relative_to(dest));
			} else {
				MemoryInputStream instances_stream(instances);
				unpack_asset_impl(mission.instances<BinaryAsset>(), instances_stream, nullptr, config, FMT_INSTANCES_MISSION);
			}
			unpack_compressed_asset(mission.classes<CollectionAsset>(), file, header.classes, config, FMT_COLLECTION_MISSION_CLASSES);
			unpack_asset(mission.sound_bank(), file, ranges.sound_banks[i], config);
		}
	}
}

static std::pair<MissionWadHeader, MaxMissionSizes> pack_missions(
	OutputStream& dest, const CollectionAsset& missions, BuildConfig config)
{
	MissionWadHeader header;
	MaxMissionSizes max_sizes;
	max_sizes.max_instances_size = 0;
	max_sizes.max_classes_size = 0;
	for (s32 i = 0; i < ARRAY_SIZE(header.instances); i++) {
		if (missions.has_child(i)) {
			const MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if (mission.has_instances()) {
				header.instances[i] = pack_asset_sa<SectorRange>(dest, mission.get_instances(), config, FMT_INSTANCES_MISSION);
			}
		}
	}
	for (s32 i = 0; i < ARRAY_SIZE(header.data); i++) {
		if (missions.has_child(i)) {
			const MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			MissionHeader mission_header = {};
			dest.pad(SECTOR_SIZE, 0);
			s64 mission_header_ofs = dest.tell();
			dest.write(mission_header);
			if (mission.has_instances()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_instances(), config, 0x10, FMT_INSTANCES_MISSION);
				
				max_sizes.max_instances_size = std::max(max_sizes.max_instances_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, "msinstncs", 8);
				
				dest.pad(0x40, 0);
				s64 begin = dest.tell();
				dest.write_n(compressed_bytes.data(), compressed_bytes.size());
				s64 end = dest.tell();
				mission_header.instances = ByteRange::from_bytes(begin, end - begin);
			}
			if (mission.has_classes()) {
				std::vector<u8> bytes;
				MemoryOutputStream stream(bytes);
				pack_asset<ByteRange>(stream, mission.get_classes(), config, 0x10, FMT_COLLECTION_MISSION_CLASSES);
				
				max_sizes.max_classes_size = std::max(max_sizes.max_classes_size, (s32) bytes.size());
				
				std::vector<u8> compressed_bytes;
				compress_wad(compressed_bytes, bytes, "msclasses", 8);
				
				dest.pad(0x40, 0);
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
	for (s32 i = 0; i < ARRAY_SIZE(header.sound_banks); i++) {
		if (missions.has_child(i)) {
			const MissionAsset& mission = missions.get_child(i).as<MissionAsset>();
			if (mission.has_sound_bank()) {
				header.sound_banks[i] = pack_asset_sa<SectorRange>(dest, mission.get_sound_bank(), config);
			}
		}
	}
	return {header, max_sizes};
}

template <typename PackerFunc>
static SectorRange pack_data_wad_outer(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config,
	PackerFunc packer)
{
	SectorRange range;
	dest.pad(SECTOR_SIZE, 0);
	range.offset.sectors = dest.tell() / SECTOR_SIZE;
	SubOutputStream data_dest(dest, dest.tell());
	packer(data_dest, chunks, src, config);
	range.size = Sector32::size_from_bytes(data_dest.tell());
	return range;
}

static SectorRange pack_dl_data_wad_outer(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	std::vector<u8>& art_instances,
	std::vector<u8>& gameplay,
	const LevelWadAsset& src,
	BuildConfig config)
{
	SectorRange range;
	dest.pad(SECTOR_SIZE, 0);
	range.offset.sectors = dest.tell() / SECTOR_SIZE;
	SubOutputStream data_dest(dest, dest.tell());
	pack_dl_level_data_wad(dest, chunks, art_instances, gameplay, src, config);
	range.size = Sector32::size_from_bytes(data_dest.tell());
	return range;
}

static SectorRange write_gameplay_section(
	OutputStream& dest, const Gameplay& gameplay, BuildConfig config)
{
	if (g_asset_packer_dry_run) {
		return {0, 0};
	}
	std::vector<u8> buffer = write_gameplay(gameplay, config.game(), *gameplay_block_descriptions_from_game(config.game()));
	std::vector<u8> compressed;
	compress_wad(compressed, buffer, "gameplay", 8);
	dest.pad(SECTOR_SIZE, 0);
	return write_section(dest, compressed.data(), compressed.size());
}

static SectorRange write_occlusion_copy(OutputStream& dest, const OcclusionAsset& occlusion, Game game)
{
	if (g_asset_packer_dry_run) {
		return {0, 0};
	}
	std::unique_ptr<InputStream> stream = occlusion.mappings().open_binary_file_for_reading();
	std::vector<u8> buffer = stream->read_multiple<u8>(stream->size());
	return write_section(dest, buffer.data(), buffer.size());
}

static SectorRange write_section(OutputStream& dest, const u8* src, s64 size)
{
	SectorRange range;
	dest.pad(SECTOR_SIZE, 0);
	range.offset.sectors = dest.tell() / SECTOR_SIZE;
	dest.write_n(src, size);
	range.size = Sector32::size_from_bytes(dest.tell() - range.offset.bytes());
	return range;
}
