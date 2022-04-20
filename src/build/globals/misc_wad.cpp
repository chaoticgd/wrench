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
#include <build/asset_packer.h>

static void pack_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game);
static SectorRange pack_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game, s64 base);
static void unpack_irx_modules(IrxWadAsset& dest, InputStream& src, SectorRange range);
static SectorRange pack_boot_wad(OutputStream& dest, BootWadAsset& src, Game game, s64 base);
static void unpack_boot_wad(BootWadAsset& dest, InputStream& src);

on_load([]() {
	MiscWadAsset::pack_func = wrap_wad_packer_func<MiscWadAsset>(pack_misc_wad);
})

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
	/* 0x40 */ SectorRange boot;
	/* 0x48 */ SectorRange gadget;
)

static void pack_misc_wad(OutputStream& dest, std::vector<u8>* header_dest, MiscWadAsset& src, Game game) {
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
	header.boot = pack_boot_wad(dest, src.get_boot().as<BootWadAsset>(), game, base);
	header.gadget = pack_asset_sa<SectorRange>(dest, src.get_gadget(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_misc_wad(MiscWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<MiscWadHeaderDL>(src);
	
	unpack_binary(dest.debug_font<BinaryAsset>(), *file, header.debug_font, "debug_font.bin");
	unpack_irx_modules(dest.irx().switch_files(), *file, header.irx);
	unpack_binary(dest.save_game(), *file, header.save_game, "save_game.bin");
	unpack_binary(dest.frontend_code(), *file, header.frontend_code, "frontend_code.bin");
	unpack_binary(dest.exit(), *file, header.exit, "exit.bin");
	SubInputStream boot_stream(*file, header.boot.bytes().offset);
	unpack_boot_wad(dest.boot().switch_files(), boot_stream);
	unpack_binary(dest.gadget(), *file, header.gadget, "gadget.bin");
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

static SectorRange pack_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game, s64 base) {
	std::vector<u8> bytes;
	MemoryOutputStream irxs(bytes);
	IrxHeader header = {0};
	static_cast<OutputStream&>(irxs).write(header);
	header.sio2man = pack_asset_aligned<ByteRange>(irxs, src.get_sio2man(), game, 0, 0x40);
	header.mcman = pack_asset_aligned<ByteRange>(irxs, src.get_mcman(), game, 0, 0x40);
	header.mcserv = pack_asset_aligned<ByteRange>(irxs, src.get_mcserv(), game, 0, 0x40);
	header.padman = pack_asset_aligned<ByteRange>(irxs, src.get_padman(), game, 0, 0x40);
	header.mtapman = pack_asset_aligned<ByteRange>(irxs, src.get_mtapman(), game, 0, 0x40);
	header.libsd = pack_asset_aligned<ByteRange>(irxs, src.get_libsd(), game, 0, 0x40);
	header._989snd = pack_asset_aligned<ByteRange>(irxs, src.get_989snd(), game, 0, 0x40);
	header.stash = pack_asset_aligned<ByteRange>(irxs, src.get_stash(), game, 0, 0x40);
	header.inet = pack_asset_aligned<ByteRange>(irxs, src.get_inet(), game, 0, 0x40);
	header.netcnf = pack_asset_aligned<ByteRange>(irxs, src.get_netcnf(), game, 0, 0x40);
	header.inetctl = pack_asset_aligned<ByteRange>(irxs, src.get_inetctl(), game, 0, 0x40);
	header.msifrpc = pack_asset_aligned<ByteRange>(irxs, src.get_msifrpc(), game, 0, 0x40);
	header.dev9 = pack_asset_aligned<ByteRange>(irxs, src.get_dev9(), game, 0, 0x40);
	header.smap = pack_asset_aligned<ByteRange>(irxs, src.get_smap(), game, 0, 0x40);
	header.libnetb = pack_asset_aligned<ByteRange>(irxs, src.get_libnetb(), game, 0, 0x40);
	header.ppp = pack_asset_aligned<ByteRange>(irxs, src.get_ppp(), game, 0, 0x40);
	header.pppoe = pack_asset_aligned<ByteRange>(irxs, src.get_pppoe(), game, 0, 0x40);
	header.usbd = pack_asset_aligned<ByteRange>(irxs, src.get_usbd(), game, 0, 0x40);
	header.lgaud = pack_asset_aligned<ByteRange>(irxs, src.get_lgaud(), game, 0, 0x40);
	header.eznetcnf = pack_asset_aligned<ByteRange>(irxs, src.get_eznetcnf(), game, 0, 0x40);
	header.eznetctl = pack_asset_aligned<ByteRange>(irxs, src.get_eznetctl(), game, 0, 0x40);
	header.lgkbm = pack_asset_aligned<ByteRange>(irxs, src.get_lgkbm(), game, 0, 0x40);
	header.streamer = pack_asset_aligned<ByteRange>(irxs, src.get_streamer(), game, 0, 0x40);
	header.astrm = pack_asset_aligned<ByteRange>(irxs, src.get_astrm(), game, 0, 0x40);
	static_cast<OutputStream&>(irxs).write(0, header);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, 8);
	
	dest.pad(SECTOR_SIZE, 0);
	s64 begin = dest.tell();
	dest.write(compressed_bytes.data(), compressed_bytes.size());
	s64 end = dest.tell();
	return SectorRange::from_bytes(begin - base, end - begin);
}

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

static SectorRange pack_boot_wad(OutputStream& dest, BootWadAsset& src, Game game, s64 base) {
	dest.pad(SECTOR_SIZE, 0);
	s64 begin = dest.tell();
	BootHeader header;
	dest.write(header);
	header.english = pack_compressed_asset_aligned<ByteRange>(dest, src.get_english(), game, begin, 0x40);
	header.french = pack_compressed_asset_aligned<ByteRange>(dest, src.get_french(), game, begin, 0x40);
	header.german = pack_compressed_asset_aligned<ByteRange>(dest, src.get_german(), game, begin, 0x40);
	header.spanish = pack_compressed_asset_aligned<ByteRange>(dest, src.get_spanish(), game, begin, 0x40);
	header.italian = pack_compressed_asset_aligned<ByteRange>(dest, src.get_italian(), game, begin, 0x40);
	if(src.hud().has_child(0)) {
		header.hudwad[0] = pack_asset_aligned<ByteRange>(dest, src.get_hud().get_child(0), game, begin, 0x40);
	}
	if(src.hud().has_child(1)) {
		header.hudwad[1] = pack_compressed_asset_aligned<ByteRange>(dest, src.get_hud().get_child(1), game, begin, 0x40);
	}
	if(src.hud().has_child(2)) {
		header.hudwad[2] = pack_compressed_asset_aligned<ByteRange>(dest, src.get_hud().get_child(2), game, begin, 0x40);
	}
	if(src.hud().has_child(3)) {
		header.hudwad[3] = pack_compressed_asset_aligned<ByteRange>(dest, src.get_hud().get_child(3), game, begin, 0x40);
	}
	if(src.hud().has_child(4)) {
		header.hudwad[4] = pack_compressed_asset_aligned<ByteRange>(dest, src.get_hud().get_child(4), game, begin, 0x40);
	}
	if(src.hud().has_child(5)) {
		header.hudwad[5] = pack_compressed_asset_aligned<ByteRange>(dest, src.get_hud().get_child(5), game, begin, 0x40);
	}
	pack_compressed_assets_aligned(dest, ARRAY_PAIR(header.boot_plates), src.get_boot_plates(), game, begin, 0x40);
	header.sram = pack_compressed_asset_aligned<ByteRange>(dest, src.get_sram(), game, begin, 0x40);
	dest.write(begin, header);
	s64 end = dest.tell();
	return SectorRange::from_bytes(begin - base, end - begin);
}

static void unpack_boot_wad(BootWadAsset& dest, InputStream& src) {
	BootHeader header = src.read<BootHeader>(0);
	
	unpack_compressed_binary(dest.english(), src, header.english, "english.bin");
	unpack_compressed_binary(dest.french(), src, header.french, "french.bin");
	unpack_compressed_binary(dest.german(), src, header.german, "german.bin");
	unpack_compressed_binary(dest.spanish(), src, header.spanish, "spanish.bin");
	unpack_compressed_binary(dest.italian(), src, header.italian, "italian.bin");
	unpack_binary(dest.hud().child<BinaryAsset>(0), src, header.hudwad[0], "hud/0.bin");
	unpack_compressed_binary(dest.hud().child<BinaryAsset>(1), src, header.hudwad[1], "hud/1.bin");
	unpack_compressed_binary(dest.hud().child<BinaryAsset>(2), src, header.hudwad[2], "hud/2.bin");
	unpack_compressed_binary(dest.hud().child<BinaryAsset>(3), src, header.hudwad[3], "hud/3.bin");
	unpack_compressed_binary(dest.hud().child<BinaryAsset>(4), src, header.hudwad[4], "hud/4.bin");
	unpack_compressed_binary(dest.hud().child<BinaryAsset>(5), src, header.hudwad[5], "hud/5.bin");
	unpack_compressed_binaries(dest.boot_plates().switch_files(), src, ARRAY_PAIR(header.boot_plates), ".bin");
	unpack_compressed_binary(dest.sram(), src, header.sram, "sram.bin");
}
