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

class level : public base_level {
public:
	struct fmt {
		struct primary_header;
		struct secondary_header;

		// Pointers are relative to this header.
		packed_struct(primary_header,
			uint32_t unknown1;                     // 0x0
			uint32_t unknown2;                     // 0x4
			file_ptr<secondary_header> snd_header; // 0x8
			uint32_t texture_segment_size;         // 0xc
			uint32_t tex_pixel_data_base;          // 0x10
			uint32_t unknown5;                     // 0x14
			uint32_t unknown6;                     // 0x18
			uint32_t unknown7;                     // 0x1c
			uint32_t unknown8;                     // 0x20
			uint32_t unknown9;                     // 0x24
			uint32_t unknown10;                    // 0x28
			uint32_t unknown11;                    // 0x2c
			uint32_t unknown12;                    // 0x30
			uint32_t unknown13;                    // 0x34
			uint32_t unknown14;                    // 0x38
			uint32_t unknown15;                    // 0x3c
			uint32_t unknown16;                    // 0x40
			uint32_t unknown17;                    // 0x44
			file_ptr<wad_header> ram_image_wad;    // 0x48
		)
		
		packed_struct(secondary_header,
			uint32_t num_textures; // 0x0
			file_ptr<level_texture_provider::fmt::texture_entry> textures; // 0x4
			uint32_t unknown1;     // 0x8
			uint32_t unknown2;     // 0xc
			uint32_t unknown3;     // 0x10
			uint32_t unknown4;     // 0x14
			uint32_t unknown5;     // 0x18
			uint32_t unknown6;     // 0x1c
			uint32_t unknown7;     // 0x20
			uint32_t unknown8;     // 0x24
			uint32_t unknown9;     // 0x28
			uint32_t unknown10;    // 0x2c
			uint32_t unknown11;    // 0x30
			uint32_t unknown12;    // 0x34
			uint32_t unknown13;    // 0x38
			uint32_t unknown14;    // 0x3c
			uint32_t unknown15;    // 0x40
			uint32_t unknown16;    // 0x44
			uint32_t unknown17;    // 0x48
			uint32_t unknown18;    // 0x4c
			uint32_t unknown19;    // 0x50
			uint32_t unknown20;    // 0x54
			uint32_t unknown21;    // 0x58
			uint32_t unknown22;    // 0x5c
			uint32_t unknown23;    // 0x60
			uint32_t unknown24;    // 0x64
			uint32_t unknown25;    // 0x68
			uint32_t unknown26;    // 0x6c
			uint32_t unknown27;    // 0x70
			uint32_t unknown28;    // 0x74
			uint32_t unknown29;    // 0x78
			uint32_t unknown30;    // 0x7c
			uint32_t unknown31;    // 0x80
			uint32_t unknown32;    // 0x84
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
				uint32_t unknown_08;                                  // 0x8
				uint32_t unknown_0c;                                  // 0xc
				file_ptr<string_table_header> english_strings;        // 0x10
				uint32_t unknown_14; // Points to 16 bytes between the English and French tables (on Barlow).
				file_ptr<string_table_header> french_strings;         // 0x18
				file_ptr<string_table_header> german_strings;         // 0x1c
				file_ptr<string_table_header> spanish_strings;        // 0x20
				file_ptr<string_table_header> italian_strings;        // 0x24
				file_ptr<string_table_header> null_strings;           // 0x28 Also what is this thing?
				uint32_t unknown_2c;                                  // 0x2c
				uint32_t unknown_30;                                  // 0x30
				file_ptr<obj_table_header> ties;                      // 0x34
				uint32_t unknown_38;                                  // 0x38
				uint32_t unknown_3c;                                  // 0x3c
				file_ptr<obj_table_header> shrubs;                    // 0x40
				uint32_t unknown_44;                                  // 0x44
				uint32_t unknown_48;                                  // 0x48
				file_ptr<obj_table_header> mobies;                    // 0x4c
				uint32_t unknown_50;                                  // 0x50
				uint32_t unknown_54;                                  // 0x54
				uint32_t unknown_58;                                  // 0x58
				uint32_t unknown_5c;                                  // 0x5c
				uint32_t unknown_60;                                  // 0x60
				uint32_t unknown_64;                                  // 0x64
				uint32_t unknown_68;                                  // 0x68
				uint32_t unknown_6c;                                  // 0x6c
				uint32_t unknown_70;                                  // 0x70
				uint32_t unknown_74;                                  // 0x74
				uint32_t splines;                                     // 0x78
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

	level(iso_stream* iso, racpak* archive, std::string display_name, worker_logger& log);

	texture_provider* get_texture_provider();

	std::size_t num_ties() const;
	std::size_t num_shrubs() const;
	std::size_t num_splines() const;
	std::size_t num_mobies() const;
	
	tie    tie_at(std::size_t i);
	shrub  shrub_at(std::size_t i);
	spline spline_at(std::size_t i);
	moby   moby_at(std::size_t i);
	
	const tie    tie_at(std::size_t i) const;
	const shrub  shrub_at(std::size_t i) const;
	const spline spline_at(std::size_t i) const;
	const moby   moby_at(std::size_t i) const;
	
	void for_each_game_object(std::function<void(game_object*)> callback);
	void for_each_point_object(std::function<void(point_object*)> callback);

	std::map<std::string, std::map<uint32_t, std::string>> game_strings() override;

	stream* moby_stream();

private:
	void read_game_strings(fmt::moby_segment::header header, worker_logger& log);

	racpak* _archive;
	std::optional<level_texture_provider> _textures;
	stream* _moby_stream;
	std::map<std::string, std::map<uint32_t, std::string>> _game_strings;
};

class spline : public game_object {
public:
	spline(stream* backing, std::size_t offset);

	std::size_t base() const override;
	std::vector<glm::vec3> points() const;

private:
	proxy_stream _backing;
	std::size_t _base;
};

#endif
