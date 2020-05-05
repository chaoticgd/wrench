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
#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../stream.h"
#include "game_model.h"

# /*
# 	Defines the types of game objects the game uses (ties, shrubs, mobies and
# 	splines) as PODs.
# */

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

enum class object_type {
	TIE = 1, SHRUB = 2, MOBY = 3, SPLINE = 4	
};

struct object_id {
	object_type type;
	std::size_t index;
	
	bool operator<(const object_id& rhs) const {
		if(type < rhs.type) {
			return true;
		} else if(type > rhs.type) {
			return false;
		} else {
			return index < rhs.index;
		}
	}
	
	bool operator==(const object_id& rhs) const {
		return type == rhs.type && index == rhs.index;
	}
};

struct object_list {
	std::vector<std::size_t> ties;
	std::vector<std::size_t> shrubs;
	std::vector<std::size_t> mobies;
	std::vector<std::size_t> splines;
	
	void add(object_id id) {
		switch(id.type) {
			case object_type::TIE: ties.push_back(id.index); break;
			case object_type::SHRUB: shrubs.push_back(id.index); break;
			case object_type::MOBY: mobies.push_back(id.index); break;
			case object_type::SPLINE: splines.push_back(id.index); break;
		}
	}
	
	std::size_t size() const {
		return ties.size() + shrubs.size() + mobies.size() + splines.size();
	}
	
	bool contains(object_id id) const {
		switch(id.type) {
			case object_type::TIE: return std::find(ties.begin(), ties.end(), id.index) != ties.end(); break;
			case object_type::SHRUB: return std::find(shrubs.begin(), shrubs.end(), id.index) != shrubs.end(); break;
			case object_type::MOBY: return std::find(mobies.begin(), mobies.end(), id.index) != mobies.end(); break;
			case object_type::SPLINE: return std::find(splines.begin(), splines.end(), id.index) != splines.end(); break;
		}
	}
	
	object_id first() {
		if(ties.size() > 0) return object_id{object_type::TIE, ties[0]};
		if(shrubs.size() > 0) return object_id{object_type::SHRUB, shrubs[0]};
		if(mobies.size() > 0) return object_id{object_type::MOBY, mobies[0]};
		if(splines.size() > 0) return object_id{object_type::SPLINE, splines[0]};
		throw std::runtime_error("object_list::first called on an empty object_list. Add an if(x.size() > 0) check.");
	}
};

enum class wrench_result {
	OKAY, NOT_FOUND
};

#endif
