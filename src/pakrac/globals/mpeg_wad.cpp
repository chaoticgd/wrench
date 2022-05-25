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

packed_struct(MpegHeader,
	SectorByteRange subtitles;
	SectorByteRange video;
)

packed_struct(UyaDlMpegWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ MpegHeader mpegs[100];
)

static void unpack_mpeg_wad(MpegWadAsset& dest, const UyaDlMpegWadHeader& header, InputStream& src, Game game);
static void pack_mpeg_wad(OutputStream& dest, UyaDlMpegWadHeader& header, MpegWadAsset& src, Game game);

on_load(Mpeg, []() {
	MpegWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<MpegWadAsset, UyaDlMpegWadHeader>(unpack_mpeg_wad);
	MpegWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<MpegWadAsset, UyaDlMpegWadHeader>(unpack_mpeg_wad);
	
	MpegWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<MpegWadAsset, UyaDlMpegWadHeader>(pack_mpeg_wad);
	MpegWadAsset::funcs.pack_dl = wrap_wad_packer_func<MpegWadAsset, UyaDlMpegWadHeader>(pack_mpeg_wad);
})

static void unpack_mpeg_wad(MpegWadAsset& dest, const UyaDlMpegWadHeader& header, InputStream& src, Game game) {
	for(s32 i = 0; i < ARRAY_SIZE(header.mpegs); i++) {
		MpegAsset& mpeg = dest.mpegs().child<MpegAsset>(i).switch_files();
		unpack_asset(mpeg.video(), src, header.mpegs[i].video, game, FMT_BINARY_PSS);
		unpack_asset(mpeg.subtitles(), src, header.mpegs[i].subtitles, game);
	}
}

static void pack_mpeg_wad(OutputStream& dest, UyaDlMpegWadHeader& header, MpegWadAsset& src, Game game) {
	CollectionAsset& mpegs = src.get_mpegs();
	for(s32 i = 0; i < ARRAY_SIZE(header.mpegs); i++) {
		if(mpegs.has_child(i)) {
			MpegAsset& mpeg = mpegs.get_child(i).as<MpegAsset>();
			if(mpeg.has_video()) {
				header.mpegs[i].subtitles = pack_asset_sa<SectorByteRange>(dest, mpeg.get_video(), game);
			}
			header.mpegs[i].video = pack_asset_sa<SectorByteRange>(dest, mpeg.get_video(), game);
		}
	}
}
