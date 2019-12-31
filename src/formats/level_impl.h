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
#include <variant>
#include <stdint.h>

#include "../util.h"
#include "../stream.h"
#include "../worker_logger.h"
#include "wad.h"
#include "racpak.h"
#include "level_types.h"
#include "texture_impl.h"

# /*
#	Read LEVEL*.WAD files.
# */

class game_world {
public:
	struct fmt {
		packed_struct(header,
			uint32_t ship;               // 0x0
			uint32_t directional_lights; // 0x4
			uint32_t unknown_08;         // 0x8
			uint32_t unknown_0c;         // 0xc
			uint32_t english_strings;    // 0x10
			uint32_t unknown_14;         // 0x14 Points to 16 bytes between the English and French tables (on Barlow).
			uint32_t french_strings;     // 0x18
			uint32_t german_strings;     // 0x1c
			uint32_t spanish_strings;    // 0x20
			uint32_t italian_strings;    // 0x24
			uint32_t null_strings;       // 0x28 Also what is this thing?
			uint32_t unknown_2c;         // 0x2c
			uint32_t unknown_30;         // 0x30
			uint32_t ties;               // 0x34
			uint32_t unknown_38;         // 0x38
			uint32_t unknown_3c;         // 0x3c
			uint32_t shrubs;             // 0x40
			uint32_t unknown_44;         // 0x44
			uint32_t unknown_48;         // 0x48
			uint32_t mobies;             // 0x4c
			uint32_t unknown_50;         // 0x50
			uint32_t unknown_54;         // 0x54
			uint32_t unknown_58;         // 0x58
			uint32_t unknown_5c;         // 0x5c
			uint32_t unknown_60;         // 0x60
			uint32_t unknown_64;         // 0x64
			uint32_t unknown_68;         // 0x68
			uint32_t unknown_6c;         // 0x6c
			uint32_t unknown_70;         // 0x70
			uint32_t unknown_74;         // 0x74
			uint32_t splines;            // 0x78
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
		
		packed_struct(object_table,
			uint32_t num_elements;
			uint32_t pad[3];
			// Elements follow.
		)
		
		packed_struct(spline_table_header,
			uint32_t num_splines;
			uint32_t data_offset;
			uint32_t unknown_08;
			uint32_t unknown_0c;
		)

		packed_struct(spline_entry,
			uint32_t num_vertices;
			uint32_t pad[3];
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
	};
	
	struct game_string {
		int id;
		std::string content;	
	};
	
	std::vector<object_id> selection;
	
	game_world() {}
	
	bool is_selected(object_id id) const;
	
	template <typename T>
	T& object_at(std::size_t index) {
		return objects_of_type<T>()[index];
	}

// Iterate over each point object. Needs to be a macro since the callback needs
// to take different types as arguments depending on the type of object being
// handled. Kinda hacky, but it works. (;
#define FOR_EACH_POINT_OBJECT(world, callback) \
	for(std::size_t i = 0; i < world.count<tie>(); i++) { \
		callback(object_id { object_type::TIE, i }, world.object_at<tie>(i)); \
	} \
	for(std::size_t i = 0; i < world.count<shrub>(); i++) { \
		callback(object_id { object_type::SHRUB, i }, world.object_at<shrub>(i)); \
	} \
	for(std::size_t i = 0; i < world.count<moby>(); i++) { \
		callback(object_id { object_type::MOBY, i }, world.object_at<moby>(i)); \
	}
	
	template <typename T>
	std::size_t count() {
		return objects_of_type<T>().size();
	}
	
	template <typename T>
	void for_each(std::function<void(std::size_t i, T&)> callback) {
		auto& objects = objects_of_type<T>();
		for(std::size_t i = 0; i < objects.size(); i++) {
			callback(i, objects[i]);
		}
	}
	
	void read(stream* src);
	void write();
	
private:
	template <typename T>
	std::vector<T>& objects_of_type() {
		if constexpr(std::is_same_v<T, tie>) return _ties;
		if constexpr(std::is_same_v<T, shrub>) return _shrubs;
		if constexpr(std::is_same_v<T, moby>) return _mobies;
		if constexpr(std::is_same_v<T, spline>) return _splines;
		static_assert("Invalid object type.");
	}

	std::map<std::string, std::vector<game_string>> _languages;
	std::vector<tie> _ties;
	std::vector<shrub> _shrubs;
	std::vector<moby> _mobies;
	std::vector<spline> _splines;
};

class level {
public:
	struct fmt {
		struct primary_header;
		struct secondary_header;

		packed_struct(file_header,
			uint32_t header_size;    // 0x0
			uint32_t unknown_4;      // 0x4
			uint32_t unknown_8;      // 0x8
			uint32_t unknown_c;      // 0xc
			sector32 primary_header; // 0x10
			uint32_t unknown_14;     // 0x14
			sector32 unknown_18;     // 0x18
			uint32_t unknown_1c;     // 0x1c
			sector32 moby_segment;   // 0x20
		)

		// Pointers are relative to this header.
		packed_struct(primary_header,
			uint32_t unknown_0;                    // 0x0
			uint32_t unknown_4;                    // 0x4
			file_ptr<secondary_header> snd_header; // 0x8
			uint32_t texture_segment_size;         // 0xc
			uint32_t tex_pixel_data_base;          // 0x10
			uint32_t unknown_14; // 0x14
			uint32_t unknown_18; // 0x18
			uint32_t unknown_1c; // 0x1c
			uint32_t unknown_20; // 0x20
			uint32_t unknown_24; // 0x24
			uint32_t unknown_28; // 0x28
			uint32_t unknown_2c; // 0x2c
			uint32_t unknown_30; // 0x30
			uint32_t unknown_34; // 0x34
			uint32_t unknown_38; // 0x38
			uint32_t unknown_3c; // 0x3c
			uint32_t unknown_40; // 0x40
			uint32_t unknown_44; // 0x44
			file_ptr<wad_header> ram_image_wad; // 0x48
		)
		
		packed_struct(secondary_header,
			uint32_t num_textures; // 0x0
			file_ptr<level_texture_provider::fmt::texture_entry> textures; // 0x4
			uint32_t unknown_8;  // 0x8
			uint32_t unknown_c;  // 0xc
			uint32_t unknown_10; // 0x10
			uint32_t unknown_14; // 0x14
			uint32_t unknown_18; // 0x18
			uint32_t models;     // 0x1c
			uint32_t unknown_20; // 0x20
			uint32_t unknown_24; // 0x24
			uint32_t unknown_28; // 0x28
			uint32_t unknown_2c; // 0x2c
			uint32_t unknown_30; // 0x30
			uint32_t unknown_34; // 0x34
			uint32_t unknown_38; // 0x38
			uint32_t unknown_3c; // 0x3c
			uint32_t unknown_40; // 0x40
			uint32_t unknown_44; // 0x44
			uint32_t unknown_48; // 0x48
			uint32_t unknown_4c; // 0x4c
			uint32_t unknown_50; // 0x50
			uint32_t unknown_54; // 0x54
			uint32_t unknown_58; // 0x58
			uint32_t unknown_5c; // 0x5c
			uint32_t unknown_60; // 0x60
			uint32_t unknown_64; // 0x64
			uint32_t unknown_68; // 0x68
			uint32_t unknown_6c; // 0x6c
			uint32_t unknown_70; // 0x70
			uint32_t unknown_74; // 0x74
			uint32_t unknown_78; // 0x78
			uint32_t unknown_7c; // 0x7c
			uint32_t unknown_80; // 0x80
			uint32_t unknown_84; // 0x84
		)
	};

	level(iso_stream* iso, std::size_t offset, std::size_t size, std::string display_name);

	texture_provider* get_texture_provider();

	std::map<std::string, std::map<uint32_t, std::string>> game_strings() { return {}; }

	stream* moby_stream();

	const std::size_t offset; // The base offset of the file_header in the ISO file.
	game_world world;
	
private:
	proxy_stream _backing;
	level_texture_provider _textures;
	stream* _moby_stream;
};

#endif
