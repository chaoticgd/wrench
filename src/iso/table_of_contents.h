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

#ifndef ISO_TABLE_OF_CONTENTS_H
#define ISO_TABLE_OF_CONTENTS_H

#include "../stream.h"
#include "../level_file_types.h"

packed_struct(toc_table_header,
	uint32_t header_size;
	sector32 base_offset;
)

struct toc_table {
	std::size_t index;
	uint32_t offset_in_toc;
	toc_table_header header;
	std::vector<sector_range> lumps;
};

packed_struct(toc_level_table_entry,
	sector_range parts[3];
)

struct toc_level_part {
	sector32 header_lba;
	sector32 file_size;
	uint32_t magic;
	sector32 file_lba;
	level_file_info info;
	std::vector<sector_range> lumps;
};

struct toc_level {
	size_t level_table_index;
	std::optional<toc_level_part> parts[3];
};

struct table_of_contents {
	std::vector<toc_table> tables;
	std::vector<toc_level> levels;
};

static const std::size_t TOC_MAX_SIZE       = 0x100000;
static const std::size_t TOC_MAX_INDEX_SIZE = 0x10000;
static const std::size_t TOC_MAX_LEVELS     = 100;

table_of_contents read_table_of_contents(stream& iso, std::size_t toc_base);
std::size_t toc_get_level_table_offset(stream& iso, std::size_t toc_base);

#endif
