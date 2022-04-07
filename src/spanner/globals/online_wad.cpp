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

#include "online_wad.h"

packed_struct(OnlineWadHeaderDL,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange data;
	/* 0x10 */ SectorRange transition_backgrounds[11];
)

OnlineWadAsset& unpack_online_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<OnlineWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("online/online.asset");
	
	OnlineWadAsset& wad = asset_file.root().child<OnlineWadAsset>("online");
	wad.set_data(unpack_binary(wad, *file, header.data, "data", "data.bin"));
	wad.set_transition_backgrounds(unpack_binaries(wad, *file, ARRAY_PAIR(header.transition_backgrounds), "transition_backgrounds", ".bin"));
	
	return wad;
}

void pack_online_wad(OutputStream& dest, OnlineWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	OnlineWadHeaderDL header = {0};
	header.header_size = sizeof(OnlineWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	// ...
	
	dest.write(base, header);
}
