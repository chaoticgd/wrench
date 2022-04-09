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

static Asset& unpack_irx_modules(Asset& parent, InputStream& src, SectorRange range);
static SectorRange pack_irx_wad(OutputStream& dest, IrxWadAsset& wad, Game game, s64 base);
static Asset& unpack_boot_wad(Asset& parent, InputStream& src, SectorRange range);
static SectorRange pack_boot_wad(OutputStream& dest, BootWadAsset& wad, Game game, s64 base);

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

MiscWadAsset& unpack_misc_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<MiscWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("misc/misc.asset");
	
	MiscWadAsset& wad = asset_file.root().child<MiscWadAsset>("misc");
	wad.set_debug_font(unpack_binary(wad, *file, header.debug_font, "debug_font", "debug_font.bin"));
	wad.set_irx(unpack_irx_modules(wad, *file, header.irx));
	wad.set_save_game(unpack_binary(wad, *file, header.save_game, "save_game", "save_game.bin"));
	wad.set_frontend_code(unpack_binary(wad, *file, header.frontend_code, "frontend_code", "frontend_code.bin"));
	wad.set_exit(unpack_binary(wad, *file, header.exit, "exit", "exit.bin"));
	wad.set_boot(unpack_boot_wad(wad, *file, header.bootwad));
	wad.set_gadget(unpack_binary(wad, *file, header.gadget, "gadget", "gadget.bin"));
	
	return wad;
}

void pack_misc_wad(OutputStream& dest, MiscWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	MiscWadHeaderDL header = {0};
	header.header_size = sizeof(MiscWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.debug_font = pack_asset_sa<SectorRange>(dest, wad.debug_font(), game, base);
	header.irx = pack_irx_wad(dest, static_cast<IrxWadAsset&>(*wad.irx()), game, base);
	header.save_game = pack_asset_sa<SectorRange>(dest, wad.save_game(), game, base);
	header.frontend_code = pack_asset_sa<SectorRange>(dest, wad.frontend_code(), game, base);
	header.exit = pack_asset_sa<SectorRange>(dest, wad.exit(), game, base);
	header.bootwad = pack_boot_wad(dest, static_cast<BootWadAsset&>(*wad.boot()), game, base);
	header.gadget = pack_asset_sa<SectorRange>(dest, wad.gadget(), game, base);
	
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

static Asset& unpack_irx_modules(Asset& parent, InputStream& src, SectorRange range) {
	src.seek(range.offset.bytes());
	std::vector<u8> compressed_bytes = src.read_multiple<u8>(range.size.bytes());
	std::vector<u8> bytes;
	decompress_wad(bytes, compressed_bytes);
	IrxHeader header = Buffer(bytes).read<IrxHeader>(0, "irx header");
	
	IrxWadAsset& irx = parent.asset_file("irx/irx.asset").child<IrxWadAsset>("irx");
	auto unpack_irx = [&](ByteRange range, const char* child) -> Asset& {
		return unpack_binary_from_memory(irx, bytes, range, child, ".irx");
	};
	
	irx.set_sio2man(unpack_irx(header.sio2man, "sio2man"));
	irx.set_mcman(unpack_irx(header.mcman, "mcman"));
	irx.set_mcserv(unpack_irx(header.mcserv, "mcserv"));
	irx.set_padman(unpack_irx(header.padman, "padman"));
	irx.set_mtapman(unpack_irx(header.mtapman, "mtapman"));
	irx.set_libsd(unpack_irx(header.libsd, "libsd"));
	irx.set_989snd(unpack_irx(header._989snd, "989snd"));
	irx.set_stash(unpack_irx(header.stash, "stash"));
	irx.set_inet(unpack_irx(header.inet, "inet"));
	irx.set_netcnf(unpack_irx(header.netcnf, "netcnf"));
	irx.set_inetctl(unpack_irx(header.inetctl, "inetctl"));
	irx.set_msifrpc(unpack_irx(header.msifrpc, "msifrpc"));
	irx.set_dev9(unpack_irx(header.dev9, "dev9"));
	irx.set_smap(unpack_irx(header.smap, "smap"));
	irx.set_libnetb(unpack_irx(header.libnetb, "libnetb"));
	irx.set_ppp(unpack_irx(header.ppp, "ppp"));
	irx.set_pppoe(unpack_irx(header.pppoe, "pppoe"));
	irx.set_usbd(unpack_irx(header.usbd, "usbd"));
	irx.set_lgaud(unpack_irx(header.lgaud, "lgaud"));
	irx.set_eznetcnf(unpack_irx(header.eznetcnf, "eznetcnf"));
	irx.set_eznetctl(unpack_irx(header.eznetctl, "eznetctl"));
	irx.set_lgkbm(unpack_irx(header.lgkbm, "lgkbm"));
	irx.set_streamer(unpack_irx(header.streamer, "streamer"));
	irx.set_astrm(unpack_irx(header.astrm, "astrm"));
	
	return irx;
}

static SectorRange pack_irx_wad(OutputStream& dest, IrxWadAsset& wad, Game game, s64 base) {
	std::vector<u8> bytes;
	MemoryOutputStream irxs(bytes);
	IrxHeader header;
	static_cast<OutputStream&>(irxs).write(header);
	header.sio2man = pack_asset_qa<ByteRange>(irxs, wad.sio2man(), game, base);
	header.mcman = pack_asset_qa<ByteRange>(irxs, wad.mcman(), game, base);
	header.mcserv = pack_asset_qa<ByteRange>(irxs, wad.mcserv(), game, base);
	header.padman = pack_asset_qa<ByteRange>(irxs, wad.padman(), game, base);
	header.mtapman = pack_asset_qa<ByteRange>(irxs, wad.mtapman(), game, base);
	header.libsd = pack_asset_qa<ByteRange>(irxs, wad.libsd(), game, base);
	header._989snd = pack_asset_qa<ByteRange>(irxs, wad._989snd(), game, base);
	header.stash = pack_asset_qa<ByteRange>(irxs, wad.stash(), game, base);
	header.inet = pack_asset_qa<ByteRange>(irxs, wad.inet(), game, base);
	header.netcnf = pack_asset_qa<ByteRange>(irxs, wad.netcnf(), game, base);
	header.inetctl = pack_asset_qa<ByteRange>(irxs, wad.inetctl(), game, base);
	header.msifrpc = pack_asset_qa<ByteRange>(irxs, wad.msifrpc(), game, base);
	header.dev9 = pack_asset_qa<ByteRange>(irxs, wad.dev9(), game, base);
	header.smap = pack_asset_qa<ByteRange>(irxs, wad.smap(), game, base);
	header.libnetb = pack_asset_qa<ByteRange>(irxs, wad.libnetb(), game, base);
	header.ppp = pack_asset_qa<ByteRange>(irxs, wad.ppp(), game, base);
	header.pppoe = pack_asset_qa<ByteRange>(irxs, wad.pppoe(), game, base);
	header.usbd = pack_asset_qa<ByteRange>(irxs, wad.usbd(), game, base);
	header.lgaud = pack_asset_qa<ByteRange>(irxs, wad.lgaud(), game, base);
	header.eznetcnf = pack_asset_qa<ByteRange>(irxs, wad.eznetcnf(), game, base);
	header.eznetctl = pack_asset_qa<ByteRange>(irxs, wad.eznetctl(), game, base);
	header.lgkbm = pack_asset_qa<ByteRange>(irxs, wad.lgkbm(), game, base);
	header.streamer = pack_asset_qa<ByteRange>(irxs, wad.streamer(), game, base);
	header.astrm = pack_asset_qa<ByteRange>(irxs, wad.astrm(), game, base);
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

static Asset& unpack_boot_wad(Asset& parent, InputStream& src, SectorRange range) {
	src.seek(range.offset.bytes());
	std::vector<u8> bytes = src.read_multiple<u8>(range.size.bytes());
	BootHeader header = Buffer(bytes).read<BootHeader>(0, "boot header");
	
	BootWadAsset& boot = parent.asset_file("boot/boot.asset").child<BootWadAsset>("boot");
	boot.set_english(unpack_compressed_binary_from_memory(boot, bytes, header.english, "english"));
	boot.set_french(unpack_compressed_binary_from_memory(boot, bytes, header.french, "french"));
	boot.set_german(unpack_compressed_binary_from_memory(boot, bytes, header.german, "german"));
	boot.set_spanish(unpack_compressed_binary_from_memory(boot, bytes, header.spanish, "spanish"));
	boot.set_italian(unpack_compressed_binary_from_memory(boot, bytes, header.italian, "italian"));
	boot.set_hud(unpack_compressed_binaries_from_memory(boot, bytes, ARRAY_PAIR(header.hudwad), "hud"));
	boot.set_boot_plates(unpack_compressed_binaries_from_memory(boot, bytes, ARRAY_PAIR(header.boot_plates), "boot_plates"));
	boot.set_sram(unpack_compressed_binary_from_memory(boot, bytes, header.sram, "sram"));
	
	return boot;
}

static SectorRange pack_boot_wad(OutputStream& dest, BootWadAsset& wad, Game game, s64 base) {
	dest.pad(SECTOR_SIZE, 0);
	s64 begin = dest.tell();
	BootHeader header;
	dest.write(header);
	header.english = compress_asset_qa<ByteRange>(dest, *wad.english(), game, base);
	header.french = compress_asset_qa<ByteRange>(dest, *wad.french(), game, base);
	header.german = compress_asset_qa<ByteRange>(dest, *wad.german(), game, base);
	header.spanish = compress_asset_qa<ByteRange>(dest, *wad.spanish(), game, base);
	header.italian = compress_asset_qa<ByteRange>(dest, *wad.italian(), game, base);
	compress_assets_qa(dest, ARRAY_PAIR(header.hudwad), wad.hud(), game, base, "hud");
	compress_assets_qa(dest, ARRAY_PAIR(header.boot_plates), wad.boot_plates(), game, base, "boot_plates");
	header.sram = compress_asset_qa<ByteRange>(dest, *wad.sram(), game, base);
	dest.write(begin, header);
	s64 end = dest.tell();
	return SectorRange::from_bytes(begin - base, end - begin);
}
