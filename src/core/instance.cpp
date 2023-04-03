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
	verify_fatal(_components_mask & COM_TRANSFORM);
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
	verify_fatal(_components_mask & COM_TRANSFORM);
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
	verify_fatal(_components_mask & COM_TRANSFORM);
	glm::mat4 matrix(1);
	matrix = glm::translate(matrix, position);
	matrix = glm::scale(matrix, glm::vec3(scale));
	matrix = glm::rotate(matrix, rotation.z, glm::vec3(0, 0, 1));
	matrix = glm::rotate(matrix, rotation.y, glm::vec3(0, 1, 0));
	matrix = glm::rotate(matrix, rotation.x, glm::vec3(1, 0, 0));
	_transform.matrix = matrix;
	_transform.inverse_matrix = glm::inverse(matrix);
	_transform.rotation = rotation;
	_transform.scale = scale;
}

glm::mat4 Instance::matrix() const {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform.matrix;
}

glm::mat3x4 Instance::inverse_matrix() const {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform.inverse_matrix;
}
 
glm::vec3 Instance::position() const {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform.matrix[3];
}

void Instance::set_position(glm::vec3 position) {
	verify_fatal(_components_mask & COM_TRANSFORM);
	_transform.matrix[3] = glm::vec4(position, 1.f);
}

glm::vec3 Instance::rotation() const {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform.rotation;
}

void Instance::set_rotation(glm::vec3 rotation) {
	verify_fatal(_components_mask & COM_TRANSFORM);
	_transform.matrix = glm::rotate(_transform.matrix, -_transform.rotation.x, glm::vec3(1, 0, 0));
	_transform.matrix = glm::rotate(_transform.matrix, -_transform.rotation.y, glm::vec3(0, 1, 0));
	_transform.matrix = glm::rotate(_transform.matrix, -_transform.rotation.z, glm::vec3(0, 0, 1));
	_transform.rotation = rotation;
	_transform.matrix = glm::rotate(_transform.matrix, _transform.rotation.z, glm::vec3(0, 0, 1));
	_transform.matrix = glm::rotate(_transform.matrix, _transform.rotation.y, glm::vec3(0, 1, 0));
	_transform.matrix = glm::rotate(_transform.matrix, _transform.rotation.x, glm::vec3(1, 0, 0));
	_transform.inverse_matrix = glm::inverse(_transform.matrix);
}

f32 Instance::scale() const {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform.scale;
}

void Instance::set_scale(f32 scale) {
	verify_fatal(_components_mask & COM_TRANSFORM);
	_transform.scale = scale;
}

f32& Instance::m33_value_do_not_use() {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform.m33;
}

const std::vector<u8>& Instance::pvars() const {
	verify_fatal(_components_mask & COM_PVARS);
	return _pvars;
}

std::vector<u8>& Instance::pvars() {
	verify_fatal(_components_mask & COM_PVARS);
	return _pvars;
}

s32 Instance::temp_pvar_index() const {
	verify_fatal(_components_mask & COM_PVARS);
	return _pvar_index;
}

s32& Instance::temp_pvar_index() {
	verify_fatal(_components_mask & COM_PVARS);
	return _pvar_index;
}

const GlobalPvarPointers& Instance::temp_global_pvar_pointers() const {
	verify_fatal(_components_mask & COM_PVARS);
	return _global_pvar_pointers;
}

GlobalPvarPointers& Instance::temp_global_pvar_pointers() {
	verify_fatal(_components_mask & COM_PVARS);
	return _global_pvar_pointers;
}

const Colour& Instance::colour() const {
	verify_fatal(_components_mask & COM_COLOUR);
	return _colour;
}

Colour& Instance::colour() {
	verify_fatal(_components_mask & COM_COLOUR);
	return _colour;
}

f32 Instance::draw_distance() const {
	verify_fatal(_components_mask & COM_DRAW_DISTANCE);
	return _draw_distance;
}

f32& Instance::draw_distance() {
	verify_fatal(_components_mask & COM_DRAW_DISTANCE);
	return _draw_distance;
}

const std::vector<glm::vec4>& Instance::spline() const {
	verify_fatal(_components_mask & COM_SPLINE);
	return _spline;
}

std::vector<glm::vec4>& Instance::spline() {
	verify_fatal(_components_mask & COM_SPLINE);
	return _spline;
}

const glm::vec4& Instance::bounding_sphere() const {
	verify_fatal(_components_mask & COM_BOUNDING_SPHERE);
	return _bounding_sphere;
}

glm::vec4& Instance::bounding_sphere() {
	verify_fatal(_components_mask & COM_BOUNDING_SPHERE);
	return _bounding_sphere;
}
