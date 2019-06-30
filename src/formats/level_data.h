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
#include "../renderer.h"
#include "../worker_thread.h"
#include "wad.h"

/*
	LEVEL*.WAD LAYOUT
	=================

	master_header
	secondary_header
	???
	ram_image_wad - Overwrites most of RAM when decompressed.
	???
	moby_wad {
		level_header
		???
		some strings
		???
		moby_table
		???
	}

	where entries in curly brackets are stored compressed within WAD segments.
*/

namespace level_data {

	std::unique_ptr<level_impl> import_level(stream& level_file, worker_logger& log);

	uint32_t locate_moby_wad(stream& level_file);
	void import_moby_wad(level_impl& lvl, stream& moby_wad);

	void import_ram_image_wad(level_impl& lvl, stream& wad);

	struct outer_header;

	namespace moby_wad {
		struct header;
		struct string_table;
		struct string_table_entry;
		struct moby_table;
		struct moby;
	}

	packed_struct(master_header,
		uint8_t unknown1[0x14];              // 0x0
		// The offset between the secondary header and the moby WAD is
		// (secondary_moby_offset_part * 0x800 + 0xfff) & 0xfffffffffffff000.
		uint32_t secondary_moby_offset_part; // 0x14
		uint8_t unknown2[0xc];               // 0x18
		// The offset between something and the moby WAD is
		// moby_wad_offset_part * 0x800 + 0xfffU & 0xfffff000
		uint32_t moby_wad_offset_part;       // 0x28
	)

	// Pointers are relative to this header.
	packed_struct(secondary_header,
		uint8_t unknown[0x48];   // 0x14
		file_ptr<wad_header> ram_image_wad; // 0x48
	)

	packed_struct(vec3f,
		float x;
		float y;
		float z;
	)

	namespace ram_image {
		// Where the image is decompressed to in memory.
		const uint32_t BASE_OFFSET = 0x854000;

		// Relative to the beginning of RAM.
	}

	namespace moby_wad {
		packed_struct(header,
			uint8_t unknown1[0x10];
			file_ptr<string_table> english_strings; // 0x10
			uint32_t unknown2; // Points to 16 bytes between the English and French tables (on Barlow).
			file_ptr<string_table> french_strings;  // 0x18
			file_ptr<string_table> german_strings;  // 0x1c
			file_ptr<string_table> spanish_strings; // 0x20
			file_ptr<string_table> italian_strings; // 0x24
			file_ptr<string_table> null_strings;    // 0x28 Also what is this thing?
			uint32_t unknown3;                      // 0x2c
			uint32_t unknown4;                      // 0x30
			uint32_t unknown5;                      // 0x34
			uint32_t unknown6;                      // 0x38
			uint32_t unknown7;                      // 0x3c
			uint32_t unknown8;                      // 0x40
			uint32_t unknown9;                      // 0x44
			uint32_t unknown10;                     // 0x48
			file_ptr<moby_table> mobies;            // 0x4c
		)

		packed_struct(string_table,
			uint32_t num_strings;
			uint32_t unknown;
			// String table entries follow.
		)

		packed_struct(string_table_entry,
			file_ptr<char*> string; // Relative to this struct.
			uint32_t id;
			uint32_t padding[2];
		)

		packed_struct(moby_table,
			uint32_t num_mobies;
			uint32_t unknown[3];
			// Mobies follow.
		)

		packed_struct(moby,
			uint32_t size;      // 0x0 Always 0x88?
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
}

#endif
