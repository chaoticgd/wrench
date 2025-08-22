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

struct GlobalWadInfo
{
	size_t index;
	u32 offset_in_toc;
	Sector32 sector;
	std::vector<u8> header;
	const Asset* asset;
	std::string name;
};

packed_struct(toc_level_table_entry,
	SectorRange parts[3];
)

struct LevelWadInfo
{
	Sector32 header_lba;
	Sector32 file_lba;
	Sector32 file_size;
	std::vector<u8> header;
	bool prepend_header = false;
	const Asset* asset;
};

struct LevelInfo
{
	Opt<LevelWadInfo> level;
	Opt<LevelWadInfo> audio;
	Opt<LevelWadInfo> scene;
	s32 level_table_index; // Not used for writing.
	s32 level_table_entry_offset = 0; // Not used for writing.
};

struct table_of_contents
{
	std::vector<GlobalWadInfo> globals;
	std::vector<LevelInfo> levels;
};

packed_struct(RacWadInfo,
	/* 0x0000 */ s32 version;
	/* 0x0004 */ s32 header_size;
	/* 0x0008 */ SectorRange debug_font;
	/* 0x0010 */ SectorRange save_game;
	/* 0x0018 */ SectorRange ratchet_seqs[28];
	/* 0x00f8 */ SectorRange hud_seqs[20];
	/* 0x0198 */ SectorRange vendor;
	/* 0x01a0 */ SectorRange vendor_audio[37];
	/* 0x02c8 */ SectorRange help_controls[12];
	/* 0x0328 */ SectorRange help_moves[15];
	/* 0x03a0 */ SectorRange help_weapons[15];
	/* 0x0418 */ SectorRange help_gadgets[14];
	/* 0x0488 */ SectorRange help_ss[7];
	/* 0x04c0 */ SectorRange options_ss[7];
	/* 0x04f8 */ SectorRange frontbin;
	/* 0x0500 */ SectorRange mission_ss[81];
	/* 0x0788 */ SectorRange planets[19];
	/* 0x0820 */ SectorRange stuff2[38];
	/* 0x0950 */ SectorRange goodies_images[10];
	/* 0x09a0 */ SectorRange character_sketches[19];
	/* 0x0a38 */ SectorRange character_renders[19];
	/* 0x0ad0 */ SectorRange skill_images[31];
	/* 0x0bc8 */ SectorRange epilogue_english[12];
	/* 0x0c28 */ SectorRange epilogue_french[12];
	/* 0x0c88 */ SectorRange epilogue_italian[12];
	/* 0x0ce8 */ SectorRange epilogue_german[12];
	/* 0x0d48 */ SectorRange epilogue_spanish[12];
	/* 0x0da8 */ SectorRange sketchbook[30];
	/* 0x0e98 */ SectorRange commercials[4];
	/* 0x0eb8 */ SectorRange item_images[9];
	/* 0x0f00 */ Sector32 qwark_boss_audio[240];
	/* 0x12c0 */ SectorRange irx;
	/* 0x12c8 */ SectorRange spaceships[4];
	/* 0x12e8 */ SectorRange anim_looking_thing_2[20];
	/* 0x1388 */ SectorRange space_plates[6];
	/* 0x13b8 */ SectorRange transition;
	/* 0x13c0 */ SectorRange space_audio[36];
	/* 0x14e0 */ SectorRange sound_bank;
	/* 0x14e0 */ SectorRange wad_14e0;
	/* 0x14e0 */ SectorRange music;
	/* 0x14f8 */ SectorRange hud_header;
	/* 0x1500 */ SectorRange hud_banks[5];
	/* 0x1528 */ SectorRange all_text;
	/* 0x1530 */ SectorRange things[28];
	/* 0x1610 */ SectorRange post_credits_helpdesk_girl_seq;
	/* 0x1618 */ SectorRange post_credits_audio[18];
	/* 0x16a8 */ SectorRange credits_images_ntsc[20];
	/* 0x1748 */ SectorRange credits_images_pal[20];
	/* 0x17e8 */ SectorRange wad_things[2];
	/* 0x17f8 */ SectorByteRange mpegs[88];
	/* 0x1ab8 */ Sector32 help_audio[900];
	/* 0x28c8 */ SectorRange levels[19];
)
static_assert(sizeof(RacWadInfo) == 0x2960);

// This is to make it easier to rewrite the relative sector offsets to absolute
// sector numbers.
packed_struct(DumbRacWadInfo,
	/* 0x0000 */ s32 version;
	/* 0x0004 */ s32 header_size;
	/* 0x0008 */ SectorRange sector_ranges_1[479];
	/* 0x0f00 */ Sector32 sectors_1[240];
	/* 0x12c0 */ SectorRange sector_ranges_2[167];
	/* 0x17f8 */ SectorByteRange sector_byte_ranges[88];
	/* 0x1ab8 */ Sector32 sectors_2[900];
	/* 0x28c8 */ SectorRange levels[19];
)
static_assert(sizeof(DumbRacWadInfo) == 0x2960);

packed_struct(RacSceneHeader,
	/* 0x000 */ Sector32 sounds[6];
	/* 0x018 */ Sector32 wads[68];
)
static_assert(sizeof(RacSceneHeader) == 0x128);

// This is what's actually stored on disc. The sector numbers are absolute and
// in the case of the audio and scene data, point to sectors before the header.
packed_struct(Rac1AmalgamatedWadHeader,
	/* 0x000 */ s32 id;
	/* 0x004 */ s32 header_size;
	/* 0x008 */ SectorRange data;
	/* 0x010 */ SectorRange gameplay_ntsc;
	/* 0x018 */ SectorRange gameplay_pal;
	/* 0x020 */ SectorRange occlusion;
	/* 0x028 */ SectorByteRange bindata[36];
	/* 0x148 */ Sector32 music[15];
	/* 0x184 */ RacSceneHeader scenes[30];
)
static_assert(sizeof(Rac1AmalgamatedWadHeader) == 0x2434);

// These are the files that get dumped out by the Wrench ISO utility. Sector
// numbers are relative to the start of the file. This is specific to Wrench.
packed_struct(RacLevelWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 unused_4;
	/* 0x008 */ s32 id;
	/* 0x00c */ s32 unused_c;
	/* 0x010 */ SectorRange data;
	/* 0x018 */ SectorRange gameplay_ntsc;
	/* 0x020 */ SectorRange gameplay_pal;
	/* 0x028 */ SectorRange occlusion;
)
static_assert(sizeof(RacLevelWadHeader) == 0x30);

packed_struct(RacLevelAudioWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 pad_4;
	/* 0x008 */ SectorByteRange bindata[36];
	/* 0x128 */ Sector32 music[15];
)
static_assert(sizeof(RacLevelAudioWadHeader) == 0x164);

packed_struct(RacLevelSceneWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ s32 pad_4;
	/* 0x008 */ RacSceneHeader scenes[30];
)
static_assert(sizeof(RacLevelSceneWadHeader) == 0x22b8);

packed_struct(LzHeader,
	char magic[3]; // "WAD"
	s32 compressed_size;
)

static const uint32_t RAC_SYSTEM_CNF_LBA = 289;
static const uint32_t GC_UYA_DL_SYSTEM_CNF_LBA = 1000;
static const uint32_t RAC_TABLE_OF_CONTENTS_LBA = 1500;
static const uint32_t GC_UYA_DL_TABLE_OF_CONTENTS_LBA = 1001;

static const std::size_t TOC_MAX_SIZE       = 0x200000;
static const std::size_t TOC_MAX_INDEX_SIZE = 0x10000;
static const std::size_t TOC_MAX_LEVELS     = 100;

table_of_contents read_table_of_contents(InputStream& src, Game game);
s64 write_table_of_contents(OutputStream& iso, const table_of_contents& toc, Game game);
Sector32 calculate_table_of_contents_size(const table_of_contents& toc, Game game);

table_of_contents read_table_of_contents_rac(InputStream& src);
s64 write_table_of_contents_rac(OutputStream& iso, const table_of_contents& toc, Game game);
Sector32 calculate_table_of_contents_size_rac(const table_of_contents& toc);

table_of_contents read_table_of_contents_rac234(InputStream& src);
s64 write_table_of_contents_rac234(OutputStream& iso, const table_of_contents& toc, Game game);
Sector32 calculate_table_of_contents_size_rac234(const table_of_contents& toc);

#endif
