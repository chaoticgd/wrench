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

packed_struct(Rac1LevelDataHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange core_sound_bank;
	/* 0x10 */ ByteRange asset_header;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange assets;
)

packed_struct(Rac23LevelDataWadHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange asset_header;
	/* 0x10 */ ByteRange gs_ram;
	/* 0x18 */ ByteRange hud_header;
	/* 0x20 */ ByteRange hud_banks[5];
	/* 0x48 */ ByteRange assets;
	/* 0x50 */ ByteRange transition_textures;
)

packed_struct(DeadlockedLevelDataHeader,
	/* 0x00 */ ByteRange moby8355_pvars;
	/* 0x08 */ ByteRange code;
	/* 0x10 */ ByteRange asset_header;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange assets;
	/* 0x58 */ ByteRange art_instances;
	/* 0x60 */ ByteRange gameplay_core;
	/* 0x68 */ ByteRange global_nav_data;
)

static void unpack_rac1_level_data_wad(LevelDataWadAsset& dest, InputStream& src, Game game);
static void pack_rac1_level_data_wad(OutputStream& dest, LevelDataWadAsset& src, Game game);
static void unpack_rac23_level_data_wad(LevelDataWadAsset& dest, InputStream& src, Game game);
static void pack_rac23_level_data_wad(OutputStream& dest, LevelDataWadAsset& src, Game game);
static void unpack_dl_level_data_wad(LevelDataWadAsset& dest, InputStream& src, Game game);
static void pack_dl_level_data_wad(OutputStream& dest, LevelDataWadAsset& src, Game game);

on_load(LevelData, []() {
	LevelDataWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<LevelDataWadAsset>(unpack_rac1_level_data_wad);
	LevelDataWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<LevelDataWadAsset>(unpack_rac23_level_data_wad);
	LevelDataWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<LevelDataWadAsset>(unpack_rac23_level_data_wad);
	LevelDataWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<LevelDataWadAsset>(unpack_dl_level_data_wad);
	
	LevelDataWadAsset::funcs.pack_rac1 = wrap_packer_func<LevelDataWadAsset>(pack_rac1_level_data_wad);
	LevelDataWadAsset::funcs.pack_rac2 = wrap_packer_func<LevelDataWadAsset>(pack_rac23_level_data_wad);
	LevelDataWadAsset::funcs.pack_rac3 = wrap_packer_func<LevelDataWadAsset>(pack_rac23_level_data_wad);
	LevelDataWadAsset::funcs.pack_dl = wrap_packer_func<LevelDataWadAsset>(pack_dl_level_data_wad);
})

static void unpack_rac1_level_data_wad(LevelDataWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac1LevelDataHeader>(0);
	
	unpack_asset(dest.code(), src, header.code, game);
	unpack_asset(dest.core_sound_bank(), src, header.core_sound_bank, game);
	unpack_asset(dest.asset_header(), src, header.asset_header, game);
	unpack_asset(dest.gs_ram(), src, header.gs_ram, game);
	unpack_asset(dest.hud_header(), src, header.hud_header, game);
	unpack_assets<BinaryAsset>(dest.hud_banks().switch_files(), src, ARRAY_PAIR(header.hud_banks), game);
	unpack_asset(dest.assets(), src, header.assets, game);
}

static void pack_rac1_level_data_wad(OutputStream& dest, LevelDataWadAsset& src, Game game) {
	Rac1LevelDataHeader header = {0};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	header.code = pack_asset<ByteRange>(dest, src.get_code(), game, 0x40, FMT_NO_HINT, &empty);
	header.core_sound_bank = pack_asset<ByteRange>(dest, src.get_core_sound_bank(), game, 0x40, FMT_NO_HINT, &empty);
	header.asset_header = pack_asset<ByteRange>(dest, src.get_asset_header(), game, 0x40, FMT_NO_HINT, &empty);
	header.gs_ram = pack_asset<ByteRange>(dest, src.get_gs_ram(), game, 0x40, FMT_NO_HINT, &empty);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), game, 0x40, FMT_NO_HINT, &empty);
	pack_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), game, 0x40, FMT_NO_HINT, &empty);
	header.assets = pack_asset<ByteRange>(dest, src.get_assets(), game, 0x40, FMT_NO_HINT, &empty);
	
	dest.write(0, header);
}

static void unpack_rac23_level_data_wad(LevelDataWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac23LevelDataWadHeader>(0);
	
	unpack_asset(dest.code(), src, header.code, game);
	unpack_asset(dest.asset_header(), src, header.asset_header, game);
	unpack_asset(dest.gs_ram(), src, header.gs_ram, game);
	unpack_asset(dest.hud_header(), src, header.hud_header, game);
	unpack_assets<BinaryAsset>(dest.hud_banks().switch_files(), src, ARRAY_PAIR(header.hud_banks), game);
	unpack_asset(dest.assets(), src, header.assets, game);
	unpack_asset(dest.transition_textures(), src, header.transition_textures, game);
}

static void pack_rac23_level_data_wad(OutputStream& dest, LevelDataWadAsset& src, Game game) {
	Rac23LevelDataWadHeader header = {0};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	header.code = pack_asset<ByteRange>(dest, src.get_code(), game, 0x40, FMT_NO_HINT, &empty);
	header.asset_header = pack_asset<ByteRange>(dest, src.get_asset_header(), game, 0x40, FMT_NO_HINT, &empty);
	header.gs_ram = pack_asset<ByteRange>(dest, src.get_gs_ram(), game, 0x40, FMT_NO_HINT, &empty);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), game, 0x40, FMT_NO_HINT, &empty);
	pack_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), game, 0x40, FMT_NO_HINT, &empty);
	header.assets = pack_asset<ByteRange>(dest, src.get_assets(), game, 0x40, FMT_NO_HINT, &empty);
	header.transition_textures = pack_asset<ByteRange>(dest, src.get_transition_textures(), game, 0x40, FMT_NO_HINT, &empty);
	
	dest.write(0, header);
}

static void unpack_dl_level_data_wad(LevelDataWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedLevelDataHeader>(0);
	
	unpack_asset(dest.moby8355_pvars(), src, header.moby8355_pvars, game);
	unpack_asset(dest.code(), src, header.code, game);
	unpack_asset(dest.asset_header(), src, header.asset_header, game);
	unpack_asset(dest.gs_ram(), src, header.gs_ram, game);
	unpack_asset(dest.hud_header(), src, header.hud_header, game);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks().switch_files(), src, ARRAY_PAIR(header.hud_banks), game);
	unpack_compressed_asset(dest.assets(), src, header.assets, game);
	unpack_compressed_asset(dest.art_instances(), src, header.art_instances, game);
	unpack_compressed_asset(dest.gameplay_core(), src, header.gameplay_core, game);
	unpack_asset(dest.global_nav_data(), src, header.global_nav_data, game);
}

static void pack_dl_level_data_wad(OutputStream& dest, LevelDataWadAsset& src, Game game) {
	DeadlockedLevelDataHeader header = {0};
	dest.write(header);
	ByteRange empty = {-1, 0};
	
	header.moby8355_pvars = pack_asset<ByteRange>(dest, src.get_moby8355_pvars(), game, 0x40, FMT_NO_HINT, &empty);
	header.code = pack_asset<ByteRange>(dest, src.get_code(), game, 0x40, FMT_NO_HINT, &empty);
	header.asset_header = pack_asset<ByteRange>(dest, src.get_asset_header(), game, 0x40, FMT_NO_HINT, &empty);
	header.gs_ram = pack_asset<ByteRange>(dest, src.get_gs_ram(), game, 0x40, FMT_NO_HINT, &empty);
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), game, 0x40, FMT_NO_HINT, &empty);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), game, 0x40, "hud_bank");
	header.assets = pack_compressed_asset<ByteRange>(dest, src.get_assets(), game, 0x40, "assetwad");
	header.art_instances = pack_compressed_asset<ByteRange>(dest, src.get_art_instances(), game, 0x40, "art_insts");
	header.gameplay_core = pack_compressed_asset<ByteRange>(dest, src.get_gameplay_core(), game, 0x40, "gameplay");
	header.global_nav_data = pack_asset<ByteRange>(dest, src.get_global_nav_data(), game, 0x40, FMT_NO_HINT, &empty);
	
	dest.write(0, header);
}
