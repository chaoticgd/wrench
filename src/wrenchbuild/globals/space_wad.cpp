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

packed_struct(UyaSpaceWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange stuff[6];
	/* 0x038 */ SectorRange wads[9];
	/* 0x080 */ Sector32 vags_1[2];
	/* 0x088 */ SectorRange subtitles;
	/* 0x090 */ Sector32 vags_2[10];
	/* 0x0b8 */ Sector32 wads_2[69];
	/* 0x1cc */ Sector32 vags_3[14];
	/* 0x204 */ Sector32 wads_3[83];
	/* 0x350 */ Sector32 wads_4[484];
	/* 0xae0 */ SectorRange moby_classes[42];
)
static_assert(sizeof(UyaSpaceWadHeader) == 0xc30);

packed_struct(DlSpaceWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorRange transition_wads[12];
)

void unpack_uya_space_wad(
	SpaceWadAsset& dest, const UyaSpaceWadHeader& header, InputStream& src, BuildConfig config);
static void pack_uya_space_wad(
	OutputStream& dest, UyaSpaceWadHeader& header, const SpaceWadAsset& src, BuildConfig config);
void unpack_dl_space_wad(
	SpaceWadAsset& dest, const DlSpaceWadHeader& header, InputStream& src, BuildConfig config);
static void pack_dl_space_wad(
	OutputStream& dest, DlSpaceWadHeader& header, const SpaceWadAsset& src, BuildConfig config);

on_load(Space, []() {
	SpaceWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<SpaceWadAsset, DlSpaceWadHeader>(unpack_dl_space_wad);
	
	SpaceWadAsset::funcs.pack_dl = wrap_wad_packer_func<SpaceWadAsset, DlSpaceWadHeader>(pack_dl_space_wad);
})

void unpack_uya_space_wad(SpaceWadAsset& dest, const UyaSpaceWadHeader& header, InputStream& src, BuildConfig config)
{
	
}

static void pack_uya_space_wad(OutputStream& dest, UyaSpaceWadHeader& header, const SpaceWadAsset& src, BuildConfig config)
{
	
}

void unpack_dl_space_wad(SpaceWadAsset& dest, const DlSpaceWadHeader& header, InputStream& src, BuildConfig config)
{
	unpack_compressed_assets<BinaryAsset>(dest.transitions(), src, ARRAY_PAIR(header.transition_wads), config);
}

static void pack_dl_space_wad(OutputStream& dest, DlSpaceWadHeader& header, const SpaceWadAsset& src, BuildConfig config)
{
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.transition_wads), src.get_transitions(), config, "trnsition");
}
