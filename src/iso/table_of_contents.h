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

#ifndef ISO_TABLE_OF_CONTENTS_H
#define ISO_TABLE_OF_CONTENTS_H

#include <core/stream.h>
#include <assetmgr/asset.h>

struct GlobalWadInfo {
	size_t index;
	u32 offset_in_toc;
	Sector32 sector;
	std::vector<u8> header;
	Asset* asset;
	std::string name;
};

packed_struct(toc_level_table_entry,
	SectorRange parts[3];
)

struct LevelWadInfo {
	Sector32 header_lba;
	Sector32 file_lba;
	Sector32 file_size;
	std::vector<u8> header;
	bool prepend_header = false;
	Asset* asset;
};

struct LevelInfo {
	Opt<LevelWadInfo> level;
	Opt<LevelWadInfo> audio;
	Opt<LevelWadInfo> scene;
	s32 level_table_index;
	s64 level_table_entry_offset = 0;
};

struct table_of_contents {
	std::vector<GlobalWadInfo> globals;
	std::vector<LevelInfo> levels;
};

packed_struct(Rac1SceneHeader,
	/* 0x000 */ Sector32 sounds[6];
	/* 0x018 */ Sector32 wads[68];
)
static_assert(sizeof(Rac1SceneHeader) == 0x128);

// This is what's actually stored on disc. The sector numbers are absolute and
// in the case of the audio and scene data, point to sectors before the header.
packed_struct(Rac1AmalgamatedWadHeader,
	/* 0x000 */ s32 level_number;
	/* 0x004 */ s32 header_size;
	/* 0x008 */ SectorRange primary;
	/* 0x010 */ SectorRange gameplay_ntsc;
	/* 0x018 */ SectorRange gameplay_pal;
	/* 0x020 */ SectorRange occlusion;
	/* 0x028 */ SectorByteRange bindata[36];
	/* 0x148 */ Sector32 music[15];
	/* 0x184 */ Rac1SceneHeader scenes[30];
)
static_assert(sizeof(Rac1AmalgamatedWadHeader) == 0x2434);

// These are the files that get dumped out by the Wrench ISO utility. Sector
// numbers are relative to the start of the file. This is specific to Wrench.
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

packed_struct(Rac1AudioWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 pad_4;
	/* 0x008 */ SectorByteRange bindata[36];
	/* 0x128 */ Sector32 music[15];
)

packed_struct(Rac1SceneWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 pad_4;
	/* 0x008 */ Rac1SceneHeader scenes[30];
)

packed_struct(VagHeader,
	/* 0x00 */ char magic[4]; // "VAGp"
	/* 0x04 */ s32 version;
	/* 0x08 */ s32 reserved_8;
	/* 0x0c */ s32 data_size;
	/* 0x10 */ s32 frequency;
	/* 0x14 */ u8 reserved_14[10];
	/* 0x1e */ u8 channel_count;
	/* 0x1f */ u8 reserved_1f;
	/* 0x20 */ char name[16];
)
static_assert(sizeof(VagHeader) == 0x30);

packed_struct(LzHeader,
	char magic[3]; // "WAD"
	s32 compressed_size;
)

static const uint32_t SYSTEM_CNF_LBA = 1000;
static const uint32_t RAC1_TABLE_OF_CONTENTS_LBA = 1500;
static const uint32_t RAC234_TABLE_OF_CONTENTS_LBA = 1001;

static const std::size_t TOC_MAX_SIZE       = 0x200000;
static const std::size_t TOC_MAX_INDEX_SIZE = 0x10000;
static const std::size_t TOC_MAX_LEVELS     = 100;

table_of_contents read_table_of_contents(InputStream& src, Game game);
table_of_contents read_table_of_contents_rac1(InputStream& src);
table_of_contents read_table_of_contents_rac234(InputStream& src);

s64 write_table_of_contents_rac234(OutputStream& iso, const table_of_contents& toc, Game game);
Sector32 calculate_table_of_contents_size(const table_of_contents& toc, s32 single_level_index);

#endif
