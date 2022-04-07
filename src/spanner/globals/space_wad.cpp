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

#include "space_wad.h"

packed_struct(SpaceWadHeaderDL,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorRange transition_wads[12];
)

SpaceWadAsset& unpack_space_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<SpaceWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("space/space.asset");
	
	SpaceWadAsset& wad = asset_file.root().child<SpaceWadAsset>("space");
	wad.set_transitions(unpack_compressed_binaries(wad, *file, ARRAY_PAIR(header.transition_wads), "transitions"));
	
	return wad;
}

void pack_space_wad(OutputStream& dest, SpaceWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	SpaceWadHeaderDL header = {0};
	header.header_size = sizeof(SpaceWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	// ...
	
	dest.write(base, header);
}
