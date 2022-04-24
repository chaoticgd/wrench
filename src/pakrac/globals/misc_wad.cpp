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

#include <engine/compression.h>
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static void unpack_rac2_misc_wad(MiscWadAsset& dest, InputStream& src, Game game);
static void pack_rac2_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game);
static void unpack_rac3_misc_wad(MiscWadAsset& dest, InputStream& src, Game game);
static void pack_rac3_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game);
static void unpack_dl_misc_wad(MiscWadAsset& dest, InputStream& src, Game game);
static void pack_dl_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game);

on_load(Misc, []() {
	MiscWadAsset::funcs.unpack_rac2 = wrap_unpacker_func<MiscWadAsset>(unpack_rac2_misc_wad);
	MiscWadAsset::funcs.unpack_rac2 = wrap_unpacker_func<MiscWadAsset>(unpack_rac3_misc_wad);
	MiscWadAsset::funcs.unpack_dl = wrap_unpacker_func<MiscWadAsset>(unpack_dl_misc_wad);
	
	MiscWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<MiscWadAsset>(pack_rac2_misc_wad);
	MiscWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<MiscWadAsset>(pack_rac3_misc_wad);
	MiscWadAsset::funcs.pack_dl = wrap_wad_packer_func<MiscWadAsset>(pack_dl_misc_wad);
})

packed_struct(Rac2MiscWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontend_code;
	/* 0x28 */ SectorRange frontend_net_code;
	/* 0x30 */ SectorRange frontend;
	/* 0x38 */ SectorRange exit;
)

packed_struct(Rac3MiscWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontend_code;
	/* 0x28 */ SectorRange frontend_net_code;
	/* 0x30 */ SectorRange unused[2];
	/* 0x40 */ SectorRange boot;
)

packed_struct(DeadlockedMiscWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontend_code;
	/* 0x28 */ SectorRange unused[3];
	/* 0x40 */ SectorRange boot;
	/* 0x48 */ SectorRange gadget;
)

static void unpack_rac2_misc_wad(MiscWadAsset& dest, InputStream& src, Game game) {
	
}

static void pack_rac2_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game) {
	
}

static void unpack_rac3_misc_wad(MiscWadAsset& dest, InputStream& src, Game game) {
	
}

static void pack_rac3_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game) {
	
}

static void unpack_dl_misc_wad(MiscWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedMiscWadHeader>(0);
	
	unpack_asset(dest.debug_font<BinaryAsset>(), src, header.debug_font, game);
	unpack_compressed_asset(dest.irx().switch_files(), src, header.irx, game);
	unpack_asset(dest.save_game(), src, header.save_game, game);
	unpack_asset(dest.frontend_code(), src, header.frontend_code, game);
	unpack_asset(dest.boot().switch_files(), src, header.boot, game);
	unpack_asset(dest.gadget(), src, header.gadget, game);
}

static void pack_dl_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	DeadlockedMiscWadHeader header = {0};
	header.header_size = sizeof(DeadlockedMiscWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.debug_font = pack_asset_sa<SectorRange>(dest, src.get_debug_font(), game, base);
	header.irx = pack_compressed_asset_sa<SectorRange>(dest, src.get_irx(), game, base);
	header.save_game = pack_asset_sa<SectorRange>(dest, src.get_save_game(), game, base);
	header.frontend_code = pack_asset_sa<SectorRange>(dest, src.get_frontend_code(), game, base);
	header.boot = pack_asset_sa<SectorRange>(dest, src.get_boot(), game, base);
	header.gadget = pack_asset_sa<SectorRange>(dest, src.get_gadget(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
