/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#include "global_wads.h"

#include <lz/compression.h>
#include <assetmgr/asset_types.h>

static Asset& unpack_irx_modules(Asset& parent, const FileHandle& src, SectorRange range);
static Asset& unpack_boot_wad(Asset& parent, const FileHandle& src, SectorRange range);

static Asset& unpack_binary_lump(Asset& parent, const FileHandle& src, SectorRange range, const char* child, fs::path path) {
	std::vector<u8> bytes = src.read_binary(range.bytes());
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(path, bytes));
	return binary;
}

static Asset& unpack_binary(Asset& parent, Buffer src, ByteRange range, const char* child, const char* extension = ".bin") {
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(std::string(child) + extension, src.subbuf(range.offset, range.size)));
	return binary;
}

static Asset& unpack_compressed_binary(Asset& parent, Buffer src, ByteRange range, const char* child, const char* extension = ".bin") {
	std::vector<u8> bytes;
	Buffer compressed_bytes = src.subbuf(range.offset, range.size);
	decompress_wad(bytes, WadBuffer{compressed_bytes.lo, compressed_bytes.hi});
	
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(std::string(child) + extension, bytes));
	return binary;
}

static std::vector<Asset*> unpack_compressed_binaries(Asset& parent, Buffer src, ByteRange* ranges, s32 count, const char* child) {
	fs::path path = fs::path(child)/child;
	CollectionAsset& collection = parent.asset_file(path).child<CollectionAsset>(child);
	
	std::vector<Asset*> assets;
	for(s32 i = 0; i < count; i++) {
		std::string name = std::to_string(i);
		assets.emplace_back(&unpack_compressed_binary(collection, src, ranges[i], name.c_str()));
	}
	
	return assets;
}

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

void unpack_misc_wad(AssetPack& pack, const FileHandle& src, Buffer header_bytes) {
	MiscWadHeaderDL header = header_bytes.read<MiscWadHeaderDL>(0, "file header");
	
	AssetFile& asset_file = pack.asset_file("misc/misc.asset");
	
	MiscWadAsset& misc_wad = asset_file.root().child<MiscWadAsset>("misc");
	misc_wad.set_debug_font(unpack_binary_lump(misc_wad, src, header.debug_font, "debug_font", "debug_font.bin"));
	misc_wad.set_irx(unpack_irx_modules(misc_wad, src, header.irx));
	misc_wad.set_save_game(unpack_binary_lump(misc_wad, src, header.save_game, "save_game", "save_game.bin"));
	misc_wad.set_frontend_code(unpack_binary_lump(misc_wad, src, header.frontend_code, "frontend_code", "frontend_code.bin"));
	misc_wad.set_exit(unpack_binary_lump(misc_wad, src, header.exit, "exit", "exit.bin"));
	misc_wad.set_boot(unpack_boot_wad(misc_wad, src, header.bootwad));
	misc_wad.set_gadget(unpack_binary_lump(misc_wad, src, header.gadget, "gadget", "gadget.bin"));
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
		return unpack_binary(irx, bytes, range, child, ".irx");
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
	boot.set_english(unpack_compressed_binary(boot, bytes, header.english, "english"));
	boot.set_french(unpack_compressed_binary(boot, bytes, header.french, "french"));
	boot.set_german(unpack_compressed_binary(boot, bytes, header.german, "german"));
	boot.set_spanish(unpack_compressed_binary(boot, bytes, header.spanish, "spanish"));
	boot.set_italian(unpack_compressed_binary(boot, bytes, header.italian, "italian"));
	boot.set_hud(unpack_compressed_binaries(boot, bytes, header.hudwad, 6, "hud"));
	boot.set_boot_plates(unpack_compressed_binaries(boot, bytes, header.boot_plates, 4, "boot_plates"));
	boot.set_sram(unpack_compressed_binary(boot, bytes, header.sram, "sram"));
	
	return boot;
}
