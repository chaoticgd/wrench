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

#include <spanner/asset_packer.h>

static void pack_online_wad(OutputStream& dest, std::vector<u8>* header_dest, OnlineWadAsset& src, Game game);

on_load([]() {
	OnlineWadAsset::pack_func = wrap_wad_packer_func<OnlineWadAsset>(pack_online_wad);
})

packed_struct(OnlineWadHeaderDL,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange data;
	/* 0x10 */ SectorRange transition_backgrounds[11];
)

static void pack_online_wad(OutputStream& dest, std::vector<u8>* header_dest, OnlineWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	OnlineWadHeaderDL header = {0};
	header.header_size = sizeof(OnlineWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	header.data = pack_asset_sa<SectorRange>(dest, src.get_data(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.transition_backgrounds), src.get_transition_backgrounds(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_online_wad(OnlineWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<OnlineWadHeaderDL>(src);
	
	unpack_binary(dest.data(), *file, header.data, "data.bin");
	unpack_binaries(dest.transition_backgrounds().switch_files(), *file, ARRAY_PAIR(header.transition_backgrounds), ".bin");
}
