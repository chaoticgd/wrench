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

packed_struct(OnlineWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange data;
	/* 0x10 */ SectorRange transition_backgrounds[11];
)

static void unpack_online_wad(
	OnlineWadAsset& dest, const OnlineWadHeader& header, InputStream& src, BuildConfig config);
static void pack_online_wad(
	OutputStream& dest, OnlineWadHeader& header, const OnlineWadAsset& src, BuildConfig config);

on_load(Online, []() {
	OnlineWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<OnlineWadAsset, OnlineWadHeader>(unpack_online_wad);
	
	OnlineWadAsset::funcs.pack_dl = wrap_wad_packer_func<OnlineWadAsset, OnlineWadHeader>(pack_online_wad);
})

static void unpack_online_wad(
	OnlineWadAsset& dest, const OnlineWadHeader& header, InputStream& src, BuildConfig config)
{
	unpack_asset(dest.data<OnlineDataWadAsset>(), src, header.data, config);
	unpack_assets<TextureAsset>(dest.transition_backgrounds(SWITCH_FILES), src, ARRAY_PAIR(header.transition_backgrounds), config, FMT_TEXTURE_RGBA);
}

static void pack_online_wad(
	OutputStream& dest, OnlineWadHeader& header, const OnlineWadAsset& src, BuildConfig config)
{
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), config);
	pack_assets_sa(dest, ARRAY_PAIR(header.transition_backgrounds), src.get_transition_backgrounds(), config, FMT_TEXTURE_RGBA);
}
