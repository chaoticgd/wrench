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

packed_struct(DeadlockedSpaceWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorRange transition_wads[12];
)

void unpack_space_wad(SpaceWadAsset& dest, InputStream& src, Game game);
static void pack_space_wad(OutputStream& dest, DeadlockedSpaceWadHeader& header, SpaceWadAsset& src, Game game);

on_load(Space, []() {
	SpaceWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<SpaceWadAsset>(unpack_space_wad);
	
	SpaceWadAsset::funcs.pack_dl = wrap_wad_packer_func<SpaceWadAsset, DeadlockedSpaceWadHeader>(pack_space_wad);
})

void unpack_space_wad(SpaceWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedSpaceWadHeader>(0);
	
	unpack_compressed_assets<BinaryAsset>(dest.transitions(), src, ARRAY_PAIR(header.transition_wads), game);
}

static void pack_space_wad(OutputStream& dest, DeadlockedSpaceWadHeader& header, SpaceWadAsset& src, Game game) {
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.transition_wads), src.get_transitions(), game, "trnsition");
}
