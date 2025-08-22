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

#include "level_data_wad.h"

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/level_core.h>
#include <wrenchbuild/level/instances_asset.h>

packed_struct(RacLevelDataHeader,
	/* 0x00 */ ByteRange overlay;
	/* 0x08 */ ByteRange sound_bank;
	/* 0x10 */ ByteRange core_index;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange core_data;
)

packed_struct(GcUyaLevelDataHeader,
	/* 0x00 */ ByteRange overlay;
	/* 0x08 */ ByteRange core_index;
	/* 0x10 */ ByteRange gs_ram;
	/* 0x18 */ ByteRange hud_header;
	/* 0x20 */ ByteRange hud_banks[5];
	/* 0x48 */ ByteRange core_data;
	/* 0x50 */ ByteRange transition_textures;
)

packed_struct(DlLevelDataHeader,
	/* 0x00 */ ByteRange moby8355_pvars;
	/* 0x08 */ ByteRange overlay;
	/* 0x10 */ ByteRange core_index;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange core_data;
	/* 0x58 */ ByteRange art_instances;
	/* 0x60 */ ByteRange gameplay_core;
	/* 0x68 */ ByteRange global_nav_data;
)

template <typename Header>
static bool test_level_data_wad(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode);
static ByteRange write_vector_of_bytes(OutputStream& dest, std::vector<u8>& bytes);

void unpack_rac_level_data_wad(LevelWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<RacLevelDataHeader>(0);
	
	unpack_level_core(dest, src, header.core_index, header.core_data, header.gs_ram, config);
	
	unpack_asset(dest.overlay<ElfFileAsset>(), src, header.overlay, config, FMT_ELFFILE_RATCHET_EXECUTABLE);
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
}

void pack_rac_level_data_wad(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config)
{
	RacLevelDataHeader header = {};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	std::vector<u8> index;
	std::vector<u8> data;
	std::vector<u8> gs_ram;
	pack_level_core(index, data, gs_ram, chunks, src, config);
	
	header.overlay = pack_asset<ByteRange>(dest, src.get_overlay(), config, 0x40, FMT_ELFFILE_RATCHET_EXECUTABLE, &empty);
	header.sound_bank = pack_asset<ByteRange>(dest, src.get_sound_bank(), config, 0x40, FMT_NO_HINT, &empty);
	header.core_index = write_vector_of_bytes(dest, index);
	header.gs_ram = write_vector_of_bytes(dest, gs_ram);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hud_bank", FMT_NO_HINT);
	header.core_data = write_vector_of_bytes(dest, data);
	
	dest.write(0, header);
}

void unpack_gc_uya_level_data_wad(LevelWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<GcUyaLevelDataHeader>(0);
	
	unpack_level_core(dest, src, header.core_index, header.core_data, header.gs_ram, config);
	
	unpack_asset(dest.overlay<ElfFileAsset>(), src, header.overlay, config, FMT_ELFFILE_RATCHET_EXECUTABLE);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
	unpack_compressed_asset(dest.transition_textures<CollectionAsset>(SWITCH_FILES), src, header.transition_textures, config, FMT_COLLECTION_PIF8);
}

void pack_gc_uya_level_data_wad(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config)
{
	GcUyaLevelDataHeader header = {};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	std::vector<u8> index;
	std::vector<u8> data;
	std::vector<u8> gs_ram;
	pack_level_core(index, data, gs_ram, chunks, src, config);
	
	header.overlay = pack_asset<ByteRange>(dest, src.get_overlay(), config, 0x40, FMT_ELFFILE_RATCHET_EXECUTABLE, &empty);
	header.core_index = write_vector_of_bytes(dest, index);
	header.gs_ram = write_vector_of_bytes(dest, gs_ram);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hud_bank", FMT_NO_HINT);
	header.core_data = write_vector_of_bytes(dest, data);
	if (src.has_transition_textures()) {
		header.transition_textures = pack_compressed_asset<ByteRange>(dest, src.get_transition_textures(), config, 0x40, "transition", FMT_COLLECTION_PIF8);
	} else {
		header.transition_textures = {-1, 0};
	}
	
	dest.write(0, header);
}

s32 unpack_dl_level_data_wad(LevelWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<DlLevelDataHeader>(0);
	
	unpack_level_core(dest, src, header.core_index, header.core_data, header.gs_ram, config);
	
	unpack_asset(dest.moby8355_pvars(), src, header.moby8355_pvars, config);
	unpack_asset(dest.overlay<ElfFileAsset>(), src, header.overlay, config, FMT_ELFFILE_RATCHET_EXECUTABLE);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
	
	std::vector<u8> gameplay = src.read_multiple<u8>(header.gameplay_core.offset, header.gameplay_core.size);
	std::vector<u8> art_instances = src.read_multiple<u8>(header.art_instances.offset, header.art_instances.size);
	s32 core_moby_count = unpack_instances(dest.gameplay<InstancesAsset>(), &dest, gameplay, &art_instances, config, FMT_INSTANCES_GAMEPLAY);
	unpack_compressed_asset(dest.global_nav_data(), src, header.global_nav_data, config);
	
	return core_moby_count;
}

void pack_dl_level_data_wad(
	OutputStream& dest,
	const std::vector<LevelChunk>& chunks,
	std::vector<u8>& art_instances,
	std::vector<u8>& gameplay,
	const LevelWadAsset& src,
	BuildConfig config)
{
	DlLevelDataHeader header = {};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	std::vector<u8> index;
	std::vector<u8> data;
	std::vector<u8> gs_ram;
	pack_level_core(index, data, gs_ram, chunks, src, config);
	
	header.moby8355_pvars = pack_asset<ByteRange>(dest, src.get_moby8355_pvars(), config, 0x40, FMT_NO_HINT, &empty);
	header.overlay = pack_asset<ByteRange>(dest, src.get_overlay(), config, 0x40, FMT_ELFFILE_RATCHET_EXECUTABLE, &empty);
	header.core_index = write_vector_of_bytes(dest, index);
	header.gs_ram = write_vector_of_bytes(dest, gs_ram);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hud_bank");
	header.core_data = write_vector_of_bytes(dest, data);

	
	header.art_instances = write_vector_of_bytes(dest, art_instances);
		header.gameplay_core = write_vector_of_bytes(dest, gameplay);
	
	header.global_nav_data = pack_compressed_asset<ByteRange>(dest, src.get_global_nav_data(), config, 0x40, "globalnav");
	
	dest.write(0, header);
}

template <typename Header>
static bool test_level_data_wad(
	std::vector<u8>& original,
	std::vector<u8>& repacked,
	BuildConfig config,
	const char* hint,
	AssetTestMode mode)
{
	Header original_header = Buffer(original).read<Header>(0, "original level data header");
	Header repacked_header = Buffer(repacked).read<Header>(0, "repacked level data header");
	
	if (original_header.core_index.size != repacked_header.core_index.size) {
		if (mode == AssetTestMode::PRINT_DIFF_ON_FAIL) {
			Buffer original_core_index = Buffer(original).subbuf(original_header.core_index.offset, original_header.core_index.size);
			Buffer repacked_core_index = Buffer(repacked).subbuf(repacked_header.core_index.offset, repacked_header.core_index.size);
			
			Buffer original_header = original_core_index.subbuf(0, 0xc0);
			Buffer repacked_header = repacked_core_index.subbuf(0, 0xc0);
			
			printf("Diffing core header...\n");
			diff_buffers(original_header, repacked_header, 0, DIFF_REST_OF_BUFFER, true, nullptr);
			
			LevelCoreHeader original_core_header = original_core_index.read<LevelCoreHeader>(0, "core header");
			LevelCoreHeader repacked_core_header = repacked_core_index.read<LevelCoreHeader>(0, "core header");
			
			// The texture data won't match, so find a good spot to start diffing that's after that.
			s64 original_ofs = original_core_header.part_defs_offset;
			s64 repacked_ofs = repacked_core_header.part_defs_offset;
			
			Buffer original_index = original_core_index.subbuf(original_ofs);
			Buffer repacked_index = repacked_core_index.subbuf(repacked_ofs);
			
			printf("Diffing core index data (starting from 0x%x original, 0x%x repacked)...\n",
				(s32) original_ofs, (s32) repacked_ofs);
			diff_buffers(original_index, repacked_index, 0, DIFF_REST_OF_BUFFER, true, nullptr);
			
			write_file("/tmp/original_level_core_headers.bin", original_core_index);
			write_file("/tmp/repacked_level_core_headers.bin", repacked_core_index);
		}
		return false;
	}
	
	return true;
}

static ByteRange write_vector_of_bytes(OutputStream& dest, std::vector<u8>& bytes)
{
	ByteRange range;
	dest.pad(0x40, 0);
	range.offset = dest.tell();
	dest.write_v(bytes);
	range.size = dest.tell() - range.offset;
	return range;
}
