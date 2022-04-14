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

#include "level_audio_wad.h"

#include <spanner/asset_packer.h>

packed_struct(LevelAudioWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorByteRange bin_data[80];
	/* 0x288 */ SectorByteRange upgrade_sample;
	/* 0x290 */ SectorByteRange platinum_bolt;
	/* 0x298 */ SectorByteRange spare;
)

void unpack_level_audio_wad(LevelAudioWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<LevelAudioWadHeaderDL>(src);
	
	// ...
}

void pack_level_audio_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelAudioWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	LevelAudioWadHeaderDL header = {0};
	header.header_size = sizeof(LevelAudioWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	// ...
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
