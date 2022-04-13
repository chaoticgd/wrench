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

#include "misc_wad.h"

#include <engine/compression.h>
#include <spanner/asset_packer.h>

static void unpack_irx_modules(IrxWadAsset& dest, InputStream& src, SectorRange range);
static SectorRange pack_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game, s64 base);
static void unpack_boot_wad(BootWadAsset& dest, InputStream& src, SectorRange range);
static SectorRange pack_boot_wad(OutputStream& dest, BootWadAsset& src, Game game, s64 base);

packed_struct(MiscWadHeaderDL,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange debug_font;
	/* 0x10 */ SectorRange irx;
	/* 0x18 */ SectorRange save_game;
	/* 0x20 */ SectorRange frontend_code;
	/* 0x28 */ SectorRange frontbin_net;
	/* 0x30 */ SectorRange frontend;
	/* 0x38 */ SectorRange exit;
	/* 0x40 */ SectorRange bootwad;
	/* 0x48 */ SectorRange gadget;
)

void unpack_misc_wad(MiscWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<MiscWadHeaderDL>(src);
	
	unpack_binary(dest.debug_font<BinaryAsset>(), *file, header.debug_font, "debug_font.bin");
	unpack_irx_modules(dest.irx(), *file, header.irx);
	unpack_binary(dest.save_game(), *file, header.save_game, "save_game.bin");
	unpack_binary(dest.frontend_code(), *file, header.frontend_code, "frontend_code.bin");
	unpack_binary(dest.exit(), *file, header.exit, "exit.bin");
	unpack_boot_wad(dest.boot(), *file, header.bootwad);
	unpack_binary(dest.gadget(), *file, header.gadget, "gadget.bin");
}

void pack_misc_wad(OutputStream& dest, MiscWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	MiscWadHeaderDL header = {0};
	header.header_size = sizeof(MiscWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.debug_font = pack_asset_sa<SectorRange>(dest, src.get_debug_font(), game, base);
	header.irx = pack_irx_wad(dest, src.get_irx().as<IrxWadAsset>(), game, base);
	header.save_game = pack_asset_sa<SectorRange>(dest, src.get_save_game(), game, base);
	header.frontend_code = pack_asset_sa<SectorRange>(dest, src.get_frontend_code(), game, base);
	header.exit = pack_asset_sa<SectorRange>(dest, src.get_exit(), game, base);
	header.bootwad = pack_boot_wad(dest, src.get_boot().as<BootWadAsset>(), game, base);
	header.gadget = pack_asset_sa<SectorRange>(dest, src.get_gadget(), game, base);
	
	dest.write(base, header);
}

packed_struct(IrxHeader,
	/* 0x00 */ s32 iopmem;
	/* 0x04 */ s32 pad;
	/* 0x08 */ ByteRange sio2man;
	/* 0x10 */ ByteRange mcman;
	/* 0x18 */ ByteRange mcserv;
	/* 0x20 */ ByteRange padman;
	/* 0x28 */ ByteRange mtapman;
	/* 0x30 */ ByteRange libsd;
	/* 0x38 */ ByteRange _989snd;
	/* 0x40 */ ByteRange stash;
	/* 0x48 */ ByteRange inet;
	/* 0x50 */ ByteRange netcnf;
	/* 0x58 */ ByteRange inetctl;
	/* 0x60 */ ByteRange msifrpc;
	/* 0x68 */ ByteRange dev9;
	/* 0x70 */ ByteRange smap;
	/* 0x78 */ ByteRange libnetb;
	/* 0x80 */ ByteRange ppp;
	/* 0x88 */ ByteRange pppoe;
	/* 0x90 */ ByteRange usbd;
	/* 0x98 */ ByteRange lgaud;
	/* 0xa0 */ ByteRange eznetcnf;
	/* 0xa8 */ ByteRange eznetctl;
	/* 0xb0 */ ByteRange lgkbm;
	/* 0xb8 */ ByteRange streamer;
	/* 0xc0 */ ByteRange astrm;
)

static void unpack_irx_modules(IrxWadAsset& dest, InputStream& src, SectorRange range) {
	src.seek(range.offset.bytes());
	std::vector<u8> compressed_bytes = src.read_multiple<u8>(range.size.bytes());
	std::vector<u8> bytes;
	decompress_wad(bytes, compressed_bytes);
	IrxHeader header = Buffer(bytes).read<IrxHeader>(0, "irx header");
	MemoryInputStream stream(bytes);
	
	auto unpack_irx = [&](BinaryAsset& irx, ByteRange range, const char* child) {
		unpack_binary(irx, stream, range, std::string(child) + ".irx");
	};
	
	unpack_irx(dest.sio2man(), header.sio2man, "sio2man");
	unpack_irx(dest.mcman(), header.mcman, "mcman");
	unpack_irx(dest.mcserv(), header.mcserv, "mcserv");
	unpack_irx(dest.padman(), header.padman, "padman");
	unpack_irx(dest.mtapman(), header.mtapman, "mtapman");
	unpack_irx(dest.libsd(), header.libsd, "libsd");
	unpack_irx(dest._989snd(), header._989snd, "989snd");
	unpack_irx(dest.stash(), header.stash, "stash");
	unpack_irx(dest.inet(), header.inet, "inet");
	unpack_irx(dest.netcnf(), header.netcnf, "netcnf");
	unpack_irx(dest.inetctl(), header.inetctl, "inetctl");
	unpack_irx(dest.msifrpc(), header.msifrpc, "msifrpc");
	unpack_irx(dest.dev9(), header.dev9, "dev9");
	unpack_irx(dest.smap(), header.smap, "smap");
	unpack_irx(dest.libnetb(), header.libnetb, "libnetb");
	unpack_irx(dest.ppp(), header.ppp, "ppp");
	unpack_irx(dest.pppoe(), header.pppoe, "pppoe");
	unpack_irx(dest.usbd(), header.usbd, "usbd");
	unpack_irx(dest.lgaud(), header.lgaud, "lgaud");
	unpack_irx(dest.eznetcnf(), header.eznetcnf, "eznetcnf");
	unpack_irx(dest.eznetctl(), header.eznetctl, "eznetctl");
	unpack_irx(dest.lgkbm(), header.lgkbm, "lgkbm");
	unpack_irx(dest.streamer(), header.streamer, "streamer");
	unpack_irx(dest.astrm(), header.astrm, "astrm");
}

static SectorRange pack_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game, s64 base) {
	std::vector<u8> bytes;
	MemoryOutputStream irxs(bytes);
	IrxHeader header = {0};
	static_cast<OutputStream&>(irxs).write(header);
	header.sio2man = pack_asset_aligned<ByteRange>(irxs, src.get_sio2man(), game, base, 0x40);
	header.mcman = pack_asset_aligned<ByteRange>(irxs, src.get_mcman(), game, base, 0x40);
	header.mcserv = pack_asset_aligned<ByteRange>(irxs, src.get_mcserv(), game, base, 0x40);
	header.padman = pack_asset_aligned<ByteRange>(irxs, src.get_padman(), game, base, 0x40);
	header.mtapman = pack_asset_aligned<ByteRange>(irxs, src.get_mtapman(), game, base, 0x40);
	header.libsd = pack_asset_aligned<ByteRange>(irxs, src.get_libsd(), game, base, 0x40);
	header._989snd = pack_asset_aligned<ByteRange>(irxs, src.get_989snd(), game, base, 0x40);
	header.stash = pack_asset_aligned<ByteRange>(irxs, src.get_stash(), game, base, 0x40);
	header.inet = pack_asset_aligned<ByteRange>(irxs, src.get_inet(), game, base, 0x40);
	header.netcnf = pack_asset_aligned<ByteRange>(irxs, src.get_netcnf(), game, base, 0x40);
	header.inetctl = pack_asset_aligned<ByteRange>(irxs, src.get_inetctl(), game, base, 0x40);
	header.msifrpc = pack_asset_aligned<ByteRange>(irxs, src.get_msifrpc(), game, base, 0x40);
	header.dev9 = pack_asset_aligned<ByteRange>(irxs, src.get_dev9(), game, base, 0x40);
	header.smap = pack_asset_aligned<ByteRange>(irxs, src.get_smap(), game, base, 0x40);
	header.libnetb = pack_asset_aligned<ByteRange>(irxs, src.get_libnetb(), game, base, 0x40);
	header.ppp = pack_asset_aligned<ByteRange>(irxs, src.get_ppp(), game, base, 0x40);
	header.pppoe = pack_asset_aligned<ByteRange>(irxs, src.get_pppoe(), game, base, 0x40);
	header.usbd = pack_asset_aligned<ByteRange>(irxs, src.get_usbd(), game, base, 0x40);
	header.lgaud = pack_asset_aligned<ByteRange>(irxs, src.get_lgaud(), game, base, 0x40);
	header.eznetcnf = pack_asset_aligned<ByteRange>(irxs, src.get_eznetcnf(), game, base, 0x40);
	header.eznetctl = pack_asset_aligned<ByteRange>(irxs, src.get_eznetctl(), game, base, 0x40);
	header.lgkbm = pack_asset_aligned<ByteRange>(irxs, src.get_lgkbm(), game, base, 0x40);
	header.streamer = pack_asset_aligned<ByteRange>(irxs, src.get_streamer(), game, base, 0x40);
	header.astrm = pack_asset_aligned<ByteRange>(irxs, src.get_astrm(), game, base, 0x40);
	static_cast<OutputStream&>(irxs).write(0, header);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, 8);
	
	dest.pad(SECTOR_SIZE, 0);
	s64 begin = dest.tell();
	dest.write(compressed_bytes.data(), compressed_bytes.size());
	s64 end = dest.tell();
	return SectorRange::from_bytes(begin - base, end - begin);
}

packed_struct(BootHeader,
	/* 0x00 */ ByteRange english;
	/* 0x08 */ ByteRange french;
	/* 0x10 */ ByteRange german;
	/* 0x18 */ ByteRange spanish;
	/* 0x20 */ ByteRange italian;
	/* 0x28 */ ByteRange hudwad[6];
	/* 0x58 */ ByteRange boot_plates[4];
	/* 0x78 */ ByteRange sram;
)

static void unpack_boot_wad(BootWadAsset& dest, InputStream& src, SectorRange range) {
	//src.seek(range.offset.bytes());
	//std::vector<u8> bytes = src.read_multiple<u8>(range.size.bytes());
	//BootHeader header = Buffer(bytes).read<BootHeader>(0, "boot header");
	//
	//BootWadAsset& boot = parent.asset_file("boot/boot.asset").child<BootWadAsset>("boot");
	//boot.set_english(unpack_compressed_binary_from_memory(boot, bytes, header.english, "english"));
	//boot.set_french(unpack_compressed_binary_from_memory(boot, bytes, header.french, "french"));
	//boot.set_german(unpack_compressed_binary_from_memory(boot, bytes, header.german, "german"));
	//boot.set_spanish(unpack_compressed_binary_from_memory(boot, bytes, header.spanish, "spanish"));
	//boot.set_italian(unpack_compressed_binary_from_memory(boot, bytes, header.italian, "italian"));
	//CollectionAsset& hud = boot.asset_file("hud/hud.asset").child<CollectionAsset>("hud");
	//std::vector<Asset*> hud_assets;
	//hud_assets.push_back(&unpack_binary_from_memory(hud, bytes, header.hudwad[0], "0"));
	//hud_assets.push_back(&unpack_compressed_binary_from_memory(hud, bytes, header.hudwad[1], "1"));
	//hud_assets.push_back(&unpack_compressed_binary_from_memory(hud, bytes, header.hudwad[2], "2"));
	//hud_assets.push_back(&unpack_compressed_binary_from_memory(hud, bytes, header.hudwad[3], "3"));
	//hud_assets.push_back(&unpack_compressed_binary_from_memory(hud, bytes, header.hudwad[4], "4"));
	//hud_assets.push_back(&unpack_compressed_binary_from_memory(hud, bytes, header.hudwad[5], "5"));
	//boot.set_hud(hud_assets);
	//boot.set_boot_plates(unpack_compressed_binaries_from_memory(boot, bytes, ARRAY_PAIR(header.boot_plates), "boot_plates"));
	//boot.set_sram(unpack_compressed_binary_from_memory(boot, bytes, header.sram, "sram"));
	//
	//return boot;
}

static SectorRange pack_boot_wad(OutputStream& dest, BootWadAsset& src, Game game, s64 base) {
	//dest.pad(SECTOR_SIZE, 0);
	//s64 begin = dest.tell();
	//BootHeader header;
	//dest.write(header);
	//header.english = compress_asset_aligned<ByteRange>(dest, src.english(), game, begin, 0x40);
	//header.french = compress_asset_aligned<ByteRange>(dest, src.french(), game, begin, 0x40);
	//header.german = compress_asset_aligned<ByteRange>(dest, src.german(), game, begin, 0x40);
	//header.spanish = compress_asset_aligned<ByteRange>(dest, src.spanish(), game, begin, 0x40);
	//header.italian = compress_asset_aligned<ByteRange>(dest, src.italian(), game, begin, 0x40);
	//std::vector<Asset*> hud = src.hud();
	//verify(hud.size() == 6, "Wrong number of boot wad hud assets.");
	//header.hudwad[0] = pack_asset_aligned<ByteRange>(dest, wad.hud()[0], game, begin, 0x40);
	//header.hudwad[1] = compress_asset_aligned<ByteRange>(dest, *wad.hud()[1], game, begin, 0x40);
	//header.hudwad[2] = compress_asset_aligned<ByteRange>(dest, *wad.hud()[2], game, begin, 0x40);
	//header.hudwad[3] = compress_asset_aligned<ByteRange>(dest, *wad.hud()[3], game, begin, 0x40);
	//header.hudwad[4] = compress_asset_aligned<ByteRange>(dest, *wad.hud()[4], game, begin, 0x40);
	//header.hudwad[5] = compress_asset_aligned<ByteRange>(dest, *wad.hud()[5], game, begin, 0x40);
	//compress_assets_aligned(dest, ARRAY_PAIR(header.boot_plates), wad.boot_plates(), game, begin, "boot_plates", 0x40);
	//header.sram = compress_asset_aligned<ByteRange>(dest, *wad.sram(), game, begin, 0x40);
	//dest.write(begin, header);
	//s64 end = dest.tell();
	//return SectorRange::from_bytes(begin - base, end - begin);
	return SectorRange::from_bytes(0, 0);
}
