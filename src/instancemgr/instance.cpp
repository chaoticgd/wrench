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

#include <math.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <instancemgr/wtf_glue.h>
#include <instancemgr/instances.h>

const glm::mat4& TransformComponent::matrix() const
{
	return m_matrix;
}

const glm::mat4& TransformComponent::inverse_matrix() const
{
	return m_inverse_matrix;
}

const glm::vec3& TransformComponent::pos() const
{
	return *(glm::vec3*) &m_matrix[3][0];
}

const glm::vec3& TransformComponent::rot() const
{
	return m_rot;
}

const f32& TransformComponent::scale() const
{
	return m_scale;
}

static void decompose_matrix(glm::mat4& matrix, glm::vec3& pos, glm::vec3& rot, glm::vec3& scale)
{
	scale[0] = glm::length(matrix[0]);
	scale[1] = glm::length(matrix[1]);
	scale[2] = glm::length(matrix[2]);
	
	matrix[0] = glm::normalize(matrix[0]);
	matrix[1] = glm::normalize(matrix[1]);
	matrix[2] = glm::normalize(matrix[2]);
	
	rot[0] = atan2f(matrix[1][2], matrix[2][2]);
	rot[1] = atan2f(-matrix[0][2], sqrtf(matrix[1][2] * matrix[1][2] + matrix[2][2] * matrix[2][2]));
	rot[2] = atan2f(matrix[0][1], matrix[0][0]);
	
	pos[0] = matrix[3].x;
	pos[1] = matrix[3].y;
	pos[2] = matrix[3].z;
}

void TransformComponent::set_from_matrix(
	const glm::mat4* new_matrix, const glm::mat4* new_inverse_matrix, const glm::vec3* new_rot)
{
	glm::mat4 temp_matrix;
	verify_fatal(new_matrix || new_inverse_matrix);
	if (new_matrix) {
		temp_matrix = *new_matrix;
	} else {
		temp_matrix = glm::inverse(*new_inverse_matrix);
	}
	switch (m_mode) {
		case TransformMode::NONE: {
			break;
		}
		case TransformMode::MATRIX:
		case TransformMode::MATRIX_INVERSE:
		case TransformMode::MATRIX_AND_INVERSE:
		case TransformMode::MATRIX_INVERSE_ROTATION: {
			m_matrix = temp_matrix;
			if (new_inverse_matrix) {
				m_inverse_matrix = *new_inverse_matrix;
			} else {
				m_inverse_matrix = glm::inverse(*new_matrix);
			}
			glm::vec3 p, r, s;
			decompose_matrix(temp_matrix, p, r, s);
			if (new_rot) {
				m_rot = *new_rot;
			} else {
				m_rot = r;
			}
			m_scale = (s[0] + s[1] + s[2]) / 3.f;
			break;
		}
		case TransformMode::POSITION: {
			glm::vec3 p, r, s;
			decompose_matrix(temp_matrix, p, r, s);
			set_from_pos_rot_scale(p, glm::vec3(0.f, 0.f, 0.f), 1.f);
			break;
		}
		case TransformMode::POSITION_ROTATION: {
			glm::vec3 p, r, s;
			decompose_matrix(temp_matrix, p, r, s);
			set_from_pos_rot_scale(p, new_rot ? *new_rot : r, 1.f);
			break;
		}
		case TransformMode::POSITION_ROTATION_SCALE: {
			glm::vec3 p, r, s;
			decompose_matrix(temp_matrix, p, r, s);
			set_from_pos_rot_scale(p, new_rot ? *new_rot : r, (s[0] + s[1] + s[2]) / 3.f);
			break;
		}
	}
}

static f32 constrain_angle(f32 angle)
{
	if (angle > -WRENCH_PI && angle < WRENCH_PI) {
		return angle;
	}
	return std::remainder(angle, 2 * WRENCH_PI);
}

void TransformComponent::set_from_pos_rot_scale(const glm::vec3& pos, const glm::vec3& rot, f32 scale)
{
	glm::vec3 rot_wrapped;
	for (s32 i = 0; i < 3; i++) {
		rot_wrapped[i] = constrain_angle(rot[i]);
	}
	
	m_matrix = glm::mat4(1.f);
	m_matrix = glm::translate(m_matrix, pos);
	m_matrix = glm::scale(m_matrix, glm::vec3(scale));
	m_matrix = glm::rotate(m_matrix, rot_wrapped.z, glm::vec3(0.f, 0.f, 1.f));
	m_matrix = glm::rotate(m_matrix, rot_wrapped.y, glm::vec3(0.f, 1.f, 0.f));
	m_matrix = glm::rotate(m_matrix, rot_wrapped.x, glm::vec3(1.f, 0.f, 0.f));
	m_inverse_matrix = glm::inverse(m_matrix);
	m_rot = rot_wrapped;
	m_scale = scale;
}

void TransformComponent::read(const WtfNode* src)
{
	switch (m_mode) {
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

void TransformComponent::write(WtfWriter* dest) const
{
	switch (m_mode) {
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

void PvarComponent::read(const WtfNode* src)
{
	read_inst_field(data, src, "pvars");
	
	const WtfAttribute* relative_pointers_attrib = wtf_attribute_of_type(src, "relative_pvar_pointers", WTF_ARRAY);
	if (relative_pointers_attrib) {
		for (const WtfAttribute* attrib = relative_pointers_attrib->first_array_element; attrib != nullptr; attrib = attrib->next) {
			verify(attrib->type == WTF_NUMBER, "Bad relative pointer list on instance.");
			
			PvarPointer& pointer = pointers.emplace_back();
			pointer.offset = attrib->number.i;
			pointer.type = PvarPointerType::RELATIVE;
		}
	}
	
	const WtfAttribute* shared_data_pointers_attrib = wtf_attribute_of_type(src, "shared_pvar_pointers", WTF_ARRAY);
	if (shared_data_pointers_attrib) {
		for (const WtfAttribute* attrib = shared_data_pointers_attrib->first_array_element; attrib != nullptr; attrib = attrib->next) {
			verify(attrib->type == WTF_ARRAY, "Bad shared data pointers list on moby instance.");
			WtfAttribute* pointer_offset = attrib->first_array_element;
			verify(pointer_offset && pointer_offset->type == WTF_NUMBER, "Bad shared data pointer list on instance.");
			WtfAttribute* shared_data_id = pointer_offset->next;
			verify(shared_data_id && shared_data_id->type == WTF_NUMBER, "Bad shared data pointer list on instance.");
			
			PvarPointer& pointer = pointers.emplace_back();
			pointer.offset = pointer_offset->number.i;
			pointer.type = PvarPointerType::SHARED;
			pointer.shared_data_id = shared_data_id->number.i;
		}
	}
	
	validate();
}

void PvarComponent::validate() const
{
	// Validate uniqueness (this is important for undo/redo integrity).
	std::vector<PvarPointer> pointers_copy = pointers;
	std::sort(BEGIN_END(pointers_copy));
	s32 last_offset = -1;
	for (size_t i = 0; i < pointers_copy.size(); i++) {
		verify_fatal(pointers_copy[i].offset > -1);
		verify_fatal(last_offset == -1 || last_offset < pointers_copy[i].offset);
		last_offset = pointers_copy[i].offset;
	}
}

void PvarComponent::write(WtfWriter* dest) const
{
	write_inst_field(dest, "pvars", data);
	
	if (!pointers.empty()) {
		wtf_begin_attribute(dest, "relative_pvar_pointers");
		wtf_begin_array(dest);
		for (const PvarPointer& pointer : pointers) {
			if (pointer.type == PvarPointerType::RELATIVE) {
				wtf_write_integer(dest, pointer.offset);
			}
		}
		wtf_end_array(dest);
		
		wtf_begin_attribute(dest, "shared_pvar_pointers");
		wtf_begin_array(dest);
		for (const PvarPointer& pointer : pointers) {
			if (pointer.type == PvarPointerType::SHARED) {
				wtf_begin_array(dest);
				wtf_write_integer(dest, pointer.offset);
				wtf_write_integer(dest, pointer.shared_data_id);
				wtf_end_array(dest);
			}
		}
		wtf_end_array(dest);
	}
}

const TransformComponent& Instance::transform() const
{
	verify_fatal(m_components_mask & COM_TRANSFORM);
	return m_transform;
}

TransformComponent& Instance::transform()
{
	verify_fatal(m_components_mask & COM_TRANSFORM);
	return m_transform;
}

s32 Instance::o_class() const
{
	verify_fatal(m_components_mask & COM_CLASS);
	return m_o_class;
}

s32& Instance::o_class()
{
	verify_fatal(m_components_mask & COM_CLASS);
	return m_o_class;
}

const PvarComponent& Instance::pvars() const
{
	verify_fatal(m_components_mask & COM_PVARS);
	return m_pvars;
}

PvarComponent& Instance::pvars() {
	verify_fatal(m_components_mask & COM_PVARS);
	return m_pvars;
}

const glm::vec3& Instance::colour() const
{
	verify_fatal(m_components_mask & COM_COLOUR);
	return m_colour;
}

glm::vec3& Instance::colour()
{
	verify_fatal(m_components_mask & COM_COLOUR);
	return m_colour;
}

f32 Instance::draw_distance() const
{
	verify_fatal(m_components_mask & COM_DRAW_DISTANCE);
	return m_draw_distance;
}

f32& Instance::draw_distance()
{
	verify_fatal(m_components_mask & COM_DRAW_DISTANCE);
	return m_draw_distance;
}

const std::vector<glm::vec4>& Instance::spline() const
{
	verify_fatal(m_components_mask & COM_SPLINE);
	return m_spline;
}

std::vector<glm::vec4>& Instance::spline()
{
	verify_fatal(m_components_mask & COM_SPLINE);
	return m_spline;
}

const CameraCollisionParams& Instance::camera_collision() const
{
	verify_fatal(m_components_mask & COM_CAMERA_COLLISION);
	return m_camera_collision;
}

CameraCollisionParams& Instance::camera_collision()
{
	verify_fatal(m_components_mask & COM_CAMERA_COLLISION);
	return m_camera_collision;
}

void Instance::read_common(const WtfNode* src)
{
	if (has_component(COM_TRANSFORM)) {
		transform().read(src);
	}
	
	if (has_component(COM_CLASS)) {
		read_inst_field(o_class(), src, "class");
	}
	
	if (has_component(COM_PVARS)) {
		pvars().read(src);
	}
	
	if (has_component(COM_COLOUR)) {
		read_inst_field(colour(), src, "col");
	}
	
	if (has_component(COM_DRAW_DISTANCE)) {
		draw_distance() = read_inst_float(src, "draw_dist");
	}
	
	if (has_component(COM_SPLINE)) {
		std::vector<glm::vec4>& points = spline();
		points.clear();
		const WtfAttribute* attrib = wtf_attribute_of_type(src, "spline", WTF_ARRAY);
		verify(attrib, "Missing 'spline' attribute.");
		for (WtfAttribute* vector_attrib = attrib->first_array_element; vector_attrib != nullptr; vector_attrib = vector_attrib->next) {
			verify(vector_attrib->type == WTF_ARRAY, "Invalid 'spline' attribute.");
			float vector[4];
			s32 i = 0;
			for (WtfAttribute* number_attrib = vector_attrib->first_array_element; number_attrib != nullptr; number_attrib = number_attrib->next) {
				verify(number_attrib->type == WTF_NUMBER && i < 4, "Invalid 'spline' attribute.");
				vector[i++] = number_attrib->number.f;
			}
			points.emplace_back(glm::vec4(vector[0], vector[1], vector[2], vector[3]));
		}
	}
}

void Instance::begin_write(WtfWriter* dest) const
{
	wtf_begin_node(dest, instance_type_to_string(type()), std::to_string(id().value).c_str());
	
	if (has_component(COM_TRANSFORM)) {
		transform().write(dest);
	}
	
	if (has_component(COM_CLASS)) {
		write_inst_field(dest, "class", o_class());
	}
	
	if (has_component(COM_PVARS)) {
		pvars().write(dest);
	}
	
	if (has_component(COM_COLOUR)) {
		write_inst_field(dest, "col", colour());
	}
	
	if (has_component(COM_DRAW_DISTANCE)) {
		wtf_write_float_attribute(dest, "draw_dist", draw_distance());
	}
	
	if (has_component(COM_SPLINE)) {
		wtf_begin_attribute(dest, "spline");
		wtf_begin_array(dest);
		for (const glm::vec4& vec : spline()) {
			wtf_write_floats(dest, &vec.x, 4);
		}
		wtf_end_array(dest);
		wtf_end_attribute(dest);
	}
}

void Instance::end_write(WtfWriter* dest) const
{
	wtf_end_node(dest);
}

#define GENERATED_INSTANCE_READ_WRITE_FUNCS
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_READ_WRITE_FUNCS

#define GENERATED_INSTANCE_TYPE_TO_STRING_FUNC
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_TYPE_TO_STRING_FUNC
