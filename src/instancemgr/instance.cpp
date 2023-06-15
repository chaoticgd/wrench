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

#include <instancemgr/wtf_glue.h>
#include <instancemgr/instances.h>

const glm::mat4& TransformComponent::matrix() const {
	return _matrix;
}

const glm::mat4& TransformComponent::inverse_matrix() const {
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

void TransformComponent::set_from_matrix(const glm::mat4* new_matrix, const glm::mat4* new_inverse_matrix, const glm::vec3* new_rot) {
	verify_fatal(new_matrix || new_inverse_matrix);
	if(new_matrix) {
		_matrix = *new_matrix;
	} else {
		_matrix = glm::inverse(*new_inverse_matrix);
	}
	if(new_inverse_matrix) {
		_inverse_matrix = *new_inverse_matrix;
	} else {
		_inverse_matrix = glm::inverse(*new_matrix);
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
	_matrix = glm::rotate(_matrix, rot.z, glm::vec3(0.f, 0.f, 1.f));
	_matrix = glm::rotate(_matrix, rot.y, glm::vec3(0.f, 1.f, 0.f));
	_matrix = glm::rotate(_matrix, rot.x, glm::vec3(1.f, 0.f, 0.f));
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
			glm::mat4 matrix;
			read_inst_field(matrix, src, "matrix");
			set_from_matrix(&matrix);
			break;
		}
		case TransformMode::MATRIX_INVERSE: {
			glm::mat4 inverse_matrix;
			read_inst_field(inverse_matrix, src, "inverse_matrix");
			set_from_matrix(nullptr, &inverse_matrix);
			break;
		}
		case TransformMode::MATRIX_AND_INVERSE: {
			glm::mat4 matrix;
			glm::mat4 inverse_matrix;
			read_inst_field(matrix, src, "matrix");
			read_inst_field(inverse_matrix, src, "inverse_matrix");
			set_from_matrix(&matrix, &inverse_matrix);
			break;
		}
		case TransformMode::MATRIX_INVERSE_ROTATION: {
			glm::mat4 matrix;
			glm::mat4 inverse_matrix;
			glm::vec3 rot;
			read_inst_field(matrix, src, "matrix");
			read_inst_field(inverse_matrix, src, "inverse_matrix");
			read_inst_field(rot, src, "rot");
			set_from_matrix(&matrix, &inverse_matrix, &rot);
			break;
		}
		case TransformMode::POSITION: {
			glm::vec3 pos;
			glm::vec3 rot = {0.f, 0.f, 0.f};
			f32 scale = 1.f;
			read_inst_field(pos, src, "pos");
			set_from_pos_rot_scale(pos, rot, scale);
			break;
		}
		case TransformMode::POSITION_ROTATION: {
			glm::vec3 pos;
			glm::vec3 rot;
			f32 scale = 1.f;
			read_inst_field(pos, src, "pos");
			read_inst_field(rot, src, "rot");
			set_from_pos_rot_scale(pos, rot, scale);
			break;
		}
		case TransformMode::POSITION_ROTATION_SCALE: {
			glm::vec3 pos;
			glm::vec3 rot;
			f32 scale;
			read_inst_field(pos, src, "pos");
			read_inst_field(rot, src, "rot");
			read_inst_field(scale, src, "scale");
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
			write_inst_field(dest, "matrix", matrix());
			break;
		}
		case TransformMode::MATRIX_INVERSE: {
			write_inst_field(dest, "inverse_matrix", inverse_matrix());
			break;
		}
		case TransformMode::MATRIX_AND_INVERSE: {
			write_inst_field(dest, "matrix", matrix());
			write_inst_field(dest, "inverse_matrix", inverse_matrix());
			break;
		}
		case TransformMode::MATRIX_INVERSE_ROTATION: {
			write_inst_field(dest, "matrix", matrix());
			write_inst_field(dest, "inverse_matrix", inverse_matrix());
			write_inst_field(dest, "rot", rot());
			break;
		}
		case TransformMode::POSITION: {
			write_inst_field(dest, "pos", pos());
			break;
		}
		case TransformMode::POSITION_ROTATION: {
			write_inst_field(dest, "pos", pos());
			write_inst_field(dest, "rot", rot());
			break;
		}
		case TransformMode::POSITION_ROTATION_SCALE: {
			write_inst_field(dest, "pos", pos());
			write_inst_field(dest, "rot", rot());
			write_inst_field(dest, "scale", scale());
			break;
		}
	}
}

void PvarComponent::read(const WtfNode* src) {
	read_inst_field(data, src, "pvars");
	
	const WtfAttribute* shared_data_pointers_attrib = wtf_attribute_of_type(src, "shared_data_pointers", WTF_ARRAY);
	if(shared_data_pointers_attrib) {
		for(const WtfAttribute* attrib = shared_data_pointers_attrib->first_array_element; attrib != nullptr; attrib = attrib->next) {
			verify(attrib->type == WTF_ARRAY, "Bad shared data pointers list on moby instance.");
			WtfAttribute* pointer_offset = attrib->first_array_element;
			verify(pointer_offset && pointer_offset->type == WTF_NUMBER, "Bad shared data pointers list on moby instance.");
			WtfAttribute* shared_data_id = pointer_offset->next;
			verify(shared_data_id && shared_data_id->type == WTF_NUMBER, "Bad shared data pointers list on moby instance.");
			shared_data_pointers.emplace_back(pointer_offset->number.i, shared_data_id->number.i);
		}
	}
}

void PvarComponent::write(WtfWriter* dest) const {
	write_inst_field(dest, "pvars", data);
	
	if(!shared_data_pointers.empty()) {
		wtf_begin_attribute(dest, "shared_data_pointers");
		wtf_begin_array(dest);
		for(auto& [pointer, shared_data_id] : shared_data_pointers) {
			wtf_begin_array(dest);
			wtf_write_integer(dest, pointer);
			wtf_write_integer(dest, shared_data_id);
			wtf_end_array(dest);
		}
		wtf_end_array(dest);
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

s32 Instance::o_class() const {
	verify_fatal(_components_mask & COM_CLASS);
	return _o_class;
}

s32& Instance::o_class() {
	verify_fatal(_components_mask & COM_CLASS);
	return _o_class;
}

const PvarComponent& Instance::pvars() const {
	verify_fatal(_components_mask & COM_PVARS);
	return _pvars;
}

PvarComponent& Instance::pvars() {
	verify_fatal(_components_mask & COM_PVARS);
	return _pvars;
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
	
	if(has_component(COM_CLASS)) {
		read_inst_field(o_class(), src, "class");
	}
	
	if(has_component(COM_PVARS)) {
		pvars().read(src);
	}
	
	if(has_component(COM_COLOUR)) {
		read_inst_field(colour(), src, "col");
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
		read_inst_field(bounding_sphere(), src, "bsphere");
	}
}

void Instance::begin_write(WtfWriter* dest) const {
	wtf_begin_node(dest, instance_type_to_string(type()), std::to_string(id().value).c_str());
	
	if(has_component(COM_TRANSFORM)) {
		transform().write(dest);
	}
	
	if(has_component(COM_CLASS)) {
		write_inst_field(dest, "class", o_class());
	}
	
	if(has_component(COM_PVARS)) {
		pvars().write(dest);
	}
	
	if(has_component(COM_COLOUR)) {
		write_inst_field(dest, "col", colour());
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
		write_inst_field(dest, "bsphere", bounding_sphere());
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
