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

#include "level_scene_wad.h"

#include <spanner/asset_packer.h>

packed_struct(SceneHeaderDL,
	/* 0x00 */ s32 speech_english_left;
	/* 0x04 */ s32 speech_english_right;
	/* 0x08 */ SectorRange subtitles;
	/* 0x10 */ s32 speech_french_left;
	/* 0x14 */ s32 speech_french_right;
	/* 0x18 */ s32 speech_german_left;
	/* 0x1c */ s32 speech_german_right;
	/* 0x20 */ s32 speech_spanish_left;
	/* 0x24 */ s32 speech_spanish_right;
	/* 0x28 */ s32 speech_italian_left;
	/* 0x2c */ s32 speech_italian_right;
	/* 0x30 */ SectorRange moby_load;
	/* 0x38 */ struct Sector32 chunks[69];
)

packed_struct(LevelSceneWadHeaderDL,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SceneHeaderDL scenes[30];
)

void unpack_level_scene_wad(LevelSceneWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<LevelSceneWadHeaderDL>(src);
	
	// ...
}

void pack_level_scene_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelSceneWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	LevelSceneWadHeaderDL header = {0};
	header.header_size = sizeof(LevelSceneWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	// ...
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
