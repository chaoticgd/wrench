/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef FORMATS_LEVEL_DATA_H
#define FORMATS_LEVEL_DATA_H

#include <memory>
#include <stdint.h>

#include "../level.h"
#include "../stream.h"
#include "wad.h"

struct level_data_header;
struct level_data_moby_table;
struct level_data_moby;

packed_struct(level_data_vec3f,
	float x;
	float y;
	float z;
)

packed_struct(level_data_header,
	uint8_t unknown1[0x4c];
	file_ptr<level_data_moby_table> mobies;
)

packed_struct(level_data_moby_table,
	uint32_t num_mobies;
	uint32_t unknown[3];
)

packed_struct(level_data_moby,
	uint32_t size;             // 0x0
	uint32_t unknown1[0x3];    // 0x4
	uint32_t uid;              // 0x10
	uint32_t unknown2[0xb];    // 0x14
	level_data_vec3f position; // 0x40
	level_data_vec3f rotation; // 0x4c
	uint32_t unknown3[0x2d];   // 0x58
)

std::unique_ptr<level_impl> import_level(stream& level_file);

uint32_t locate_main_level_segment(stream& level_file);

#endif
