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
		uint32_t id;
		std::string content;	
	};
	
	object_list selection;
	
	game_world() {}
	
	bool is_selected(object_id id) const;
	
	void read(stream* src);
	void write();
	
	// Maps from logical object IDs to physical array indices and vice versa.
	struct object_mappings {
		std::map<object_id, std::size_t> id_to_index;
		std::map<std::size_t, object_id> index_to_id;
	};
	
	template <typename T>
	bool object_exists(object_id id) {
		auto& id_to_index = mappings_of_type<T>().id_to_index;
		return id_to_index.find(id) != id_to_index.end();
	}
	
	template <typename T>
	T& object_from_id(object_id id) {
		object_mappings& mappings = mappings_of_type<T>();
		return objects_of_type<T>()[mappings.id_to_index.at(id)];
	}

	template <typename T>
	std::size_t count() {
		return objects_of_type<T>().size();
	}
		
	struct object_callbacks {
		std::function<void(object_id, tie&)> ties;
		std::function<void(object_id, shrub&)> shrubs;
		std::function<void(object_id, moby&)> mobies;
		std::function<void(object_id, spline&)> splines;
	};
	
	#define find_object_by_id(id, ...) \
		find_object_by_id_impl(id, {__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__})
		
	void find_object_by_id_impl(object_id id, object_callbacks cbs) {
		for_each_object_type([&]<typename T>() {
			if(object_exists<T>(id)) {
				member_of_type<T>(cbs)(id, object_from_id<T>(id));
			}
		});
	}
	
	template <typename T>
	void for_each_object_of_type(std::function<void(object_id, T&)> callback) {
		auto& objects = objects_of_type<T>();
		for(std::size_t i = 0; i < objects.size(); i++) {
			object_id id = mappings_of_type<T>().index_to_id.at(i);
			callback(id, objects[i]);
		}
	}
	
	#define for_each_object(...) \
		for_each_object_impl({__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__})
	
	void for_each_object_impl(object_callbacks cbs) {
		for_each_object_type([&]<typename T>() {
			for_each_object_of_type<T>(member_of_type<T>(cbs));
		});
	}
	
	template <typename T>
	void for_each_object_of_type_in(
			std::vector<object_id> objects, std::function<void(object_id, T&)> callback) {
		for(object_id id : objects) {
			if(object_exists<T>(id)) {
				callback(id, object_from_id<T>(id));
			}
		}
	}
	
	#define for_each_object_in(list, ...) \
		for_each_object_in_impl(list, {__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__})
	
	void for_each_object_in_impl(object_list& list, object_callbacks cbs) {
		for_each_object_type([&]<typename T>() {
			for_each_object_of_type_in<T>
				(member_of_type<T>(list), member_of_type<T>(cbs));
		});
	}
	
private:
	template <typename T>
	std::vector<T>& objects_of_type() {
		return member_of_type<T>(_object_store);
	}
	
	struct {
		std::vector<tie> ties;
		std::vector<shrub> shrubs;
		std::vector<moby> mobies;
		std::vector<spline> splines;
	} _object_store;
	
	template <typename T>
	object_mappings& mappings_of_type() {
		return member_of_type<T>(_object_mappings);
	}
	
	struct {
		object_mappings ties;
		object_mappings shrubs;
		object_mappings mobies;
		object_mappings splines;
	} _object_mappings;
	std::size_t _next_object_id = 1; // ID to assign to the next new object.
	
	std::map<std::string, std::vector<game_string>> _languages;
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
			uint32_t snd_header_size;              // 0xc
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
			file_ptr<wad_header> asset_wad; // 0x48
		)
		
		// Barlow 0x418200
		packed_struct(secondary_header,
			uint32_t num_textures; // 0x0
			file_ptr<level_texture_provider::fmt::texture_entry> textures; // 0x4
			uint32_t ptr_into_asset_wad_8;  // 0x8
			uint32_t ptr_into_asset_wad_c;  // 0xc
			uint32_t ptr_into_asset_wad_10; // 0x10
			uint32_t models_something;      // 0x14
			uint32_t num_models; // 0x18
			uint32_t models;     // 0x1c
			uint32_t unknown_20; // 0x20
			uint32_t unknown_24; // 0x24
			uint32_t unknown_28; // 0x28
			uint32_t unknown_2c; // 0x2c
			uint32_t terrain_texture_count;  // 0x30
			uint32_t terrain_texture_offset; // 0x34 Relative to secondary header.
			uint32_t some1_texture_count;    // 0x38
			uint32_t some1_texture_offset;   // 0x3c
			uint32_t tie_texture_count;      // 0x40
			uint32_t tie_texture_offset;     // 0x44
			uint32_t shrub_texture_count;    // 0x48
			uint32_t shrub_texture_offset;   // 0x4c
			uint32_t some2_texture_count;    // 0x50
			uint32_t some2_texture_offset;   // 0x54
			uint32_t sprite_texture_count;   // 0x58
			uint32_t sprite_texture_offset;  // 0x5c
			uint32_t tex_data_in_asset_wad;  // 0x60
			uint32_t ptr_into_asset_wad_64;  // 0x64
			uint32_t unknown_68; // 0x68
			uint32_t unknown_6c; // 0x6c
			uint32_t unknown_70; // 0x70
			uint32_t unknown_74; // 0x74
			uint32_t unknown_78; // 0x78
			uint32_t unknown_7c; // 0x7c
			uint32_t unknown_80; // 0x80
			uint32_t unknown_84; // 0x84
			uint32_t unknown_88; // 0x88
			uint32_t unknown_8c; // 0x8c
			uint32_t unknown_90; // 0x90
			uint32_t unknown_94; // 0x94
			uint32_t unknown_98; // 0x98
			uint32_t unknown_9c; // 0x9c
			uint32_t unknown_a0; // 0xa0
			uint32_t ptr_into_asset_wad_a4; // 0xa4
			uint32_t unknown_a8; // 0xa8
			uint32_t unknown_ac; // 0xac
			uint32_t ptr_into_asset_wad_b0; // 0xb0
		)
	};

	level(iso_stream* iso, std::size_t offset, std::size_t size, std::string display_name);

	std::map<std::string, std::map<uint32_t, std::string>> game_strings() { return {}; }

	stream* moby_stream();

	const std::size_t offset; // The base offset of the file_header in the ISO file.
	game_world world;
	
	std::map<uint32_t, std::size_t> moby_class_to_model;
	std::vector<game_model> moby_models;
	std::vector<texture> terrain_textures;
	std::vector<texture> tie_textures;
	std::vector<texture> moby_textures;
	std::vector<texture> sprite_textures;
	
private:
	proxy_stream _backing;
	stream* _moby_stream;
};

#endif
