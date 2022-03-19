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

static Asset& unpack_irx_modules(Asset& parent, const FileHandle& src, SectorRange range);
static Asset& unpack_boot_wad(Asset& parent, const FileHandle& src, SectorRange range);

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

void unpack_misc_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<MiscWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("misc/misc.asset");
	
	MiscWadAsset& wad = asset_file.root().child<MiscWadAsset>("misc");
	wad.set_debug_font(unpack_binary(wad, file, header.debug_font, "debug_font", "debug_font.bin"));
	wad.set_irx(unpack_irx_modules(wad, file, header.irx));
	wad.set_save_game(unpack_binary(wad, file, header.save_game, "save_game", "save_game.bin"));
	wad.set_frontend_code(unpack_binary(wad, file, header.frontend_code, "frontend_code", "frontend_code.bin"));
	wad.set_exit(unpack_binary(wad, file, header.exit, "exit", "exit.bin"));
	wad.set_boot(unpack_boot_wad(wad, file, header.bootwad));
	wad.set_gadget(unpack_binary(wad, file, header.gadget, "gadget", "gadget.bin"));
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

static Asset& unpack_irx_modules(Asset& parent, const FileHandle& src, SectorRange range) {
	std::vector<u8> compressed_bytes = src.read_binary(range.bytes());
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

static Asset& unpack_boot_wad(Asset& parent, const FileHandle& src, SectorRange range) {
	std::vector<u8> bytes = src.read_binary(range.bytes());
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
