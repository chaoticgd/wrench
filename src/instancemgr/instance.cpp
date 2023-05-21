/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include <instancemgr/wtf_util.h>
#include <instancemgr/instances.h>

const glm::mat4& TransformComponent::matrix() const {
	return _matrix;
}

const glm::mat3x4& TransformComponent::inverse_matrix() const {
	return _inverse_matrix;
}

const glm::vec3& TransformComponent::pos() const {
	return *(glm::vec3*) &_matrix[3][0];
}

const glm::vec3& TransformComponent::rot() const {
	return _rot;
}

const f32& TransformComponent::scale() const {
	return _scale;
}

void TransformComponent::set_from_matrix(const glm::mat4& new_matrix, const glm::mat3x4* new_inverse_matrix, const glm::vec3* new_rot) {
	_matrix = new_matrix;
	if(new_inverse_matrix) {
		_inverse_matrix = *new_inverse_matrix;
	} else {
		_inverse_matrix = glm::inverse(_matrix);
	}
	glm::vec3 scale = {0.f, 0.f, 0.f};
	glm::quat orientation = {0.f, 0.f, 0.f, 0.f};
	glm::vec3 translation = {0.f, 0.f, 0.f};
	glm::vec3 skew = {0.f, 0.f, 0.f};
	glm::vec4 perspective = {0.f, 0.f, 0.f, 0.f};
	glm::decompose(_matrix, scale, orientation, translation, skew, perspective);
	if(new_rot) {
		_rot = *new_rot;
	} else {
		_rot = glm::eulerAngles(orientation);
	}
	_scale = (scale[0] + scale[1] + scale[2]) / 3.f;
}

void TransformComponent::set_from_pos_rot_scale(const glm::vec3& pos, const glm::vec3& rot, f32 scale) {
	_matrix = glm::mat4(1.f);
	_matrix = glm::translate(_matrix, pos);
	_matrix = glm::scale(_matrix, glm::vec3(scale));
	_matrix = glm::rotate(_matrix, _rot.z, glm::vec3(0.f, 0.f, 1.f));
	_matrix = glm::rotate(_matrix, _rot.y, glm::vec3(0.f, 1.f, 0.f));
	_matrix = glm::rotate(_matrix, _rot.x, glm::vec3(1.f, 0.f, 0.f));
	_inverse_matrix = glm::inverse(_matrix);
	_rot = rot;
	_scale = scale;
}

void TransformComponent::read(const WtfNode* src) {
	switch(_mode) {
		case TransformMode::NONE: {
			break;
		}
		case TransformMode::MATRIX: {
			glm::mat4 matrix = read_inst_float_list<glm::mat4>(src, "matrix");
			set_from_matrix(matrix);
			break;
		}
		case TransformMode::MATRIX_AND_INVERSE: {
			glm::mat4 matrix = read_inst_float_list<glm::mat4>(src, "matrix");
			glm::mat3x4 inverse_matrix = read_inst_float_list<glm::mat3x4>(src, "inverse_matrix");
			set_from_matrix(matrix, &inverse_matrix);
			break;
		}
		case TransformMode::MATRIX_INVERSE_ROTATION: {
			glm::mat4 matrix = read_inst_float_list<glm::mat4>(src, "matrix");
			glm::mat3x4 inverse_matrix = read_inst_float_list<glm::mat3x4>(src, "inverse_matrix");
			glm::vec3 rot = read_inst_float_list<glm::vec3>(src, "rot");
			set_from_matrix(matrix, &inverse_matrix, &rot);
			break;
		}
		case TransformMode::POSITION: {
			glm::vec3 pos = read_inst_float_list<glm::vec3>(src, "pos");
			glm::vec3 rot = {0.f, 0.f, 0.f};
			f32 scale = 1.f;
			set_from_pos_rot_scale(pos, rot, scale);
			break;
		}
		case TransformMode::POSITION_ROTATION: {
			glm::vec3 pos = read_inst_float_list<glm::vec3>(src, "pos");
			glm::vec3 rot = read_inst_float_list<glm::vec3>(src, "rot");
			f32 scale = 1.f;
			set_from_pos_rot_scale(pos, rot, scale);
			break;
		}
		case TransformMode::POSITION_ROTATION_SCALE: {
			glm::vec3 pos = read_inst_float_list<glm::vec3>(src, "pos");
			glm::vec3 rot = read_inst_float_list<glm::vec3>(src, "rot");
			f32 scale = read_inst_float(src, "scale");
			set_from_pos_rot_scale(pos, rot, scale);
			break;
		}
	}
}

void TransformComponent::write(WtfWriter* dest) const {
	switch(_mode) {
		case TransformMode::NONE: {
			break;
		}
		case TransformMode::MATRIX: {
			write_inst_float_list(dest, "matrix", matrix());
			break;
		}
		case TransformMode::MATRIX_AND_INVERSE: {
			write_inst_float_list(dest, "matrix", matrix());
			write_inst_float_list(dest, "inverse_matrix", inverse_matrix());
			break;
		}
		case TransformMode::MATRIX_INVERSE_ROTATION: {
			write_inst_float_list(dest, "matrix", matrix());
			write_inst_float_list(dest, "inverse_matrix", inverse_matrix());
			write_inst_float_list(dest, "rot", rot());
			break;
		}
		case TransformMode::POSITION: {
			write_inst_float_list(dest, "pos", pos());
			break;
		}
		case TransformMode::POSITION_ROTATION: {
			write_inst_float_list(dest, "pos", pos());
			write_inst_float_list(dest, "rot", rot());
			break;
		}
		case TransformMode::POSITION_ROTATION_SCALE: {
			write_inst_float_list(dest, "pos", pos());
			write_inst_float_list(dest, "rot", rot());
			wtf_write_float_attribute(dest, "scale", scale());
			break;
		}
	}
}

const TransformComponent& Instance::transform() const {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform;
}

TransformComponent& Instance::transform() {
	verify_fatal(_components_mask & COM_TRANSFORM);
	return _transform;
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

const glm::vec3& Instance::colour() const {
	verify_fatal(_components_mask & COM_COLOUR);
	return _colour;
}

glm::vec3& Instance::colour() {
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

const CameraCollisionParams& Instance::camera_collision() const {
	verify_fatal(_components_mask & COM_CAMERA_COLLISION);
	return _camera_collision;
}

CameraCollisionParams& Instance::camera_collision() {
	verify_fatal(_components_mask & COM_CAMERA_COLLISION);
	return _camera_collision;
}

void Instance::read_common(const WtfNode* src) {
	if(has_component(COM_TRANSFORM)) {
		transform().read(src);
	}
	
	if(has_component(COM_PVARS)) {
		pvars() = read_inst_byte_list(src, "pvars");
	}
	
	if(has_component(COM_COLOUR)) {
		colour() = read_inst_float_list<glm::vec3>(src, "col");
	}
	
	if(has_component(COM_DRAW_DISTANCE)) {
		draw_distance() = read_inst_float(src, "draw_dist");
	}
	
	if(has_component(COM_SPLINE)) {
		std::vector<glm::vec4>& points = spline();
		points.clear();
		const WtfAttribute* attrib = wtf_attribute_of_type(src, "spline", WTF_ARRAY);
		verify(attrib, "Missing 'spline' attribute.");
		for(WtfAttribute* vector_attrib = attrib->first_array_element; vector_attrib != nullptr; vector_attrib = vector_attrib->next) {
			verify(vector_attrib->type == WTF_ARRAY, "Invalid 'spline' attribute.");
			float vector[4];
			s32 i = 0;
			for(WtfAttribute* number_attrib = vector_attrib->first_array_element; number_attrib != nullptr; number_attrib = number_attrib->next) {
				verify(number_attrib->type == WTF_NUMBER && i < 4, "Invalid 'spline' attribute.");
				vector[i++] = number_attrib->number.f;
			}
			points.emplace_back(glm::vec4(vector[0], vector[1], vector[2], vector[3]));
		}
	}
	
	if(has_component(COM_BOUNDING_SPHERE)) {
		bounding_sphere() = read_inst_float_list<glm::vec4>(src, "bsphere");
	}
}

void Instance::begin_write(WtfWriter* dest) const {
	wtf_begin_node(dest, instance_type_to_string(type()), std::to_string(id().value).c_str());
	
	if(has_component(COM_TRANSFORM)) {
		transform().write(dest);
	}
	
	if(has_component(COM_PVARS)) {
		write_inst_byte_list(dest, "pvars", pvars());
	}
	
	if(has_component(COM_COLOUR)) {
		write_inst_float_list(dest, "col", colour());
	}
	
	if(has_component(COM_DRAW_DISTANCE)) {
		wtf_write_float_attribute(dest, "draw_dist", draw_distance());
	}
	
	if(has_component(COM_SPLINE)) {
		wtf_begin_attribute(dest, "spline");
		wtf_begin_array(dest);
		for(const glm::vec4& vec : spline()) {
			wtf_write_floats(dest, &vec.x, 4);
		}
		wtf_end_array(dest);
		wtf_end_attribute(dest);
	}
	
	if(has_component(COM_BOUNDING_SPHERE)) {
		write_inst_float_list(dest, "bsphere", bounding_sphere());
	}
}

void Instance::end_write(WtfWriter* dest) const {
	wtf_end_node(dest);
}

#define GENERATED_INSTANCE_READ_WRITE_FUNCS
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_READ_WRITE_FUNCS

#define GENERATED_INSTANCE_TYPE_TO_STRING_FUNC
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_TYPE_TO_STRING_FUNC
