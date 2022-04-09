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

#include "mpeg_wad.h"

#include <spanner/asset_packer.h>

packed_struct(MpegWadHeaderDL,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorByteRange story[200];
)

MpegWadAsset& unpack_mpeg_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<MpegWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("mpegs/mpegs.asset");
	
	MpegWadAsset& wad = asset_file.root().child<MpegWadAsset>("mpegs");
	wad.set_story(unpack_binaries(wad, *file, ARRAY_PAIR(header.story), "story", ".pss"));
	
	return wad;
}

void pack_mpeg_wad(OutputStream& dest, MpegWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	MpegWadHeaderDL header = {0};
	header.header_size = sizeof(MpegWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.story), wad.story(), game, base, "story");
	
	dest.write(base, header);
}
