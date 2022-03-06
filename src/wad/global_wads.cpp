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

static Asset& unpack_binary_lump(Asset& parent, FILE* src, SectorRange range, const char* child, fs::path path) {
	std::vector<u8> bytes = read_file(src, range.offset.bytes(), range.size.bytes());
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(path, bytes));
	return binary;
}

packed_struct(IrxHeader,
	s32 iopmem;
	s32 pad;
	ByteRange sio2man;
	ByteRange mcman;
	ByteRange mcserv;
	ByteRange padman;
	ByteRange mtapman;
	ByteRange libsd;
	ByteRange _989snd;
	ByteRange stash;
	ByteRange inet;
	ByteRange netcnf;
	ByteRange inetctl;
	ByteRange msifrpc;
	ByteRange dev9;
	ByteRange smap;
	ByteRange libnetb;
	ByteRange ppp;
	ByteRange pppoe;
	ByteRange usbd;
	ByteRange lgaud;
	ByteRange eznetcnf;
	ByteRange eznetctl;
	ByteRange lgkbm;
	ByteRange streamer;
	ByteRange astrm;
)

static Asset& unpack_irx_module(Asset& parent, Buffer src, ByteRange range, const char* child) {
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(child + std::string(".irx"), src.subbuf(range.offset, range.size)));
	return binary;
}

static Asset& unpack_irx_modules(Asset& parent, FILE* src, SectorRange range) {
	std::vector<u8> compressed_bytes = read_file(src, range.offset.bytes(), range.size.bytes());
	std::vector<u8> bytes;
	decompress_wad(bytes, compressed_bytes);
	IrxHeader header = Buffer(bytes).read<IrxHeader>(0, "irx header");
	
	IrxModulesAsset& irx = parent.child<IrxModulesAsset>("irx", "irx/irx.asset");
	irx.set_sio2man(unpack_irx_module(irx, bytes, header.sio2man, "sio2man"));
	irx.set_mcman(unpack_irx_module(irx, bytes, header.mcman, "mcman"));
	irx.set_mcserv(unpack_irx_module(irx, bytes, header.mcserv, "mcserv"));
	irx.set_padman(unpack_irx_module(irx, bytes, header.padman, "padman"));
	irx.set_mtapman(unpack_irx_module(irx, bytes, header.mtapman, "mtapman"));
	irx.set_libsd(unpack_irx_module(irx, bytes, header.libsd, "libsd"));
	irx.set_989snd(unpack_irx_module(irx, bytes, header._989snd, "989snd"));
	irx.set_stash(unpack_irx_module(irx, bytes, header.stash, "stash"));
	irx.set_inet(unpack_irx_module(irx, bytes, header.inet, "inet"));
	irx.set_netcnf(unpack_irx_module(irx, bytes, header.netcnf, "netcnf"));
	irx.set_inetctl(unpack_irx_module(irx, bytes, header.inetctl, "inetctl"));
	irx.set_msifrpc(unpack_irx_module(irx, bytes, header.msifrpc, "msifrpc"));
	irx.set_dev9(unpack_irx_module(irx, bytes, header.dev9, "dev9"));
	irx.set_smap(unpack_irx_module(irx, bytes, header.smap, "smap"));
	irx.set_libnetb(unpack_irx_module(irx, bytes, header.libnetb, "libnetb"));
	irx.set_ppp(unpack_irx_module(irx, bytes, header.ppp, "ppp"));
	irx.set_pppoe(unpack_irx_module(irx, bytes, header.pppoe, "pppoe"));
	irx.set_usbd(unpack_irx_module(irx, bytes, header.usbd, "usbd"));
	irx.set_lgaud(unpack_irx_module(irx, bytes, header.lgaud, "lgaud"));
	irx.set_eznetcnf(unpack_irx_module(irx, bytes, header.eznetcnf, "eznetcnf"));
	irx.set_eznetctl(unpack_irx_module(irx, bytes, header.eznetctl, "eznetctl"));
	irx.set_lgkbm(unpack_irx_module(irx, bytes, header.lgkbm, "lgkbm"));
	irx.set_streamer(unpack_irx_module(irx, bytes, header.streamer, "streamer"));
	irx.set_astrm(unpack_irx_module(irx, bytes, header.astrm, "astrm"));
	
	return irx;
}

packed_struct(MiscWadHeaderDL,
	s32 header_size;
	Sector32 sector;
	SectorRange debug_font;
	SectorRange irx;
	SectorRange save_game;
	SectorRange frontend_code;
	SectorRange frontbin_net;
	SectorRange frontend;
	SectorRange exit;
	SectorRange bootwad;
	SectorRange gadget;
)

void unpack_misc_wad(AssetPack& pack, FILE* src, Buffer header_bytes) {
	MiscWadHeaderDL header = header_bytes.read<MiscWadHeaderDL>(0, "file header");
	
	AssetFile& asset_file = pack.asset_file("misc/misc.asset");
	MiscWadAsset& misc_wad = asset_file.root().child<MiscWadAsset>("misc");
	
	misc_wad.set_debug_font(unpack_binary_lump(misc_wad, src, header.debug_font, "debug_font", "debug_font.bin"));
	misc_wad.set_irx(unpack_irx_modules(misc_wad, src, header.irx));
	misc_wad.set_save_game(unpack_binary_lump(misc_wad, src, header.save_game, "save_game", "save_game.bin"));
	misc_wad.set_frontend_code(unpack_binary_lump(misc_wad, src, header.frontend_code, "frontend_code", "frontend_code.bin"));
	misc_wad.set_exit(unpack_binary_lump(misc_wad, src, header.exit, "exit", "exit.bin"));
	misc_wad.set_boot(unpack_binary_lump(misc_wad, src, header.bootwad, "boot", "boot.bin"));
	misc_wad.set_gadget(unpack_binary_lump(misc_wad, src, header.gadget, "gadget", "gadget.bin"));
}
