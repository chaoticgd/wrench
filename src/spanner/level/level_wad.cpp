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

#include "level_wad.h"

#include <spanner/asset_packer.h>

packed_struct(LevelWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ s32 level;
	/* 0x00c */ s32 reverb;
	/* 0x010 */ s32 max_mission_sizes[2];
	/* 0x018 */ SectorRange data;
	/* 0x020 */ SectorRange core_sound_bank;
	/* 0x028 */ SectorRange chunk[3];
	/* 0x040 */ SectorRange chunk_sound_bank[3];
	/* 0x058 */ SectorRange gameplay_core;
	/* 0x060 */ SectorRange gameplay_mission_instances[128];
	/* 0x460 */ SectorRange gameplay_mission_data[128];
	/* 0x860 */ SectorRange mission_sound_banks[128];
	/* 0xc60 */ SectorRange art_instances;
)

void unpack_level_wad(LevelWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<LevelWadHeaderDL>(src);
	
	// ...
}

void pack_level_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	LevelWadHeaderDL header = {0};
	header.header_size = sizeof(LevelWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	// ...
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
