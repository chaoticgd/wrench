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

#ifndef FORMATS_LEVEL_STREAM_H
#define FORMATS_LEVEL_STREAM_H

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
	secondary_header {
		texture_header {
			... uncompressed textures ...
		}
	}
	???
	ram_image_wad
	???
	moby_wad: wad(
		level_header
		???
		some strings
		???
		moby_table
		???
	)
	???

	where entries in curly brackets are pointed to by a header, entries in
	wad(...) are within a compressed segment.
*/

class texture_provider_fmt_header {};

class level_stream : public level, public proxy_stream {
public:
	level_stream(stream* iso_file, uint32_t level_offset, uint32_t level_size);

	void populate(app* a) override;

	std::vector<const point_object*> point_objects() const override;

	std::map<uint32_t, moby*> mobies() override;
	std::map<uint32_t, const moby*> mobies() const override;

	// Used by the inspector.
	template <typename... T>
	void reflect(T... callbacks);

	struct fmt {
		struct master_header;
		struct secondary_header;
		struct moby_segment_header;
		
		struct ship_data;
		struct directional_light_table;
		struct string_table;
		struct moby_table;

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
			uint32_t unknown1;                  // 0x0
			uint32_t unknown2;                  // 0x4
			file_ptr<texture_provider_fmt_header> textures; // 0x8
			uint32_t texture_segment_size;      // 0xc
			uint32_t unknown4;                  // 0x10
			uint32_t unknown5;                  // 0x14
			uint32_t unknown6;                  // 0x18
			uint32_t unknown7;                  // 0x1c
			uint32_t unknown8;                  // 0x20
			uint32_t unknown9;                  // 0x24
			uint32_t unknown10;                 // 0x28
			uint32_t unknown11;                 // 0x2c
			uint32_t unknown12;                 // 0x30
			uint32_t unknown13;                 // 0x34
			uint32_t unknown14;                 // 0x38
			uint32_t unknown15;                 // 0x3c
			uint32_t unknown16;                 // 0x40
			uint32_t unknown17;                 // 0x44
			file_ptr<wad_header> ram_image_wad; // 0x48
		)

		packed_struct(moby_segment_header,
			file_ptr<ship_data> ship;                             // 0x0
			file_ptr<directional_light_table> directional_lights; // 0x4
			uint32_t unknown1;                                    // 0x8
			uint32_t unknown2;                                    // 0xc
			file_ptr<string_table> english_strings;               // 0x10
			uint32_t unknown3; // Points to 16 bytes between the English and French tables (on Barlow).
			file_ptr<string_table> french_strings;                // 0x18
			file_ptr<string_table> german_strings;                // 0x1c
			file_ptr<string_table> spanish_strings;               // 0x20
			file_ptr<string_table> italian_strings;               // 0x24
			file_ptr<string_table> null_strings;                  // 0x28 Also what is this thing?
			uint32_t unknown4;                                    // 0x2c
			uint32_t unknown5;                                    // 0x30
			uint32_t unknown6;                                    // 0x34
			uint32_t unknown7;                                    // 0x38
			uint32_t unknown8;                                    // 0x3c
			uint32_t unknown9;                                    // 0x40
			uint32_t unknown10;                                   // 0x44
			uint32_t unknown11;                                   // 0x48
			file_ptr<moby_table> mobies;                          // 0x4c
		)
	};

private:
	uint32_t locate_moby_wad();
	uint32_t locate_secondary_header(fmt::master_header header, uint32_t moby_wad_offset);

	std::unique_ptr<wad_stream> _moby_segment_stream;
};

template <typename... T>
void level_stream::reflect(T... callbacks) {
	
}

#endif
