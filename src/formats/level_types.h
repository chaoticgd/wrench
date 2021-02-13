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

#ifndef FORMATS_LEVEL_TYPES_H
#define FORMATS_LEVEL_TYPES_H

#include <variant>
#include <optional>
#include <stdint.h>
#include <glm/glm.hpp>

#include "../stream.h"

# /*
# 	Defines the types that make up the level format, including game objects.
# */

// *****************************************************************************
// Basic types
// *****************************************************************************

packed_struct(mat4f,
	float m11 = 0;
	float m12 = 0;
	float m13 = 0;
	float m14 = 0;
	float m21 = 0;
	float m22 = 0;
	float m23 = 0;
	float m24 = 0;
	float m31 = 0;
	float m32 = 0;
	float m33 = 0;
	float m34 = 0;
	float m41 = 0;
	float m42 = 0;
	float m43 = 0;
	float m44 = 0; // Always equal to 0.01f (in the game files)?

	mat4f() {}

	mat4f(glm::mat4 mat) {
		m11 = mat[0][0];
		m12 = mat[0][1];
		m13 = mat[0][2];
		m14 = mat[0][3];
		m21 = mat[1][0];
		m22 = mat[1][1];
		m23 = mat[1][2];
		m24 = mat[1][3];
		m31 = mat[2][0];
		m32 = mat[2][1];
		m33 = mat[2][2];
		m34 = mat[2][3];
		m41 = mat[3][0];
		m42 = mat[3][1];
		m43 = mat[3][2];
		m44 = mat[3][3];
	}

	operator glm::mat4() const {
		glm::mat4 result;
		result[0][0] = m11;
		result[0][1] = m12;
		result[0][2] = m13;
		result[0][3] = m14;
		result[1][0] = m21;
		result[1][1] = m22;
		result[1][2] = m23;
		result[1][3] = m24;
		result[2][0] = m31;
		result[2][1] = m32;
		result[2][2] = m33;
		result[2][3] = m34;
		result[3][0] = m41;
		result[3][1] = m42;
		result[3][2] = m43;
		result[3][3] = m44;
		return result;
	}
)

packed_struct(vec3f,
	float x = 0;
	float y = 0;
	float z = 0;

	vec3f() {}

	vec3f(glm::vec3 vec) {
		x = vec.x;
		y = vec.y;
		z = vec.z;
	}

	operator glm::vec3() const {
		glm::vec3 result;
		result.x = x;
		result.y = y;
		result.z = z;
		return result;
	}
	
	glm::vec3 operator()() const {
		return static_cast<glm::vec3>(*this);
	}
	
	bool operator==(const vec3f& rhs) const {
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
	
	bool operator!=(const vec3f& rhs) const {
		return x != rhs.x || y != rhs.y || z != rhs.z;
	}
)

packed_struct(colour48,
	uint32_t red;
	uint32_t green;
	uint32_t blue;
)

struct level_primary_header;
struct level_asset_header;
struct level_texture_entry;

// *****************************************************************************
// Outer level structures
// *****************************************************************************

packed_struct(sector_range,
	sector32 offset;
	sector32 size;
)

packed_struct(byte_range,
	uint32_t offset;
	uint32_t size;
)

// These are also present in the table of contents for GC, UYA and DL.
packed_struct(level_file_header_rac23,
	uint32_t magic;              // 0x0 Equal to 0x60.
	sector32 base_offset;        // 0x4
	uint32_t level_number;       // 0x8
	uint32_t unknown_c;          // 0xc
	sector_range primary_header; // 0x10
	sector_range sound_bank_1;   // 0x18
	sector_range world_segment;  // 0x20
	sector_range unknown_28;     // 0x28
	sector_range unknown_30;     // 0x30
	sector_range unknown_38;     // 0x38
	uint32_t unknown_40;         // 0x40
	uint32_t unknown_44;         // 0x44
	sector_range sound_bank_2;   // 0x48
	sector_range sound_bank_3;   // 0x50
)

packed_struct(level_file_header_rac2_68,
	uint32_t magic;              // 0x0 Equal to 0x68.
	sector32 base_offset;        // 0x4
	uint32_t level_number;       // 0x8
	sector_range primary_header; // 0xc
	sector32 unknown_14;         // 0x14
	uint32_t unknown_18;         // 0x18
	sector_range world_segment;  // 0x1c
)

packed_struct(level_file_header_rac4,
	uint32_t magic;          // 0x0 Equal to 0xc68.
	sector32 base_offset;    // 0x4
	uint32_t level_number;   // 0x8
	uint32_t unknown_c;      // 0xc
	uint32_t unknown_10;     // 0x10
	uint32_t unknown_14;     // 0x14
	sector_range primary_header; // 0x18
	uint32_t unknown_20;     // 0x20
	uint32_t unknown_24;     // 0x24
	uint32_t unknown_28;     // 0x28
	uint32_t unknown_2c;     // 0x2c
	uint32_t unknown_30;     // 0x30
	uint32_t unknown_34;     // 0x34
	uint32_t unknown_38;     // 0x38
	uint32_t unknown_3c;     // 0x3c
	uint32_t unknown_40;     // 0x40
	uint32_t unknown_44;     // 0x44
	uint32_t unknown_48;     // 0x48
	uint32_t unknown_4c;     // 0x4c
	uint32_t unknown_50;     // 0x50
	uint32_t unknown_54;     // 0x54
	sector_range world_segment; // 0x58
)

// Pointers are relative to this header.
packed_struct(level_primary_header_rac23,
	uint32_t code_segment_offset; // 0x0
	uint32_t code_segment_size;   // 0x4
	uint32_t asset_header;        // 0x8
	uint32_t asset_header_size;   // 0xc
	uint32_t tex_pixel_data_base; // 0x10
	uint32_t unknown_14;          // 0x14
	uint32_t hud_header_offset;   // 0x18
	uint32_t unknown_1c;          // 0x1c
	uint32_t hud_bank_0_offset;   // 0x20
	uint32_t hud_bank_0_size;     // 0x24
	uint32_t hud_bank_1_offset;   // 0x28
	uint32_t hud_bank_1_size;     // 0x2c
	uint32_t hud_bank_2_offset;   // 0x30
	uint32_t hud_bank_2_size;     // 0x34
	uint32_t hud_bank_3_offset;   // 0x38
	uint32_t hud_bank_3_size;     // 0x3c
	uint32_t hud_bank_4_offset;   // 0x40
	uint32_t hud_bank_4_size;     // 0x44
	uint32_t asset_wad;           // 0x48
	uint32_t unknown_4c;          // 0x4c
	uint32_t loading_screen_textures_offset; // 0x50
	uint32_t loading_screen_textures_size;   // 0x54
)

packed_struct(level_primary_header_rac4,
	uint32_t unknown_0;           // 0x0
	uint32_t unknown_4;           // 0x4
	uint32_t code_segment_offset; // 0x8
	uint32_t code_segment_size;   // 0xc
	uint32_t asset_header;        // 0x10
	uint32_t asset_header_size;   // 0x14
	uint32_t tex_pixel_data_base; // 0x18
	uint32_t unknown_14;          // 0x1c
	uint32_t hud_header_offset;   // 0x20
	uint32_t unknown_1c;          // 0x24
	uint32_t hud_bank_0_offset;   // 0x28
	uint32_t hud_bank_0_size;     // 0x2c
	uint32_t hud_bank_1_offset;   // 0x30
	uint32_t hud_bank_1_size;     // 0x34
	uint32_t hud_bank_2_offset;   // 0x38
	uint32_t hud_bank_2_size;     // 0x3c
	uint32_t hud_bank_3_offset;   // 0x40
	uint32_t hud_bank_3_size;     // 0x44
	uint32_t hud_bank_4_offset;   // 0x48
	uint32_t hud_bank_4_size;     // 0x4c
	uint32_t asset_wad;           // 0x50
	uint32_t unknown_54;          // 0x54
	uint32_t loading_screen_textures_offset; // 0x58
	uint32_t loading_screen_textures_size;   // 0x5c
	
)

packed_struct(level_code_segment_header,
	uint32_t base_address; // 0x0 Where to load it in RAM.
	uint32_t unknown_4;
	uint32_t unknown_8;
	uint32_t entry_offset; // 0xc The address of the main_loop function, relative to base_address.
	// Code segment immediately follows.
)

// Barlow 0x418200
packed_struct(level_asset_header,
	uint32_t mipmap_count;           // 0x0
	uint32_t mipmap_offset;          // 0x4
	uint32_t tfrag_geomtry;          // 0x8
	uint32_t occlusion;              // 0xc
	uint32_t sky;                    // 0x10
	uint32_t collision;              // 0x14
	uint32_t moby_model_count;       // 0x18
	uint32_t moby_model_offset;      // 0x1c
	uint32_t unknown_20;             // 0x20
	uint32_t unknown_24;             // 0x24
	uint32_t unknown_28;             // 0x28
	uint32_t unknown_2c;             // 0x2c
	uint32_t terrain_texture_count;  // 0x30
	uint32_t terrain_texture_offset; // 0x34 Relative to asset header.
	uint32_t moby_texture_count;     // 0x38
	uint32_t moby_texture_offset;    // 0x3c
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
	uint32_t ptr_into_asset_wad_68;  // 0x68
	uint32_t rel_to_asset_header_6c; // 0x6c
	uint32_t rel_to_asset_header_70; // 0x70
	uint32_t unknown_74; // 0x74
	uint32_t rel_to_asset_header_78; // 0x78
	uint32_t unknown_7c; // 0x7c
	uint32_t index_into_some1_texs; // 0x80
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

packed_struct(level_moby_model_entry,
	uint32_t offset_in_asset_wad;
	uint32_t o_class;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint8_t textures[16];
)

packed_struct(level_mipmap_entry,
	uint32_t unknown_0; // Type?
	uint16_t width;
	uint16_t height;
	uint32_t offset_1;
	uint32_t offset_2; // Duplicate of offset_1?
)

packed_struct(level_texture_entry,
	uint32_t ptr;
	uint16_t width;
	uint16_t height;
	uint16_t unknown_8;
	uint16_t palette;
	uint32_t field_c;
)

packed_struct(level_hud_header,
	uint32_t unknown_0;  // 0x0
	uint32_t unknown_4;  // 0x4
	uint32_t unknown_8;  // 0x8
	uint32_t unknown_c;  // 0xc
	uint32_t unknown_10; // 0x10
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
	uint32_t unknown_48; // 0x48
	uint32_t unknown_4c; // 0x4c
	uint32_t unknown_50; // 0x50
	uint32_t bank_0;     // 0x54
	uint32_t unknown_58; // 0x58
	uint32_t bank_2;     // 0x5c
	uint32_t bank_3;     // 0x60
	uint32_t bank_4;     // 0x64
)

// *****************************************************************************
// World segment structures
// *****************************************************************************

packed_struct(world_header_rac23,
	uint32_t properties;         // 0x0
	uint32_t directional_lights; // 0x4
	uint32_t unknown_8;          // 0x8
	uint32_t unknown_c;          // 0xc
	uint32_t us_english_strings; // 0x10
	uint32_t uk_english_strings; // 0x14
	uint32_t french_strings;     // 0x18
	uint32_t german_strings;     // 0x1c
	uint32_t spanish_strings;    // 0x20
	uint32_t italian_strings;    // 0x24
	uint32_t japanese_strings;   // 0x28
	uint32_t korean_strings;     // 0x2c
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
	uint32_t pvar_table;         // 0x5c
	uint32_t pvar_data;          // 0x60
	uint32_t unknown_64;         // 0x64
	uint32_t triggers;           // 0x68
	uint32_t unknown_6c;         // 0x6c
	uint32_t unknown_70;         // 0x70
	uint32_t unknown_74;         // 0x74
	uint32_t splines;            // 0x78
	uint32_t grindrails;         // 0x7c
	uint32_t unknown_80;         // 0x80
	uint32_t unknown_84;         // 0x84
	uint32_t unknown_88;         // 0x88
	uint32_t unknown_8c;         // 0x8c
	uint32_t unknown_90;         // 0x90
	uint32_t unknown_94;         // 0x94
	uint32_t unknown_98;         // 0x98
)

packed_struct(world_header_rac4,
	uint32_t properties;         // 0x0
	uint32_t unknown_4;          // 0x4 -- rac3 0x08
	uint32_t unknown_8;          // 0x8 -- rac3 0x0c
	uint32_t us_english_strings; // 0xc
	uint32_t uk_english_strings; // 0x10
	uint32_t french_strings;     // 0x14
	uint32_t german_strings;     // 0x18
	uint32_t spanish_strings;    // 0x1c
	uint32_t italian_strings;    // 0x20
	uint32_t japanese_strings;   // 0x24
	uint32_t korean_strings;     // 0x28
	uint32_t unknown_2c;         // 0x2c -- rac3 0x48
	uint32_t mobies;             // 0x30
	uint32_t unknown_34;         // 0x34
	uint32_t unknown_38;         // 0x38
	uint32_t unknown_3c;         // 0x3c
	uint32_t pvar_table;         // 0x40
	uint32_t pvar_data;          // 0x44
	uint32_t unknown_48;         // 0x48 -- rac3 0x64
	uint32_t triggers;	         // 0x4c
	uint32_t unknown_50;         // 0x50 -- rac3 0x6c
	uint32_t unknown_54;         // 0x54 -- rac3 0x70
	uint32_t unknown_58;         // 0x58 -- rac3 0x74
	uint32_t splines;            // 0x5c
	uint32_t grindrails;         // 0x60
	uint32_t unknown_64;         // 0x64 -- rac3 0x80?
	uint32_t unknown_68;         // 0x68
	uint32_t unknown_6c;         // 0x6c -- rac3 0x88
	uint32_t unknown_70;         // 0x70 -- rac3 0x8c
	uint32_t unknown_74;         // 0x74 -- rac3 0x98
)

packed_struct(world_properties,
	uint32_t unknown_0;     // 0x0
	uint32_t unknown_4;     // 0x4
	uint32_t unknown_8;     // 0x8
	colour48 fog_colour;    // 0xc
	uint32_t unknown_18;    // 0x18
	uint32_t unknown_1c;    // 0x1c
	float fog_distance;     // 0x20
	uint32_t unknown_24;    // 0x24
	uint32_t death_plane_z; // 0x28
	uint32_t unknown_2c;    // 0x2c
	uint32_t unknown_30;    // 0x30
	uint32_t unknown_34;    // 0x34
	uint32_t unknown_38;    // 0x38
	vec3f ship_position;    // 0x3c
	float ship_rotation_z;  // 0x48
	uint32_t unknown_4c;    // 0x4c
	uint32_t unknown_50;    // 0x50
	uint32_t unknown_54;    // 0x54
	uint32_t unknown_58;    // 0x58
	uint32_t unknown_5c;    // 0x5c
)

packed_struct(world_property_thing,
	uint32_t unknown_0;
	uint32_t unknown_4;
	uint32_t count;
	uint32_t unknown_c;
	uint32_t unknown_10;
	uint32_t unknown_14;
	uint32_t unknown_18;
	uint32_t unknown_1c;
)

packed_struct(world_object_table,
	uint32_t count_1;
	uint32_t count_2;
	uint32_t count_3;
	uint32_t pad;
	// Elements follow.
)

packed_struct(world_directional_light,
	uint8_t unknown[0x40];
)

packed_struct(world_thing_8,
	uint8_t unknown[0x20];
)

packed_struct(world_thing_c,
	uint8_t unknown[0x90];
)

packed_struct(world_string_table_header,
	uint32_t num_strings;
	uint32_t size; // Size of table + string data.
	// String table entries follow.
)

packed_struct(world_string_table_entry,
	file_ptr<char*> string; // Relative to this struct.
	uint32_t id;
	uint32_t secondary_id; // Usually -1.
	uint16_t unknown_c; // Always zero for R&C2.
	uint16_t unknown_e; // Always zero for R&C2.
)

packed_struct(world_thing_14,
	uint8_t unknown[0x10];
)

packed_struct(world_tie,
	uint32_t unknown_0;      // 0x0
	uint32_t unknown_4;      // 0x4
	uint32_t unknown_8;      // 0x8
	uint32_t unknown_c;      // 0xc
	mat4f    local_to_world; // 0x10
	uint32_t unknown_50;     // 0x50
	int32_t  uid;            // 0x54
	uint32_t unknown_58;     // 0x58
	uint32_t unknown_5c;     // 0x5c
)

packed_struct(world_shrub,
	uint32_t unknown_0;      // 0x0
	float    unknown_4;      // 0x4
	uint32_t unknown_8;      // 0x8
	uint32_t unknown_c;      // 0xc
	mat4f    local_to_world; // 0x10
	uint32_t unknown_50;     // 0x50
	uint32_t unknown_54;     // 0x54
	uint32_t unknown_58;     // 0x58
	uint32_t unknown_5c;     // 0x5c
	uint32_t unknown_60;     // 0x60
	uint32_t unknown_64;     // 0x64
	uint32_t unknown_68;     // 0x68
	uint32_t unknown_6c;     // 0x6c
)

packed_struct(world_moby_rac23,
	uint32_t size;       // 0x0 Always 0x88.
	int32_t unknown_4;  // 0x4
	uint32_t unknown_8;  // 0x8
	uint32_t unknown_c;  // 0xc
	int32_t  uid;        // 0x10
	uint32_t unknown_14; // 0x14
	uint32_t unknown_18; // 0x18
	uint32_t unknown_1c; // 0x1c
	uint32_t unknown_20; // 0x20
	uint32_t unknown_24; // 0x24
	uint32_t class_num;  // 0x28
	float    scale;      // 0x2c
	uint32_t unknown_30; // 0x30
	uint32_t unknown_34; // 0x34
	uint32_t unknown_38; // 0x38
	uint32_t unknown_3c; // 0x3c
	vec3f    position;   // 0x40
	vec3f    rotation;   // 0x4c
	int32_t unknown_58; // 0x58
	uint32_t unknown_5c; // 0x5c
	uint32_t unknown_60; // 0x60
	uint32_t unknown_64; // 0x64
	int32_t  pvar_index; // 0x68
	uint32_t unknown_6c; // 0x6c
	uint32_t unknown_70; // 0x70
	uint32_t unknown_74; // 0x74
	uint32_t unknown_78; // 0x78
	uint32_t unknown_7c; // 0x7c
	uint32_t unknown_80; // 0x80
	int32_t unknown_84; // 0x84
)

packed_struct(world_moby_rac4,
	uint32_t size;       // 0x0 Always 0x70.
	uint32_t unknown_4;  // 0x4
	int32_t  uid;        // 0x8
	uint32_t unknown_c;  // 0xc
	uint32_t class_num;  // 0x10
	float    scale;      // 0x14
	uint32_t unknown_18; // 0x18
	uint32_t unknown_1c; // 0x1c
	uint32_t unknown_20; // 0x20
	uint32_t unknown_24; // 0x24
	vec3f    position;   // 0x28
	vec3f    rotation;   // 0x34
	uint32_t unknown_40; // 0x40
	uint32_t unknown_44; // 0x44
	uint32_t unknown_48; // 0x48
	uint32_t unknown_4c; // 0x4c
	uint32_t pvar_index; // 0x50
	uint32_t unknown_54; // 0x54
	uint32_t unknown_58; // 0x58
	uint32_t unknown_5c; // 0x5c
	uint32_t unknown_60; // 0x60
	uint32_t unknown_64; // 0x64
	uint32_t unknown_68; // 0x68
	uint32_t unknown_6c; // 0x6c
)

packed_struct(world_thing_58,
	int32_t unknown_0;
	int32_t unknown_4;
)

packed_struct(pvar_table_entry,
	uint32_t offset;
	uint32_t size;
)

packed_struct(world_thing_64,
	uint8_t unknown[0x8];
)

packed_struct(world_trigger,
	mat4f mat1;
	mat4f mat2;
)

packed_struct(world_thing_6c,
	mat4f mat1;
	mat4f mat2;
)

packed_struct(world_thing_70,
	mat4f mat1;
	mat4f mat2;
)

packed_struct(world_spline_table,
	uint32_t spline_count;
	uint32_t data_offset;
	uint32_t data_size;
	uint32_t pad;
)

packed_struct(world_vertex_header,
	uint32_t vertex_count;
	uint32_t pad[3];
)

packed_struct(world_grindrail_header,
	uint32_t count;
	uint32_t part_2_data_offset;
	uint32_t part_2_data_size;
	uint32_t pad;
)

packed_struct(world_grindrail_part_1,
	float x;
	float y;
	float z;
	float w;
	uint8_t unknown_10[0x10];
)

packed_struct(world_thing_84,
	uint8_t unknown[0x90];
)

packed_struct(world_thing_8c,
	uint8_t unknown[0x20];
)

packed_struct(world_thing_90_header,
	uint32_t count_1;
	uint32_t count_2;
	uint32_t count_3;
	uint32_t pad;
)

packed_struct(world_thing_90,
	uint32_t unknown_0;
	uint32_t unknown_4;
)

packed_struct(world_thing_98_header,
	uint32_t size; // Not including this field.
	uint32_t part_1_count;
	uint32_t part_offsets[5];
	uint32_t unknown_1c;
	uint32_t unknown_20;
)

packed_struct(world_thing_98,
	vec3f    position;   // 0x0
	float 	 unknown_c;  // 0xc
	uint16_t counts[6];  // 0x10 -- only 5 are used, last one is padding
	uint32_t offsets[5]; // 0x1c
)

#endif
