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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/level_core.h>

packed_struct(RacLevelDataHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange sound_bank;
	/* 0x10 */ ByteRange core_index;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange core_data;
)

packed_struct(GcUyaLevelDataHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange core_index;
	/* 0x10 */ ByteRange gs_ram;
	/* 0x18 */ ByteRange hud_header;
	/* 0x20 */ ByteRange hud_banks[5];
	/* 0x48 */ ByteRange core_data;
	/* 0x50 */ ByteRange transition_textures;
)

packed_struct(DlLevelDataHeader,
	/* 0x00 */ ByteRange moby8355_pvars;
	/* 0x08 */ ByteRange code;
	/* 0x10 */ ByteRange core_index;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange core_data;
	/* 0x58 */ ByteRange art_instances;
	/* 0x60 */ ByteRange gameplay;
	/* 0x68 */ ByteRange global_nav_data;
)

static void unpack_rac_level_data_wad(LevelDataWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_rac_level_data_wad(OutputStream& dest, const LevelDataWadAsset& src, BuildConfig config);
static void unpack_gc_uya_level_data_wad(LevelDataWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_gc_uya_level_data_wad(OutputStream& dest, const LevelDataWadAsset& src, BuildConfig config);
static void unpack_dl_level_data_wad(LevelDataWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_dl_level_data_wad(OutputStream& dest, const LevelDataWadAsset& src, BuildConfig config);
static ByteRange write_vector_of_bytes(OutputStream& dest, std::vector<u8>& bytes);

on_load(LevelData, []() {
	LevelDataWadAsset::funcs.unpack_rac1 = wrap_unpacker_func<LevelDataWadAsset>(unpack_rac_level_data_wad);
	LevelDataWadAsset::funcs.unpack_rac2 = wrap_unpacker_func<LevelDataWadAsset>(unpack_gc_uya_level_data_wad);
	LevelDataWadAsset::funcs.unpack_rac3 = wrap_unpacker_func<LevelDataWadAsset>(unpack_gc_uya_level_data_wad);
	LevelDataWadAsset::funcs.unpack_dl = wrap_unpacker_func<LevelDataWadAsset>(unpack_dl_level_data_wad);
	
	LevelDataWadAsset::funcs.pack_rac1 = wrap_packer_func<LevelDataWadAsset>(pack_rac_level_data_wad);
	LevelDataWadAsset::funcs.pack_rac2 = wrap_packer_func<LevelDataWadAsset>(pack_gc_uya_level_data_wad);
	LevelDataWadAsset::funcs.pack_rac3 = wrap_packer_func<LevelDataWadAsset>(pack_gc_uya_level_data_wad);
	LevelDataWadAsset::funcs.pack_dl = wrap_packer_func<LevelDataWadAsset>(pack_dl_level_data_wad);
})

static void unpack_rac_level_data_wad(LevelDataWadAsset& dest, InputStream& src, BuildConfig config) {
	auto header = src.read<RacLevelDataHeader>(0);
	
	unpack_level_core(dest.core(), src, header.core_index, header.core_data, header.gs_ram, config);
	
	unpack_asset(dest.code(), src, header.code, config);
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
}

static void pack_rac_level_data_wad(OutputStream& dest, const LevelDataWadAsset& src, BuildConfig config) {
	RacLevelDataHeader header = {0};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	std::vector<u8> index;
	std::vector<u8> data;
	std::vector<u8> gs_ram;
	pack_level_core(index, data, gs_ram, src.get_core(), config);
	
	header.code = pack_asset<ByteRange>(dest, src.get_code(), config, 0x40, FMT_NO_HINT, &empty);
	header.sound_bank = pack_asset<ByteRange>(dest, src.get_sound_bank(), config, 0x40, FMT_NO_HINT, &empty);
	header.core_index = write_vector_of_bytes(dest, index);
	header.gs_ram = write_vector_of_bytes(dest, gs_ram);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hud_bank", FMT_NO_HINT);
	header.core_data = write_vector_of_bytes(dest, data);
	
	dest.write(0, header);
}

static void unpack_gc_uya_level_data_wad(LevelDataWadAsset& dest, InputStream& src, BuildConfig config) {
	auto header = src.read<GcUyaLevelDataHeader>(0);
	
	unpack_level_core(dest.core(), src, header.core_index, header.core_data, header.gs_ram, config);
	
	unpack_asset(dest.code(), src, header.code, config);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
	unpack_compressed_asset(dest.transition_textures<CollectionAsset>(SWITCH_FILES), src, header.transition_textures, config, FMT_COLLECTION_PIF8);
}

static void pack_gc_uya_level_data_wad(OutputStream& dest, const LevelDataWadAsset& src, BuildConfig config) {
	GcUyaLevelDataHeader header = {0};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	std::vector<u8> index;
	std::vector<u8> data;
	std::vector<u8> gs_ram;
	pack_level_core(index, data, gs_ram, src.get_core(), config);
	
	header.code = pack_asset<ByteRange>(dest, src.get_code(), config, 0x40, FMT_NO_HINT, &empty);
	header.core_index = write_vector_of_bytes(dest, index);
	header.gs_ram = write_vector_of_bytes(dest, gs_ram);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hud_bank", FMT_NO_HINT);
	header.core_data = write_vector_of_bytes(dest, data);
	if(src.has_transition_textures()) {
		header.transition_textures = pack_compressed_asset<ByteRange>(dest, src.get_transition_textures(), config, 0x40, "transition", FMT_COLLECTION_PIF8);
	} else {
		header.transition_textures = {-1, 0};
	}
	
	dest.write(0, header);
}

static void unpack_dl_level_data_wad(LevelDataWadAsset& dest, InputStream& src, BuildConfig config) {
	auto header = src.read<DlLevelDataHeader>(0);
	
	unpack_level_core(dest.core(), src, header.core_index, header.core_data, header.gs_ram, config);
	
	unpack_asset(dest.moby8355_pvars(), src, header.moby8355_pvars, config);
	unpack_asset(dest.code(), src, header.code, config);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
	unpack_compressed_asset(dest.art_instances(), src, header.art_instances, config);
	unpack_compressed_asset(dest.gameplay(), src, header.gameplay, config);
	unpack_compressed_asset(dest.global_nav_data(), src, header.global_nav_data, config);
}

static void pack_dl_level_data_wad(OutputStream& dest, const LevelDataWadAsset& src, BuildConfig config) {
	DlLevelDataHeader header = {0};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	std::vector<u8> index;
	std::vector<u8> data;
	std::vector<u8> gs_ram;
	pack_level_core(index, data, gs_ram, src.get_core(), config);
	
	header.moby8355_pvars = pack_asset<ByteRange>(dest, src.get_moby8355_pvars(), config, 0x40, FMT_NO_HINT, &empty);
	header.code = pack_asset<ByteRange>(dest, src.get_code(), config, 0x40, FMT_NO_HINT, &empty);
	header.core_index = write_vector_of_bytes(dest, index);
	header.gs_ram = write_vector_of_bytes(dest, gs_ram);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hud_bank");
	header.core_data = write_vector_of_bytes(dest, data);
	header.art_instances = pack_compressed_asset<ByteRange>(dest, src.get_art_instances(), config, 0x40, "art_insts");
	header.gameplay = pack_compressed_asset<ByteRange>(dest, src.get_gameplay(), config, 0x40, "gameplay");
	header.global_nav_data = pack_compressed_asset<ByteRange>(dest, src.get_global_nav_data(), config, 0x40, "globalnav");
	
	dest.write(0, header);
}

static ByteRange write_vector_of_bytes(OutputStream& dest, std::vector<u8>& bytes) {
	ByteRange range;
	dest.pad(0x40, 0);
	range.offset = dest.tell();
	dest.write_v(bytes);
	range.size = dest.tell() - range.offset;
	return range;
}
