/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#ifndef WAD_WAD_FILE_H
#define WAD_WAD_FILE_H

#include <vector>

#include <core/util.h>
#include <core/level.h>
#include <core/buffer.h>
#include <engine/compression.h>
#include <wad/gameplay.h>

// Note: This header is specific to files emitted by the Wrench ISO utility and
// the Wrench WAD utility. The header stored on the disc is different and is not
// usable as a regular file header as-is.
packed_struct(Rac1LevelWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 pad_4;
	/* 0x008 */ s32 level_number;
	/* 0x00c */ s32 pad_c;
	/* 0x010 */ SectorRange primary;
	/* 0x018 */ SectorRange gameplay_ntsc;
	/* 0x020 */ SectorRange gameplay_pal;
	/* 0x028 */ SectorRange occlusion;
)
static_assert(sizeof(Rac1LevelWadHeader) == 0x30);

packed_struct(Rac23LevelWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ s32 lba;
	/* 0x08 */ s32 level_number;
	/* 0x0c */ s32 reverb;
	/* 0x10 */ SectorRange primary;
	/* 0x18 */ SectorRange core_bank;
	/* 0x20 */ SectorRange gameplay;
	/* 0x28 */ SectorRange occlusion;
	/* 0x30 */ SectorRange chunks[3];
	/* 0x48 */ SectorRange chunk_banks[3];
)
static_assert(sizeof(Rac23LevelWadHeader) == 0x60);

packed_struct(Rac23LevelWadHeader68,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ s32 lba;
	/* 0x08 */ s32 level_number;
	/* 0x0c */ SectorRange primary;
	/* 0x14 */ SectorRange core_bank;
	/* 0x1c */ SectorRange gameplay_1;
	/* 0x24 */ SectorRange gameplay_2;
	/* 0x2c */ SectorRange occlusion;
	/* 0x34 */ SectorRange chunks[3];
	/* 0x4c */ s32 reverb;
	/* 0x50 */ SectorRange chunk_banks[3];
)
static_assert(sizeof(Rac23LevelWadHeader68) == 0x68);

packed_struct(DeadlockedLevelWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 lba;
	/* 0x008 */ s32 level_number;
	/* 0x00c */ s32 reverb;
	/* 0x010 */ s32 max_mission_instances_size;
	/* 0x014 */ s32 max_mission_classes_size;
	/* 0x018 */ SectorRange primary;
	/* 0x020 */ SectorRange core_bank;
	/* 0x028 */ SectorRange chunks[3];
	/* 0x040 */ SectorRange chunk_banks[3];
	/* 0x058 */ SectorRange gameplay_core;
	/* 0x060 */ SectorRange gameplay_mission_instances[128];
	/* 0x460 */ SectorRange gameplay_mission_data[128];
	/* 0x860 */ SectorRange mission_banks[128];
	/* 0xc60 */ SectorRange art_instances;
)
static_assert(sizeof(DeadlockedLevelWadHeader) == 0xc68);

packed_struct(ChunkHeader,
	/* 0x0 */ s32 tfrags;
	/* 0x4 */ s32 collision;
)

// These offsets are relative to the beginning of the level file.
packed_struct(MissionHeader,
	/* 0x0 */ ByteRange instances;
	/* 0x8 */ ByteRange classes;
)

std::unique_ptr<Wad> read_wad(FILE* file);
void write_wad(FILE* file, Wad* wad);

std::vector<u8> read_lump(FILE* file, SectorRange range, const char* name);
template <typename Header>
static Header read_header(FILE* file) {
	Header header;
	verify(fseek(file, 0, SEEK_SET) == 0, "Failed to seek to WAD header.");
	verify(fread(&header, sizeof(Header), 1, file) == 1, "Failed to read WAD header.");
	return header;
}
Game detect_game_rac23(std::vector<u8>& src);

#endif
