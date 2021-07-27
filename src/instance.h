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
	INST_NONE = 0,
	INST_GC_8c_DL_70 = 1,
	INST_LIGHT_TRIGGER = 2,
	INST_CAMERA = 3,
	INST_SOUND = 4,
	INST_MOBY = 5,
	INST_PATH = 6,
	INST_CUBOID = 7,
	INST_SPHERE = 8,
	INST_CYLINDER = 9,
	INST_GRIND_PATH = 10,
	INST_LIGHT = 11,
	INST_TIE = 12,
	INST_SHRUB = 13
};

struct InstanceId {
	InstanceType type;
	s32 generation;
	s32 value;
	
	bool operator==(const InstanceId& rhs) const {
		return type == rhs.type && generation == rhs.generation && value == rhs.value;
	}
};

static constexpr const InstanceId NULL_INSTANCE_ID = {
	INST_NONE, -1, -1
};

enum InstanceComponent : u32 {
	COM_NONE = 0,
	COM_TRANSFORM = (1 << 1),
	COM_PVARS = (1 << 2),
	COM_COLOUR = (1 << 3),
	COM_DRAW_DISTANCE = (1 << 4),
	COM_SPLINE = (1 << 5),
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
	
	glm::vec4 unpack() const {
		glm::vec4 result;
		result.x = x;
		result.y = y;
		result.z = z;
		result.w = radius;
		return result;
	}
	
	static BoundingSphere pack(glm::vec4 vec) {
		BoundingSphere result;
		result.x = vec.x;
		result.y = vec.y;
		result.z = vec.z;
		result.radius = vec.w;
		return result;
	}
)

struct FromJsonVisitor;

struct Instance {
	virtual ~Instance() {}
	
	InstanceId id() const { return _id; }
	void set_id_value(s32 value) { assert(_id.value == -1); _id.value = value; }
	InstanceType type() const { return _id.type; }
	u32 components_mask() const { return _components_mask; }
	bool has_component(InstanceComponent component) const { return (_components_mask & component) == component; }
	bool selected = false;
	
	void set_transform(glm::mat4 matrix, glm::mat3x4* inverse = nullptr);
	void set_transform(glm::mat4 matrix, glm::mat3x4 inverse, glm::vec3 rotation);
	void set_transform(glm::vec3 position, glm::vec3 rotation, f32 scale = 1.f);
	glm::mat4 matrix() const;
	glm::mat3x4 inverse_matrix() const;
	glm::vec3 position() const;
	void set_position(glm::vec3 position);
	glm::vec3 rotation() const;
	void set_rotation(glm::vec3 rotation);
	f32 scale() const;
	f32& m33_value_do_not_use();
	
	const std::vector<u8>& pvars() const;
	std::vector<u8>& pvars();
	// These last two pvar members are only used during reading/writing!
	s32 temp_pvar_index() const;
	s32& temp_pvar_index();
	const GlobalPvarPointers& temp_global_pvar_pointers() const;
	GlobalPvarPointers& temp_global_pvar_pointers();
	
	const Colour& colour() const;
	Colour& colour();
	
	f32 draw_distance() const;
	f32& draw_distance();
	
	const std::vector<glm::vec4>& spline() const;
	std::vector<glm::vec4>& spline();
	
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
		f32 m33 = 0.01f; // Preserve the original value of matrix[3][3].
	} _transform;
	std::vector<u8> _pvars;
	s32 _pvar_index = -1; // Only used during reading/writing!
	GlobalPvarPointers _global_pvar_pointers; // Only used when writing!
	Colour _colour = {0, 0, 0};
	f32 _draw_distance = 0.f;
	std::vector<glm::vec4> _spline;
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
					matrix[3][3] = _transform.m33;
					DEF_FIELD(matrix);
					_transform.m33 = matrix[3][3];
					matrix[3][3] = 1.f;
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(_transform.matrix);
					}
					break;
				}
				case TransformMode::MATRIX_AND_INVERSE: {
					glm::mat4& matrix = _transform.matrix;
					matrix[3][3] = _transform.m33;
					DEF_FIELD(matrix);
					_transform.m33 = matrix[3][3];
					matrix[3][3] = 1.f;
					glm::mat3x4& inverse_matrix = _transform.inverse_matrix;
					DEF_FIELD(inverse_matrix);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(_transform.matrix, &_transform.inverse_matrix);
					}
					break;
				}
				case TransformMode::MATRIX_INVERSE_ROTATION: {
					glm::mat4& matrix = _transform.matrix;
					matrix[3][3] = _transform.m33;
					DEF_FIELD(matrix);
					_transform.m33 = matrix[3][3];
					matrix[3][3] = 1.f;
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
		if(has_component(COM_SPLINE)) {
			auto& vertices = _spline;
			DEF_FIELD(vertices);
		}
		if(has_component(COM_BOUNDING_SPHERE)) {
			BoundingSphere bounding_sphere = BoundingSphere::pack(_bounding_sphere);
			DEF_FIELD(bounding_sphere);
			_bounding_sphere = bounding_sphere.unpack();
		}
	}
};

#endif
