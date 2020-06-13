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
	uint32_t offset_in_toc;
	toc_table_header header;
	array_stream data;
};

packed_struct(toc_level_table_entry,
	sector32 header_1;
	uint32_t unknown_4;
	sector32 header_2;
	uint32_t unknown_c;
	sector32 header_3;
	uint32_t unknown_14;
)

packed_struct(level_file_header_60,
	uint32_t magic;    // 0x0 Equal to 0x60.
	sector32 base_offset;    // 0x4
	uint32_t level_number;   // 0x8
	uint32_t unknown_c;      // 0xc
	sector32 primary_header; // 0x10
	uint32_t unknown_14;     // 0x14
	sector32 unknown_18;     // 0x18
	uint32_t unknown_1c;     // 0x1c
	sector32 moby_segment;   // 0x20
)

packed_struct(level_file_header_68,
	uint32_t magic;    // 0x0 Equal to 0x68.
	sector32 base_offset;    // 0x4
	uint32_t level_number;   // 0x8
	sector32 primary_header; // 0xc
	uint32_t unknown_10;     // 0x10
	sector32 unknown_14;     // 0x14
	uint32_t unknown_18;     // 0x18
	sector32 moby_segment;   // 0x1c
)

struct level_file_header {
	uint32_t magic;
	uint32_t base_offset;
	uint32_t level_number;
	uint32_t primary_header_offset;
	uint32_t moby_segment_offset;
};

struct toc_level {
	level_file_header main_part;
	sector32 audio_part;
	sector32 scene_part;
};

struct table_of_contents {
	std::vector<toc_table> tables;
	std::vector<toc_level> levels;
};

table_of_contents read_toc(stream& iso, std::size_t toc_base);
std::optional<level_file_header> level_read_file_header(stream* src, std::size_t offset);

#endif
