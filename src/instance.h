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

#ifndef INSTANCE_H
#define INSTANCE_H

#include <glm/glm.hpp>

#include "util.h"

enum InstanceType : u32 {
	INST_GC_8c_DL_70,
	INST_LIGHT_TRIGGER,
	INST_CAMERA,
	INST_SOUND,
	INST_MOBY,
	INST_PATH,
	INST_CUBOID,
	INST_SPHERE,
	INST_CYLINDER,
	INST_GRIND_PATH,
	INST_LIGHT,
	INST_TIE,
	INST_SHRUB
};

struct InstanceId {
	InstanceType type;
	s32 generation;
	s32 value;
};

enum InstanceComponent : u32 {
	COM_TRANSFORM = (1 << 1),
	COM_PVARS = (1 << 2),
	COM_COLOUR = (1 << 3),
	COM_DRAW_DISTANCE = (1 << 4),
	COM_PATH = (1 << 5),
	COM_BOUNDING_SPHERE = (1 << 6)
};

struct Colour {
	u8 r;
	u8 g;
	u8 b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(r);
		DEF_FIELD(g);
		DEF_FIELD(b);
	}
};

enum class TransformMode {
	NONE,
	MATRIX,
	MATRIX_AND_INVERSE,
	MATRIX_INVERSE_ROTATION,
	POSITION_ROTATION,
	POSITION_ROTATION_SCALE
};

using GlobalPvarPointers = std::vector<std::pair<s32, s32>>;

packed_struct(BoundingSphere,
	f32 x;
	f32 y;
	f32 z;
	f32 radius;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(x);
		DEF_PACKED_FIELD(y);
		DEF_PACKED_FIELD(z);
		DEF_PACKED_FIELD(radius);
	}
)

struct FromJsonVisitor;

struct Instance {
	InstanceId id() const { return _id; }
	void set_id_value(s32 value) { assert(_id.value == -1); _id.value = value; }
	InstanceType type() const { return _id.type; }
	bool has_component(InstanceComponent component) const { return _components_mask & component; }
	bool selected = false;
	
	void set_transform(glm::mat4 matrix, glm::mat3x4* inverse = nullptr);
	void set_transform(glm::mat4 matrix, glm::mat3x4 inverse, glm::vec3 rotation);
	void set_transform(glm::vec3 position, glm::vec3 rotation, f32 scale = 1.f);
	glm::mat4 matrix() const { return _transform.matrix; }
	glm::mat3x4 inverse_matrix() const { return _transform.inverse_matrix; }
	glm::vec3 position() const { return _transform.matrix[3]; }
	glm::vec3 rotation() const { return _transform.rotation; }
	f32 scale() const { return _transform.scale; }
	
	const std::vector<u8>& pvars() const;
	std::vector<u8>& pvars();
	// These last two pvar members are only used during reading/writing!
	s32 temp_pvar_index() const;
	s32& temp_pvar_index();
	const GlobalPvarPointers& temp_global_pvar_pointers() const;
	GlobalPvarPointers& temp_global_pvar_pointers();
	
	const Colour& colour() const;
	Colour& colour();
	
	const f32& draw_distance() const;
	f32& draw_distance();
	
	const std::vector<glm::vec4>& path() const;
	std::vector<glm::vec4>& path();
	
	const glm::vec4& bounding_sphere() const;
	glm::vec4& bounding_sphere();
	
protected:
	Instance(InstanceType type, u32 components_mask, TransformMode transform_mode)
		: _id({type, 0, -1}), _components_mask(components_mask), _transform_mode(transform_mode) {}

private:
	InstanceId _id;
	u32 _components_mask;
	TransformMode _transform_mode; // Only relevant while reading/writing JSON.
	struct {
		glm::mat4 matrix = glm::mat4(1.f);
		glm::mat3x4 inverse_matrix = glm::mat3x4(1.f);
		glm::vec3 rotation = glm::vec3(0.f);
		f32 scale;
	} _transform;
	std::vector<u8> _pvars;
	s32 _pvar_index = -1; // Only used during reading/writing!
	GlobalPvarPointers _global_pvar_pointers; // Only used when writing!
	Colour _colour = {0, 0, 0};
	f32 _draw_distance = 0.f;
	std::vector<glm::vec4> _path;
	glm::vec4 _bounding_sphere = glm::vec4(0.f);

public:
template <typename T>
	void enumerate_fields(T& t) {
		s32& id = _id.value;
		DEF_FIELD(id);
		if(has_component(COM_TRANSFORM)) {
			switch(_transform_mode) {
				case TransformMode::MATRIX: {
					glm::mat4& matrix = _transform.matrix;
					DEF_FIELD(matrix);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(_transform.matrix);
					}
					break;
				}
				case TransformMode::MATRIX_AND_INVERSE: {
					glm::mat4& matrix = _transform.matrix;
					DEF_FIELD(matrix);
					glm::mat3x4& inverse_matrix = _transform.inverse_matrix;
					DEF_FIELD(inverse_matrix);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(_transform.matrix, &_transform.inverse_matrix);
					}
					break;
				}
				case TransformMode::MATRIX_INVERSE_ROTATION: {
					glm::mat4& matrix = _transform.matrix;
					DEF_FIELD(matrix);
					glm::mat3x4& inverse_matrix = _transform.inverse_matrix;
					DEF_FIELD(inverse_matrix);
					glm::vec3& rotation = _transform.rotation;
					DEF_FIELD(rotation);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(_transform.matrix, _transform.inverse_matrix, _transform.rotation);
					}
					break;
				}
				case TransformMode::POSITION_ROTATION: {
					glm::vec3 position = _transform.matrix[3];
					DEF_FIELD(position);
					_transform.matrix[3] = glm::vec4(position, 1.f);
					glm::vec3& rotation = _transform.rotation;
					DEF_FIELD(rotation);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(position, rotation);
					}
					break;
				}
				case TransformMode::POSITION_ROTATION_SCALE: {
					glm::vec3 position = _transform.matrix[3];
					DEF_FIELD(position);
					_transform.matrix[3] = glm::vec4(position, 1.f);
					glm::vec3& rotation = _transform.rotation;
					DEF_FIELD(rotation);
					f32& scale = _transform.scale;
					DEF_FIELD(scale);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(position, rotation, scale);
					}
					break;
				}
				default: {
					assert_not_reached("Instance with a transform component lacks a valid transform mode.");
				}
			}
		}
		if(has_component(COM_PVARS)) {
			auto& pvars = _pvars;
			DEF_HEXDUMP(pvars);
		}
		if(has_component(COM_COLOUR)) {
			auto& colour = _colour;
			DEF_FIELD(colour);
		}
		if(has_component(COM_DRAW_DISTANCE)) {
			auto& draw_distance = _draw_distance;
			DEF_FIELD(draw_distance);
		}
		if(has_component(COM_PATH)) {
			auto& vertices = _path;
			DEF_FIELD(vertices);
		}
		if(has_component(COM_BOUNDING_SPHERE)) {
			auto& bounding_sphere = (BoundingSphere&) _bounding_sphere;
			DEF_FIELD(bounding_sphere);
		}
	}
};

#endif