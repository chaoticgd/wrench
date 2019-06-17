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

namespace level_data {
	struct header;
	struct moby_table;
	struct moby;

	packed_struct(vec3f,
		float x;
		float y;
		float z;
	)

	packed_struct(header,
		uint8_t unknown1[0x4c];
		file_ptr<moby_table> mobies;
	)

	packed_struct(moby_table,
		uint32_t num_mobies;
		uint32_t unknown[3];
	)

	packed_struct(moby,
		uint32_t size;      // 0x0
		uint32_t unknown1;  // 0x4
		uint32_t unknown2;  // 0x8
		uint32_t unknown3;  // 0xc
		uint32_t uid;       // 0x10
		uint32_t unknown4;  // 0x14
		uint32_t unknown5;  // 0x18
		uint32_t unknown6;  // 0x1c
		uint32_t unknown7;  // 0x20
		uint32_t unknown8;  // 0x24
		uint32_t class_num; // 0x28
		uint32_t unknown9;  // 0x2c
		uint32_t unknown10; // 0x30
		uint32_t unknown11; // 0x34
		uint32_t unknown12; // 0x38
		uint32_t unknown13; // 0x3c
		vec3f position;     // 0x40
		vec3f rotation;     // 0x4c
		uint32_t unknown14; // 0x58
		uint32_t unknown15; // 0x5c
		uint32_t unknown16; // 0x60
		uint32_t unknown17; // 0x64
		uint32_t unknown18; // 0x68
		uint32_t unknown19; // 0x6c
		uint32_t unknown20; // 0x70
		uint32_t unknown21; // 0x74
		uint32_t unknown22; // 0x78
		uint32_t unknown23; // 0x7c
		uint32_t unknown24; // 0x80
		uint32_t unknown25; // 0x84
	)
}

std::unique_ptr<level_impl> import_level(stream& level_file);

uint32_t locate_main_level_segment(stream& level_file);

#endif
