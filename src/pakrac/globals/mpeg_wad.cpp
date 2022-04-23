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

static void unpack_mpeg_wad(MpegWadAsset& dest, InputStream& src, Game game);
static void pack_mpeg_wad(OutputStream& dest, std::vector<u8>* header_dest, MpegWadAsset& src, Game game);

on_load(Mpeg, []() {
	MpegWadAsset::funcs.unpack_dl = wrap_unpacker_func<MpegWadAsset>(unpack_mpeg_wad);
	
	MpegWadAsset::funcs.pack_dl = wrap_wad_packer_func<MpegWadAsset>(pack_mpeg_wad);
})

packed_struct(DeadlockedMpegWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorByteRange story[200];
)

static void unpack_mpeg_wad(MpegWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedMpegWadHeader>(0);
	
	unpack_assets<BinaryAsset>(dest.story().switch_files(), src, ARRAY_PAIR(header.story), game);
}

static void pack_mpeg_wad(OutputStream& dest, std::vector<u8>* header_dest, MpegWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	DeadlockedMpegWadHeader header = {0};
	header.header_size = sizeof(DeadlockedMpegWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.story), src.get_story(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
