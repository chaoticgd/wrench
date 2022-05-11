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

packed_struct(Rac2IrxModules,
	/* 0x00 */ ByteRange sio2man;
	/* 0x08 */ ByteRange mcman;
	/* 0x10 */ ByteRange mcserv;
	/* 0x18 */ ByteRange padman;
	/* 0x20 */ ByteRange mtapman;
	/* 0x28 */ ByteRange libsd;
	/* 0x30 */ ByteRange _989snd;
)

packed_struct(Rac3IrxModules,
	/* 0x00 */ ByteRange stash;
	/* 0x08 */ ByteRange inet;
	/* 0x10 */ ByteRange netcnf;
	/* 0x18 */ ByteRange inetctl;
	/* 0x20 */ ByteRange msifrpc;
	/* 0x28 */ ByteRange dev9;
	/* 0x30 */ ByteRange smap;
	/* 0x38 */ ByteRange libnetb;
	/* 0x40 */ ByteRange ppp;
	/* 0x48 */ ByteRange pppoe;
	/* 0x50 */ ByteRange usbd;
	/* 0x58 */ ByteRange lgaud;
	/* 0x60 */ ByteRange eznetcnf;
	/* 0x68 */ ByteRange eznetctl;
	/* 0x70 */ ByteRange lgkbm;
)

packed_struct(DeadlockedIrxModules,
	/* 0x0 */ ByteRange streamer;
	/* 0x8 */ ByteRange astrm;
)

static void unpack_rac1_irx_wad(IrxWadAsset& dest, InputStream& src, Game game);
static void pack_rac1_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game);
static void unpack_rac2_irx_wad(IrxWadAsset& dest, InputStream& src, Game game);
static void pack_rac2_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game);
static void unpack_rac3_irx_wad(IrxWadAsset& dest, InputStream& src, Game game);
static void pack_rac3_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game);
static void unpack_dl_irx_wad(IrxWadAsset& dest, InputStream& src, Game game);
static void pack_dl_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game);
static void unpack_rac2_irx_modules(IrxWadAsset& dest, const Rac2IrxModules& header, InputStream& src, Game game);
static Rac2IrxModules pack_rac2_irx_modules(OutputStream& dest, IrxWadAsset& src, Game game);
static void unpack_rac3_irx_modules(IrxWadAsset& dest, const Rac3IrxModules& header, InputStream& src, Game game);
static Rac3IrxModules pack_rac3_irx_modules(OutputStream& dest, IrxWadAsset& src, Game game);
static void unpack_dl_irx_modules(IrxWadAsset& dest, const DeadlockedIrxModules& header, InputStream& src, Game game);
static DeadlockedIrxModules pack_dl_irx_modules(OutputStream& dest, IrxWadAsset& src, Game game);

on_load(Irx, []() {
	IrxWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<IrxWadAsset>(unpack_rac1_irx_wad);
	IrxWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<IrxWadAsset>(unpack_rac2_irx_wad);
	IrxWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<IrxWadAsset>(unpack_rac3_irx_wad);
	IrxWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<IrxWadAsset>(unpack_dl_irx_wad);
	
	IrxWadAsset::funcs.pack_rac1 = wrap_packer_func<IrxWadAsset>(pack_rac1_irx_wad);
	IrxWadAsset::funcs.pack_rac2 = wrap_packer_func<IrxWadAsset>(pack_rac2_irx_wad);
	IrxWadAsset::funcs.pack_rac3 = wrap_packer_func<IrxWadAsset>(pack_rac3_irx_wad);
	IrxWadAsset::funcs.pack_dl = wrap_packer_func<IrxWadAsset>(pack_dl_irx_wad);
})

packed_struct(Rac2IrxHeader,
	/* 0x00 */ ByteRange texture;
	/* 0x04 */ ByteRange unused[2];
	/* 0x18 */ Rac2IrxModules rac2;
)

packed_struct(Rac3IrxHeader,
	/* 0x00 */ s32 unused[2];
	/* 0x08 */ Rac2IrxModules rac2;
	/* 0x40 */ Rac3IrxModules rac3;
)

packed_struct(DeadlockedIrxHeader,
	/* 0x00 */ s32 unused[2];
	/* 0x08 */ Rac2IrxModules rac2;
	/* 0x40 */ Rac3IrxModules rac3;
	/* 0xb8 */ DeadlockedIrxModules dl;
)

static void unpack_rac1_irx_wad(IrxWadAsset& dest, InputStream& src, Game game) {
	verify_not_reached("R&C1 IRX unpacking not yet implemented.");
}

static void pack_rac1_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game) {
	verify_not_reached("R&C1 IRX packing not yet implemented.");
}

static void unpack_rac2_irx_wad(IrxWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac2IrxHeader>(0);
	unpack_rac2_irx_modules(dest, header.rac2, src, game);
}

static void pack_rac2_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game) {
	Rac2IrxHeader header = {0};
	dest.write(header);
	header.rac2 = pack_rac2_irx_modules(dest, src, game);
	dest.write(0, header);
}

static void unpack_rac3_irx_wad(IrxWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac3IrxHeader>(0);
	unpack_rac2_irx_modules(dest, header.rac2, src, game);
	unpack_rac3_irx_modules(dest, header.rac3, src, game);
}

static void pack_rac3_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game) {
	Rac3IrxHeader header = {0};
	dest.write(header);
	header.rac2 = pack_rac2_irx_modules(dest, src, game);
	header.rac3 = pack_rac3_irx_modules(dest, src, game);
	dest.write(0, header);
}

static void unpack_dl_irx_wad(IrxWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedIrxHeader>(0);
	unpack_rac2_irx_modules(dest, header.rac2, src, game);
	unpack_rac3_irx_modules(dest, header.rac3, src, game);
	unpack_dl_irx_modules(dest, header.dl, src, game);
}

static void pack_dl_irx_wad(OutputStream& dest, IrxWadAsset& src, Game game) {
	DeadlockedIrxHeader header = {0};
	dest.write(header);
	header.rac2 = pack_rac2_irx_modules(dest, src, game);
	header.rac3 = pack_rac3_irx_modules(dest, src, game);
	header.dl = pack_dl_irx_modules(dest, src, game);
	dest.write(0, header);
}

static void unpack_rac2_irx_modules(IrxWadAsset& dest, const Rac2IrxModules& header, InputStream& src, Game game) {
	unpack_asset(dest.sio2man(), src, header.sio2man, game);
	unpack_asset(dest.mcman(), src, header.mcman, game);
	unpack_asset(dest.mcserv(), src, header.mcserv, game);
	unpack_asset(dest.padman(), src, header.padman, game);
	unpack_asset(dest.mtapman(), src, header.mtapman, game);
	unpack_asset(dest.libsd(), src, header.libsd, game);
	unpack_asset(dest._989snd(), src, header._989snd, game);
}

static Rac2IrxModules pack_rac2_irx_modules(OutputStream& dest, IrxWadAsset& src, Game game) {
	Rac2IrxModules modules;
	modules.sio2man = pack_asset<ByteRange>(dest, src.get_sio2man(), game, 0x40);
	modules.mcman = pack_asset<ByteRange>(dest, src.get_mcman(), game, 0x40);
	modules.mcserv = pack_asset<ByteRange>(dest, src.get_mcserv(), game, 0x40);
	modules.padman = pack_asset<ByteRange>(dest, src.get_padman(), game, 0x40);
	modules.mtapman = pack_asset<ByteRange>(dest, src.get_mtapman(), game, 0x40);
	modules.libsd = pack_asset<ByteRange>(dest, src.get_libsd(), game, 0x40);
	modules._989snd = pack_asset<ByteRange>(dest, src.get_989snd(), game, 0x40);
	return modules;
}

static void unpack_rac3_irx_modules(IrxWadAsset& dest, const Rac3IrxModules& header, InputStream& src, Game game) {
	unpack_asset(dest.stash(), src, header.stash, game);
	unpack_asset(dest.inet(), src, header.inet, game);
	unpack_asset(dest.netcnf(), src, header.netcnf, game);
	unpack_asset(dest.inetctl(), src, header.inetctl, game);
	unpack_asset(dest.msifrpc(), src, header.msifrpc, game);
	unpack_asset(dest.dev9(), src, header.dev9, game);
	unpack_asset(dest.smap(), src, header.smap, game);
	unpack_asset(dest.libnetb(), src, header.libnetb, game);
	unpack_asset(dest.ppp(), src, header.ppp, game);
	unpack_asset(dest.pppoe(), src, header.pppoe, game);
	unpack_asset(dest.usbd(), src, header.usbd, game);
	unpack_asset(dest.lgaud(), src, header.lgaud, game);
	unpack_asset(dest.eznetcnf(), src, header.eznetcnf, game);
	unpack_asset(dest.eznetctl(), src, header.eznetctl, game);
	unpack_asset(dest.lgkbm(), src, header.lgkbm, game);
}

static Rac3IrxModules pack_rac3_irx_modules(OutputStream& dest, IrxWadAsset& src, Game game) {
	Rac3IrxModules modules;
	modules.stash = pack_asset<ByteRange>(dest, src.get_stash(), game, 0x40);
	modules.inet = pack_asset<ByteRange>(dest, src.get_inet(), game, 0x40);
	modules.netcnf = pack_asset<ByteRange>(dest, src.get_netcnf(), game, 0x40);
	modules.inetctl = pack_asset<ByteRange>(dest, src.get_inetctl(), game, 0x40);
	modules.msifrpc = pack_asset<ByteRange>(dest, src.get_msifrpc(), game, 0x40);
	modules.dev9 = pack_asset<ByteRange>(dest, src.get_dev9(), game, 0x40);
	modules.smap = pack_asset<ByteRange>(dest, src.get_smap(), game, 0x40);
	modules.libnetb = pack_asset<ByteRange>(dest, src.get_libnetb(), game, 0x40);
	modules.ppp = pack_asset<ByteRange>(dest, src.get_ppp(), game, 0x40);
	modules.pppoe = pack_asset<ByteRange>(dest, src.get_pppoe(), game, 0x40);
	modules.usbd = pack_asset<ByteRange>(dest, src.get_usbd(), game, 0x40);
	modules.lgaud = pack_asset<ByteRange>(dest, src.get_lgaud(), game, 0x40);
	modules.eznetcnf = pack_asset<ByteRange>(dest, src.get_eznetcnf(), game, 0x40);
	modules.eznetctl = pack_asset<ByteRange>(dest, src.get_eznetctl(), game, 0x40);
	modules.lgkbm = pack_asset<ByteRange>(dest, src.get_lgkbm(), game, 0x40);
	return modules;
}

static void unpack_dl_irx_modules(IrxWadAsset& dest, const DeadlockedIrxModules& header, InputStream& src, Game game) {
	unpack_asset(dest.streamer(), src, header.streamer, game);
	unpack_asset(dest.astrm(), src, header.astrm, game);
}

static DeadlockedIrxModules pack_dl_irx_modules(OutputStream& dest, IrxWadAsset& src, Game game) {
	DeadlockedIrxModules modules;
	modules.streamer = pack_asset<ByteRange>(dest, src.get_streamer(), game, 0x40);
	modules.astrm = pack_asset<ByteRange>(dest, src.get_astrm(), game, 0x40);
	return modules;
}
