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

#include <spanner/asset_packer.h>

packed_struct(SpaceWadHeaderDL,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorRange transition_wads[12];
)

void unpack_space_wad(SpaceWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<SpaceWadHeaderDL>(src);
	
	unpack_compressed_binaries(dest.transitions(), *file, ARRAY_PAIR(header.transition_wads));
}

void pack_space_wad(OutputStream& dest, std::vector<u8>* header_dest, SpaceWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	SpaceWadHeaderDL header = {0};
	header.header_size = sizeof(SpaceWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.transition_wads), wad.get_transitions(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
