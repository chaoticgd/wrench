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

#ifndef FORMATS_LEVEL_IMPL_H
#define FORMATS_LEVEL_IMPL_H

#include <memory>
#include <stdint.h>

#include "../util.h"
#include "../level.h"
#include "../stream.h"
#include "../worker_logger.h"
#include "wad.h"
#include "racpak.h"
#include "tie_impl.h"
#include "shrub_impl.h"
#include "moby_impl.h"
#include "texture_impl.h"

# /*
#	A level stored using a stream. The member function wrap read/write calls.
# */

/*
	LEVEL*.WAD LAYOUT
	=================

	racpak index: contents

	0: ???
	1: secondary_header {
		texture_header {
			... uncompressed textures ...
		}
	}
	2: ???
	3: moby_wad: wad(
		level_header
		???
		some strings
		???
		moby_table
		???
	)
	4: ???
	5: ???
	6: ram_image_wad
	7: ???
	8: ???
	9: ???
	10: ???

	where entries in curly brackets are pointed to by a header, entries in
	wad(...) are within a compressed segment.
*/

class spline_impl;

class level_impl : public level {
public:
	struct fmt {
		struct secondary_header;

		// Pointers are relative to this header.
		packed_struct(secondary_header,
			uint32_t unknown1;                  // 0x0
			uint32_t unknown2;                  // 0x4
			file_ptr<level_texture_provider::fmt::header> textures; // 0x8
			uint32_t texture_segment_size;      // 0xc
			uint32_t tex_pixel_data_base;       // 0x10
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

		struct moby_segment {
			struct header;
			struct ship_data;
			struct directional_light_table;
			struct string_table_header;
			struct string_table_entry;
			struct obj_table_header;

			packed_struct(header,
				file_ptr<ship_data> ship;                             // 0x0
				file_ptr<directional_light_table> directional_lights; // 0x4
				uint32_t unknown1;                                    // 0x8
				uint32_t unknown2;                                    // 0xc
				file_ptr<string_table_header> english_strings;        // 0x10
				uint32_t unknown3; // Points to 16 bytes between the English and French tables (on Barlow).
				file_ptr<string_table_header> french_strings;         // 0x18
				file_ptr<string_table_header> german_strings;         // 0x1c
				file_ptr<string_table_header> spanish_strings;        // 0x20
				file_ptr<string_table_header> italian_strings;        // 0x24
				file_ptr<string_table_header> null_strings;           // 0x28 Also what is this thing?
				uint32_t unknown4;                                    // 0x2c
				uint32_t unknown5;                                    // 0x30
				file_ptr<obj_table_header> ties;                      // 0x34
				uint32_t unknown7;                                    // 0x38
				uint32_t unknown8;                                    // 0x3c
				file_ptr<obj_table_header> shrubs;                    // 0x40
				uint32_t splines;                                     // 0x44
				uint32_t unknown11;                                   // 0x48
				file_ptr<obj_table_header> mobies;                    // 0x4c
			)

			packed_struct(ship_data,
				uint32_t unknown1[0xf];
				vec3f position;
				float rotationZ;
			)

			packed_struct(directional_light_table,
				uint32_t num_directional_lights; // Max 0xb.
				// Directional lights follow.
			)

			packed_struct(directional_light,
				uint8_t unknown[64];
			)

			packed_struct(string_table_header,
				uint32_t num_strings;
				uint32_t unknown;
				// String table entries follow.
			)

			packed_struct(string_table_entry,
				file_ptr<char*> string; // Relative to this struct.
				uint32_t id;
				uint32_t padding[2];
			)

			packed_struct(obj_table_header,
				uint32_t num_elements;
				uint32_t pad[3];
				// Elements follow.
			)

			packed_struct(tie_entry,
				uint8_t unknown[0x60];
			)
		};
	};

	level_impl(racpak* archive, std::string display_name, worker_logger& log);

	texture_provider* get_texture_provider();

	std::vector<tie*> ties() override;
	std::vector<shrub*> shrubs() override;
	std::vector<spline*> splines() override;
	std::map<int32_t, moby*> mobies() override;

	std::map<std::string, std::map<uint32_t, std::string>> game_strings() override;

private:
	void read_game_strings(fmt::moby_segment::header header, worker_logger& log);
	void read_ties(fmt::moby_segment::header header, worker_logger& log);
	void read_shrubs(fmt::moby_segment::header header, worker_logger& log);
	void read_splines(fmt::moby_segment::header header, worker_logger& log);
	void read_mobies(fmt::moby_segment::header header, worker_logger& log);

	racpak* _archive;
	std::optional<level_texture_provider> _textures;
	stream* _moby_stream;
	std::vector<std::unique_ptr<tie_impl>> _ties;
	std::vector<std::unique_ptr<shrub_impl>> _shrubs;
	std::vector<std::unique_ptr<spline_impl>> _splines;
	std::vector<std::unique_ptr<moby_impl>> _mobies;
	std::map<std::string, std::map<uint32_t, std::string>> _game_strings;
};

class spline_impl : public spline {
public:
	spline_impl(stream* backing, std::size_t offset, std::size_t size);

	std::vector<glm::vec3> points() const;

private:
	proxy_stream _backing;	
};

#endif
