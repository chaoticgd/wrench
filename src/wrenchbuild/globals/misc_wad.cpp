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
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(GcMiscWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontbin;
	/* 0x28 */ SectorRange frontbin_net;
	/* 0x30 */ SectorRange frontend;
	/* 0x38 */ SectorRange exit;
)

packed_struct(UyaMiscWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontbin;
	/* 0x28 */ SectorRange frontbin_net;
	/* 0x30 */ SectorRange unused[2];
	/* 0x40 */ SectorRange boot;
)

packed_struct(DlMiscWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontbin;
	/* 0x28 */ SectorRange unused[3];
	/* 0x40 */ SectorRange boot;
	/* 0x48 */ SectorRange gadget;
)

static void unpack_gc_misc_wad(MiscWadAsset& dest, const GcMiscWadHeader& header, InputStream& src, Game game);
static void pack_gc_misc_wad(OutputStream& dest, GcMiscWadHeader& header, const MiscWadAsset& src, Game game);
static void unpack_uya_misc_wad(MiscWadAsset& dest, const UyaMiscWadHeader& header, InputStream& src, Game game);
static void pack_uya_misc_wad(OutputStream& dest, UyaMiscWadHeader& header, const MiscWadAsset& src, Game game);
static void unpack_dl_misc_wad(MiscWadAsset& dest, const DlMiscWadHeader& header, InputStream& src, Game game);
static void pack_dl_misc_wad(OutputStream& dest, DlMiscWadHeader& header, const MiscWadAsset& src, Game game);

on_load(Misc, []() {
	MiscWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<MiscWadAsset, GcMiscWadHeader>(unpack_gc_misc_wad);
	MiscWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<MiscWadAsset, UyaMiscWadHeader>(unpack_uya_misc_wad);
	MiscWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<MiscWadAsset, DlMiscWadHeader>(unpack_dl_misc_wad);
	
	MiscWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<MiscWadAsset, GcMiscWadHeader>(pack_gc_misc_wad);
	MiscWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<MiscWadAsset, UyaMiscWadHeader>(pack_uya_misc_wad);
	MiscWadAsset::funcs.pack_dl = wrap_wad_packer_func<MiscWadAsset, DlMiscWadHeader>(pack_dl_misc_wad);
})

static void unpack_gc_misc_wad(MiscWadAsset& dest, const GcMiscWadHeader& header, InputStream& src, Game game) {
	unpack_asset(dest.debug_font<TextureAsset>(), src, header.debug_font, game, FMT_TEXTURE_PIF8);
	unpack_compressed_asset(dest.irx(SWITCH_FILES), src, header.irx, game);
	unpack_asset(dest.save_game(), src, header.save_game, game);
	unpack_asset(dest.frontbin(), src, header.frontbin, game);
	unpack_compressed_asset(dest.frontbin_net(), src, header.frontbin_net, game);
	unpack_asset(dest.frontend(), src, header.frontend, game);
	unpack_asset(dest.exit(), src, header.exit, game);
}

static void pack_gc_misc_wad(OutputStream& dest, GcMiscWadHeader& header, const MiscWadAsset& src, Game game) {
	header.debug_font = pack_asset_sa<SectorRange>(dest, src.get_debug_font(), game, FMT_TEXTURE_PIF8);
	header.irx = pack_compressed_asset_sa<SectorRange>(dest, src.get_irx(), game, "irx");
	header.save_game = pack_asset_sa<SectorRange>(dest, src.get_save_game(), game);
	header.frontbin = pack_asset_sa<SectorRange>(dest, src.get_frontbin(), game);
	header.frontbin_net = pack_compressed_asset_sa<SectorRange>(dest, src.get_frontbin_net(), game, "frontbin_net");
	header.frontend = pack_asset_sa<SectorRange>(dest, src.get_frontend(), game);
	header.exit = pack_asset_sa<SectorRange>(dest, src.get_exit(), game);
}

static void unpack_uya_misc_wad(MiscWadAsset& dest, const UyaMiscWadHeader& header, InputStream& src, Game game) {
	unpack_asset(dest.debug_font<TextureAsset>(), src, header.debug_font, game, FMT_TEXTURE_PIF8);
	unpack_compressed_asset(dest.irx(SWITCH_FILES), src, header.irx, game);
	unpack_asset(dest.save_game(), src, header.save_game, game);
	unpack_asset(dest.frontbin(), src, header.frontbin, game);
	unpack_compressed_asset(dest.frontbin_net(), src, header.frontbin_net, game);
	unpack_asset(dest.boot(SWITCH_FILES), src, header.boot, game);
}

static void pack_uya_misc_wad(OutputStream& dest, UyaMiscWadHeader& header, const MiscWadAsset& src, Game game) {
	header.debug_font = pack_asset_sa<SectorRange>(dest, src.get_debug_font(), game, FMT_TEXTURE_PIF8);
	header.irx = pack_compressed_asset_sa<SectorRange>(dest, src.get_irx(), game, "irx");
	header.save_game = pack_asset_sa<SectorRange>(dest, src.get_save_game(), game);
	header.frontbin = pack_asset_sa<SectorRange>(dest, src.get_frontbin(), game);
	header.frontbin_net = pack_compressed_asset_sa<SectorRange>(dest, src.get_frontbin_net(), game, "frontbin_net");
	header.boot = pack_asset_sa<SectorRange>(dest, src.get_boot(), game);
}

static void unpack_dl_misc_wad(MiscWadAsset& dest, const DlMiscWadHeader& header, InputStream& src, Game game) {
	unpack_asset(dest.debug_font<TextureAsset>(), src, header.debug_font, game, FMT_TEXTURE_PIF8);
	unpack_compressed_asset(dest.irx(SWITCH_FILES), src, header.irx, game);
	unpack_asset(dest.save_game(), src, header.save_game, game);
	unpack_asset(dest.frontbin(), src, header.frontbin, game);
	unpack_asset(dest.boot(SWITCH_FILES), src, header.boot, game);
	unpack_asset(dest.gadget(), src, header.gadget, game);
}

static void pack_dl_misc_wad(OutputStream& dest, DlMiscWadHeader& header, const MiscWadAsset& src, Game game) {
	header.debug_font = pack_asset_sa<SectorRange>(dest, src.get_debug_font(), game, FMT_TEXTURE_PIF8);
	header.irx = pack_compressed_asset_sa<SectorRange>(dest, src.get_irx(), game, "irx");
	header.save_game = pack_asset_sa<SectorRange>(dest, src.get_save_game(), game);
	header.frontbin = pack_asset_sa<SectorRange>(dest, src.get_frontbin(), game);
	header.boot = pack_asset_sa<SectorRange>(dest, src.get_boot(), game);
	header.gadget = pack_asset_sa<SectorRange>(dest, src.get_gadget(), game);
}
