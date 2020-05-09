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
#include <glm/gtc/matrix_transform.hpp>

#include "../stream.h"
#include "game_model.h"

# /*
# 	Defines the types of game objects the game uses (ties, shrubs, mobies and
# 	splines) as PODs.
# */

// *****************************************************************************
// Basic types
// *****************************************************************************

// A racmat is a 4x4 matrix where the
// last index is used for something else
packed_struct(racmat,
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

	racmat() {}

	racmat(glm::mat4 mat) {
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
	}

	glm::mat4 operator()() const {
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
		result[3][3] = 1;
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

	glm::vec3 operator()() const {
		glm::vec3 result;
		result.x = x;
		result.y = y;
		result.z = z;
		return result;
	}
	
	bool operator==(const vec3f& rhs) const {
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}
	
	bool operator!=(const vec3f& rhs) const {
		return x != rhs.x || y != rhs.y || z != rhs.z;
	}
)

struct level_primary_header;
struct level_asset_header;
struct level_texture_entry;

// *****************************************************************************
// Outer level structures
// *****************************************************************************

packed_struct(level_file_header,
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
packed_struct(level_primary_header,
	uint32_t unknown_0;           // 0x0
	uint32_t unknown_4;           // 0x4
	uint32_t asset_header;        // 0x8
	uint32_t asset_header_size;   // 0xc
	uint32_t tex_pixel_data_base; // 0x10
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
packed_struct(level_asset_header,
	uint32_t num_textures; // 0x0
	file_ptr<level_texture_entry> textures; // 0x4
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
	uint32_t terrain_texture_offset; // 0x34 Relative to asset header.
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

packed_struct(level_texture_entry,
	uint32_t ptr;
	uint16_t width;
	uint16_t height;
	uint32_t palette;
	uint32_t field_c;
);

// *****************************************************************************
// World segment structures
// *****************************************************************************

packed_struct(world_header,
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

packed_struct(world_string_table_header,
	uint32_t num_strings;
	uint32_t unknown;
	// String table entries follow.
)

packed_struct(world_string_table_entry,
	file_ptr<char*> string; // Relative to this struct.
	uint32_t id;
	uint32_t padding[2];
)

packed_struct(world_object_table,
	uint32_t num_elements;
	uint32_t pad[3];
	// Elements follow.
)

packed_struct(world_spline_table_header,
	uint32_t num_splines;
	uint32_t data_offset;
	uint32_t unknown_08;
	uint32_t unknown_0c;
)

packed_struct(world_spline_entry,
	uint32_t num_vertices;
	uint32_t pad[3];
)

packed_struct(world_ship_data,
	uint32_t unknown1[0xf];
	vec3f position;
	float rotationZ;
)

packed_struct(world_directional_light_table,
	uint32_t num_directional_lights; // Max 0xb.
	// Directional lights follow.
)

packed_struct(world_directional_light,
	uint8_t unknown[64];
)

packed_struct(tie,
	uint32_t unknown_0;  // 0x0
	uint32_t unknown_4;  // 0x4
	uint32_t unknown_8;  // 0x8
	uint32_t unknown_c;  // 0xc
	racmat mat; //0x10
	uint32_t unknown_4c; // 0x4c
	uint32_t unknown_50; // 0x50
	int32_t  uid;        // 0x54
	uint32_t unknown_58; // 0x58
	uint32_t unknown_5c; // 0x5c

	void set_translation(glm::vec3 trans) {
		mat.m41 = trans.x;
		mat.m42 = trans.y;
		mat.m43 = trans.z;
	}

	void translate(glm::vec3 trans) {
		mat = glm::translate(mat(), trans);
	}

	void rotate(glm::vec3 rot) {
		mat = glm::rotate(mat(), rot.x, glm::vec3(1, 0, 1));
		mat = glm::rotate(mat(), rot.y, glm::vec3(0, 1, 1));
		mat = glm::rotate(mat(), rot.z, glm::vec3(0, 0, 1));
	}
)

packed_struct(shrub,
	uint32_t unknown_0;  // 0x0
	uint32_t unknown_4;  // 0x4
	uint32_t unknown_8;  // 0x8
	uint32_t unknown_c;  // 0xc
	racmat mat; //0x10
	uint32_t unknown_4c; // 0x4c
	uint32_t unknown_50; // 0x50
	uint32_t unknown_54; // 0x54
	uint32_t unknown_58; // 0x58
	uint32_t unknown_5c; // 0x5c
	uint32_t unknown_60; // 0x60
	uint32_t unknown_64; // 0x64
	uint32_t unknown_68; // 0x68
	uint32_t unknown_6c; // 0x6c

	void set_translation(glm::vec3 trans) {
		mat.m41 = trans.x;
		mat.m42 = trans.y;
		mat.m43 = trans.z;
	}

	void translate(glm::vec3 trans) {
		mat = glm::translate(mat(), trans);
	}

	void rotate(glm::vec3 rot) {
		mat = glm::rotate(mat(), rot.x, glm::vec3(1, 0, 1));
		mat = glm::rotate(mat(), rot.y, glm::vec3(0, 1, 1));
		mat = glm::rotate(mat(), rot.z, glm::vec3(0, 0, 1));
	}
)

packed_struct(moby,
	uint32_t size; // 0x0 Always 0x88?
	uint32_t unknown_4;  // 0x4
	uint32_t unknown_8;  // 0x8
	uint32_t unknown_c;  // 0xc
	int32_t  uid;    	// 0x10
	uint32_t unknown_14; // 0x14
	uint32_t unknown_18; // 0x18
	uint32_t unknown_1c; // 0x1c
	uint32_t unknown_20; // 0x20
	uint32_t unknown_24; // 0x24
	uint32_t class_num;  // 0x28
	uint32_t unknown_2c; // 0x2c
	uint32_t unknown_30; // 0x30
	uint32_t unknown_34; // 0x34
	uint32_t unknown_38; // 0x38
	uint32_t unknown_3c; // 0x3c
	vec3f    position;   // 0x40
	vec3f    rotation;   // 0x4c
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

	glm::mat4 mat() {
		glm::mat4 model_matrix = glm::translate(glm::mat4(1.f), position());
		model_matrix = glm::rotate(model_matrix, rotation.x, glm::vec3(1, 0, 0));
		model_matrix = glm::rotate(model_matrix, rotation.y, glm::vec3(0, 1, 0));
		model_matrix = glm::rotate(model_matrix, rotation.z, glm::vec3(0, 0, 1));
		return model_matrix;
	}

	void set_translation(glm::vec3 trans) {
		position = trans;
	}

	void translate(glm::vec3 trans) {
		position = position() + trans;
	}

	void rotate(glm::vec3 rot) {
		rotation = rotation() + rot;
	}
)

struct spline {
	std::vector<glm::vec3> points;	
	
	glm::mat4 mat() {
		if(points.size() < 1) {
			return glm::mat4(1.f);
		}
		return glm::translate(glm::mat4(1.f), points[0]);
	}

	void set_translation(glm::vec3 trans) {
		// TODO
	}

	void translate(glm::vec3 trans) {
		// TODO
	}

	void rotate(glm::vec3 rot) {
		// TODO
	}
};

#endif
