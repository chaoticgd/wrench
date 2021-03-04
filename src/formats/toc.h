/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef FORMATS_TOC_H
#define FORMATS_TOC_H

#include "../stream.h"

# /*
# 	Read the sector table/table of contents (.HDR) file.
# */

packed_struct(toc_table_header,
	uint32_t size;
	sector32 base_offset;
)

struct toc_table {
	std::size_t index;
	uint32_t offset_in_toc;
	toc_table_header header;
	array_stream data;
};

packed_struct(toc_level_table_entry,
	sector32 header_1;
	sector32 header_1_size;
	sector32 header_2;
	sector32 header_2_size;
	sector32 header_3;
	sector32 header_3_size;
)

struct toc_level {
	std::size_t level_table_index;
	size_t main_part_size_offset; // Absolute offset of size field in ISO.
	sector32 main_part;
	sector32 main_part_size;
	sector32 audio_part;
	sector32 audio_part_size;
	sector32 scene_part;
	sector32 scene_part_size;
};

struct table_of_contents {
	std::vector<toc_table> tables;
	std::vector<toc_level> levels;
};

static const std::size_t TOC_MAX_SIZE       = 0x100000;
static const std::size_t TOC_MAX_INDEX_SIZE = 0x10000;
static const std::size_t TOC_MAX_LEVELS     = 0x100;

static const std::vector<uint32_t> TOC_MAIN_PART_MAGIC = { 0x60, 0x68, 0xc68 };
static const std::vector<uint32_t> TOC_AUDIO_PART_MAGIC = { 0x1018, 0x1818, 0x1000, 0x2a0 };
static const std::vector<uint32_t> TOC_SCENE_PART_MAGIC = { 0x137c, 0x2420, 0x26f0 };

table_of_contents read_toc(stream& iso, std::size_t toc_base);
std::size_t toc_get_level_table_offset(stream& iso, std::size_t toc_base);

#endif
