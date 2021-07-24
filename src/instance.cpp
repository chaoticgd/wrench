/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "instance.h"

#include <glm/gtx/matrix_decompose.hpp>

void Instance::set_transform(glm::mat4 matrix, glm::mat3x4* inverse) {
	assert(_components_mask & COM_TRANSFORM);
	_transform.matrix = matrix;
	if(inverse == nullptr) {
		_transform.inverse_matrix = glm::inverse(matrix);
	} else {
		_transform.inverse_matrix = *inverse;
	}
	glm::vec3 scale;
	glm::quat orientation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, orientation, translation, skew, perspective);
	_transform.rotation = glm::eulerAngles(orientation);
	if(_transform_mode == TransformMode::POSITION_ROTATION_SCALE) {
		_transform.scale = (scale[0] + scale[1] + scale[2]) / 3.f;
	} else {
		_transform.scale = 1.f;
	}
}

void Instance::set_transform(glm::mat4 matrix, glm::mat3x4 inverse, glm::vec3 rotation) {
	assert(_components_mask & COM_TRANSFORM);
	_transform.matrix = matrix;
	_transform.inverse_matrix = inverse;
	_transform.rotation = rotation;
	if(_transform_mode == TransformMode::POSITION_ROTATION_SCALE) {
		glm::vec3 scale;
		glm::quat orientation;
		glm::vec3 translation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(matrix, scale, orientation, translation, skew, perspective);
		_transform.scale = (scale[0] + scale[1] + scale[2]) / 3.f;
	} else {
		_transform.scale = 1.f;
	}
}

void Instance::set_transform(glm::vec3 position, glm::vec3 rotation, f32 scale) {
	assert(_components_mask & COM_TRANSFORM);
	glm::mat4 matrix(1);
	matrix = glm::translate(matrix, position);
	matrix = glm::scale(matrix, glm::vec3(scale));
	matrix = glm::rotate(matrix, rotation.x, glm::vec3(1, 0, 0));
	matrix = glm::rotate(matrix, rotation.y, glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, rotation.z, glm::vec3(0, 0, 1));
	_transform.matrix = matrix;
	_transform.inverse_matrix = glm::inverse(matrix);
	_transform.rotation = rotation;
	_transform.scale = scale;
}

const std::vector<u8>& Instance::pvars() const { 
	assert(_components_mask & COM_PVARS);
	return _pvars;
}

std::vector<u8>& Instance::pvars() { 
	assert(_components_mask & COM_PVARS);
	return _pvars;
}

s32 Instance::temp_pvar_index() const { 
	assert(_components_mask & COM_PVARS);
	return _pvar_index;
}

s32& Instance::temp_pvar_index() { 
	assert(_components_mask & COM_PVARS);
	return _pvar_index;
}

const GlobalPvarPointers& Instance::temp_global_pvar_pointers() const { 
	assert(_components_mask & COM_PVARS);
	return _global_pvar_pointers;
}

GlobalPvarPointers& Instance::temp_global_pvar_pointers() { 
	assert(_components_mask & COM_PVARS);
	return _global_pvar_pointers;
}

const Colour& Instance::colour() const { 
	assert(_components_mask & COM_COLOUR);
	return _colour;
}

Colour& Instance::colour() { 
	assert(_components_mask & COM_COLOUR);
	return _colour;
}

const f32& Instance::draw_distance() const { 
	assert(_components_mask & COM_DRAW_DISTANCE);
	return _draw_distance;
}

f32& Instance::draw_distance() { 
	assert(_components_mask & COM_DRAW_DISTANCE);
	return _draw_distance;
}

const std::vector<glm::vec4>& Instance::path() const { 
	assert(_components_mask & COM_PATH);
	return _path;
}

std::vector<glm::vec4>& Instance::path() { 
	assert(_components_mask & COM_PATH);
	return _path;
}

const glm::vec4& Instance::bounding_sphere() const { 
	assert(_components_mask & COM_BOUNDING_SPHERE);
	return _bounding_sphere;
}

glm::vec4& Instance::bounding_sphere() { 
	assert(_components_mask & COM_BOUNDING_SPHERE);
	return _bounding_sphere;
}
