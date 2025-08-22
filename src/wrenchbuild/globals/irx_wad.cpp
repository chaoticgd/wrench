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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(GcIrxHeader,
	/* 0x00 */ ByteRange image;
	/* 0x04 */ ByteRange unused[2];
	/* 0x18 */ ByteRange sio2man;
	/* 0x20 */ ByteRange mcman;
	/* 0x28 */ ByteRange mcserv;
	/* 0x30 */ ByteRange dbcman;
	/* 0x38 */ ByteRange sio2d;
	/* 0x40 */ ByteRange ds2u;
	/* 0x48 */ ByteRange stash;
	/* 0x50 */ ByteRange libsd;
	/* 0x58 */ ByteRange _989snd;
)

packed_struct(UyaIrxHeader,
	/* 0x00 */ s32 unused[2];
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
)

packed_struct(DlIrxHeader,
	/* 0x00 */ s32 unused[2];
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

static void unpack_rac_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_rac_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config);
static void unpack_gc_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_gc_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config);
static void unpack_uya_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_uya_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config);
template <typename Header>
static void unpack_uya_dl_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config);
template <typename Header>
static void pack_uya_dl_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config);

on_load(Irx, []() {
	IrxWadAsset::funcs.unpack_rac1 = wrap_unpacker_func<IrxWadAsset>(unpack_rac_irx_wad);
	IrxWadAsset::funcs.unpack_rac2 = wrap_unpacker_func<IrxWadAsset>(unpack_gc_irx_wad);
	IrxWadAsset::funcs.unpack_rac3 = wrap_unpacker_func<IrxWadAsset>(unpack_uya_dl_irx_wad<UyaIrxHeader>);
	IrxWadAsset::funcs.unpack_dl = wrap_unpacker_func<IrxWadAsset>(unpack_uya_dl_irx_wad<DlIrxHeader>);
	
	IrxWadAsset::funcs.pack_rac1 = wrap_packer_func<IrxWadAsset>(pack_rac_irx_wad);
	IrxWadAsset::funcs.pack_rac2 = wrap_packer_func<IrxWadAsset>(pack_gc_irx_wad);
	IrxWadAsset::funcs.pack_rac3 = wrap_packer_func<IrxWadAsset>(pack_uya_dl_irx_wad<UyaIrxHeader>);
	IrxWadAsset::funcs.pack_dl = wrap_packer_func<IrxWadAsset>(pack_uya_dl_irx_wad<DlIrxHeader>);
})

static void unpack_rac_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config)
{
	verify_not_reached("R&C1 IRX unpacking not yet implemented.");
}

static void pack_rac_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config)
{
	verify_not_reached("R&C1 IRX packing not yet implemented.");
}

static void unpack_gc_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<GcIrxHeader>(0);
	unpack_asset(dest.image<TextureAsset>(), src, header.image, config, FMT_TEXTURE_RGBA);
	unpack_asset(dest.sio2man(), src, header.sio2man, config);
	unpack_asset(dest.mcman(), src, header.mcman, config);
	unpack_asset(dest.mcserv(), src, header.mcserv, config);
	unpack_asset(dest.dbcman(), src, header.dbcman, config);
	unpack_asset(dest.sio2d(), src, header.sio2d, config);
	unpack_asset(dest.ds2u(), src, header.ds2u, config);
	unpack_asset(dest.stash(), src, header.stash, config);
	unpack_asset(dest.libsd(), src, header.libsd, config);
	unpack_asset(dest._989snd(), src, header._989snd, config);
}

static void pack_gc_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config)
{
	GcIrxHeader header = {0};
	dest.write(header);
	header.image = pack_asset<ByteRange>(dest, src.get_image(), config, 0x40, FMT_TEXTURE_RGBA);
	header.sio2man = pack_asset<ByteRange>(dest, src.get_sio2man(), config, 0x40);
	header.mcman = pack_asset<ByteRange>(dest, src.get_mcman(), config, 0x40);
	header.mcserv = pack_asset<ByteRange>(dest, src.get_mcserv(), config, 0x40);
	header.dbcman = pack_asset<ByteRange>(dest, src.get_dbcman(), config, 0x40);
	header.sio2d = pack_asset<ByteRange>(dest, src.get_sio2d(), config, 0x40);
	header.ds2u = pack_asset<ByteRange>(dest, src.get_ds2u(), config, 0x40);
	header.stash = pack_asset<ByteRange>(dest, src.get_stash(), config, 0x40);
	header.libsd = pack_asset<ByteRange>(dest, src.get_libsd(), config, 0x40);
	header._989snd = pack_asset<ByteRange>(dest, src.get_989snd(), config, 0x40);
	dest.write(0, header);
}

template <typename Header>
static void unpack_uya_dl_irx_wad(IrxWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<Header>(0);
	unpack_asset(dest.sio2man(), src, header.sio2man, config);
	unpack_asset(dest.mcman(), src, header.mcman, config);
	unpack_asset(dest.mcserv(), src, header.mcserv, config);
	unpack_asset(dest.padman(), src, header.padman, config);
	unpack_asset(dest.mtapman(), src, header.mtapman, config);
	unpack_asset(dest.libsd(), src, header.libsd, config);
	unpack_asset(dest._989snd(), src, header._989snd, config);
	unpack_asset(dest.stash(), src, header.stash, config);
	unpack_asset(dest.inet(), src, header.inet, config);
	unpack_asset(dest.netcnf(), src, header.netcnf, config);
	unpack_asset(dest.inetctl(), src, header.inetctl, config);
	unpack_asset(dest.msifrpc(), src, header.msifrpc, config);
	unpack_asset(dest.dev9(), src, header.dev9, config);
	unpack_asset(dest.smap(), src, header.smap, config);
	unpack_asset(dest.libnetb(), src, header.libnetb, config);
	unpack_asset(dest.ppp(), src, header.ppp, config);
	unpack_asset(dest.pppoe(), src, header.pppoe, config);
	unpack_asset(dest.usbd(), src, header.usbd, config);
	unpack_asset(dest.lgaud(), src, header.lgaud, config);
	unpack_asset(dest.eznetcnf(), src, header.eznetcnf, config);
	unpack_asset(dest.eznetctl(), src, header.eznetctl, config);
	unpack_asset(dest.lgkbm(), src, header.lgkbm, config);
	if constexpr(std::is_same_v<Header, DlIrxHeader>) {
		unpack_asset(dest.streamer(), src, header.streamer, config);
		unpack_asset(dest.astrm(), src, header.astrm, config);
	}
}

template <typename Header>
static void pack_uya_dl_irx_wad(OutputStream& dest, const IrxWadAsset& src, BuildConfig config)
{
	Header header = {0};
	dest.write(header);
	header.sio2man = pack_asset<ByteRange>(dest, src.get_sio2man(), config, 0x40);
	header.mcman = pack_asset<ByteRange>(dest, src.get_mcman(), config, 0x40);
	header.mcserv = pack_asset<ByteRange>(dest, src.get_mcserv(), config, 0x40);
	header.padman = pack_asset<ByteRange>(dest, src.get_padman(), config, 0x40);
	header.mtapman = pack_asset<ByteRange>(dest, src.get_mtapman(), config, 0x40);
	header.libsd = pack_asset<ByteRange>(dest, src.get_libsd(), config, 0x40);
	header._989snd = pack_asset<ByteRange>(dest, src.get_989snd(), config, 0x40);
	header.stash = pack_asset<ByteRange>(dest, src.get_stash(), config, 0x40);
	header.inet = pack_asset<ByteRange>(dest, src.get_inet(), config, 0x40);
	header.netcnf = pack_asset<ByteRange>(dest, src.get_netcnf(), config, 0x40);
	header.inetctl = pack_asset<ByteRange>(dest, src.get_inetctl(), config, 0x40);
	header.msifrpc = pack_asset<ByteRange>(dest, src.get_msifrpc(), config, 0x40);
	header.dev9 = pack_asset<ByteRange>(dest, src.get_dev9(), config, 0x40);
	header.smap = pack_asset<ByteRange>(dest, src.get_smap(), config, 0x40);
	header.libnetb = pack_asset<ByteRange>(dest, src.get_libnetb(), config, 0x40);
	header.ppp = pack_asset<ByteRange>(dest, src.get_ppp(), config, 0x40);
	header.pppoe = pack_asset<ByteRange>(dest, src.get_pppoe(), config, 0x40);
	header.usbd = pack_asset<ByteRange>(dest, src.get_usbd(), config, 0x40);
	header.lgaud = pack_asset<ByteRange>(dest, src.get_lgaud(), config, 0x40);
	header.eznetcnf = pack_asset<ByteRange>(dest, src.get_eznetcnf(), config, 0x40);
	header.eznetctl = pack_asset<ByteRange>(dest, src.get_eznetctl(), config, 0x40);
	header.lgkbm = pack_asset<ByteRange>(dest, src.get_lgkbm(), config, 0x40);
	if constexpr(std::is_same_v<Header, DlIrxHeader>) {
		header.streamer = pack_asset<ByteRange>(dest, src.get_streamer(), config, 0x40);
		header.astrm = pack_asset<ByteRange>(dest, src.get_astrm(), config, 0x40);
	}
	dest.write(0, header);
}
