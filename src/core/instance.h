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

#include <core/util.h>

enum InstanceType : u32 {
	INST_NONE = 0,
	INST_RAC1_7c = 15,
	INST_ENV_SAMPLE_POINT = 1,
	INST_ENV_TRANSITION = 2,
	INST_CAMERA = 3,
	INST_SOUND = 4,
	INST_MOBY = 5,
	INST_PATH = 6,
	INST_CUBOID = 7,
	INST_SPHERE = 8,
	INST_CYLINDER = 9,
	INST_PILL = 10,
	INST_GRIND_PATH = 11,
	INST_LIGHT = 12,
	INST_TIE = 13,
	INST_SHRUB = 14,
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
	COM_BOUNDING_SPHERE = (1 << 6),
	COM_CAMERA_COLLISION = (1 << 7)
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
	POSITION,
	POSITION_ROTATION,
	POSITION_ROTATION_SCALE
};

using GlobalPvarPointers = std::vector<std::pair<s32, s32>>;

struct FromJsonVisitor;

struct CameraCollisionParams {
	bool enabled = false;
	s32 flags = 0;
	s32 i_value = 0;
	f32 f_value = 0.f;
	
	friend auto operator<=>(const CameraCollisionParams& lhs, const CameraCollisionParams& rhs) = default;
};

struct Instance {
	virtual ~Instance() {}
	
	InstanceId id() const { return _id; }
	void set_id_value(s32 value) { verify_fatal(_id.value == -1); _id.value = value; }
	InstanceType type() const { return _id.type; }
	u32 components_mask() const { return _components_mask; }
	bool has_component(InstanceComponent component) const { return (_components_mask & component) == component; }
	bool selected = false;
	
	void set_transform(glm::mat4 matrix, glm::mat3x4* inverse = nullptr);
	void set_transform(glm::mat4 matrix, glm::mat3x4 inverse, glm::vec3 rotation);
	void set_transform(glm::vec3 position, glm::vec3 rotation = glm::vec3(0.f, 0.f, 0.f), f32 scale = 1.f);
	glm::mat4 matrix() const;
	glm::mat3x4 inverse_matrix() const;
	glm::vec3 position() const;
	void set_position(glm::vec3 position);
	glm::vec3 rotation() const;
	void set_rotation(glm::vec3 rotation);
	f32 scale() const;
	void set_scale(f32 scale);
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
	
	CameraCollisionParams camera_collision() const;
	CameraCollisionParams& camera_collision();
	
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
	CameraCollisionParams _camera_collision;
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
				case TransformMode::POSITION: {
					glm::vec3 position = _transform.matrix[3];
					DEF_FIELD(position);
					_transform.matrix[3] = glm::vec4(position, 1.f);
					if constexpr(std::is_same_v<T, FromJsonVisitor>) {
						set_transform(position);
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
					verify_not_reached_fatal("Instance with a transform component lacks a valid transform mode.");
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
			auto& bounding_sphere = _bounding_sphere;
			DEF_FIELD(bounding_sphere);
		}
	}
};

struct RAC1_7c : Instance {
	RAC1_7c() : Instance(INST_RAC1_7c, COM_NONE, TransformMode::NONE) {}
	
	u32 unknown_0;
	u32 unknown_4;
	u32 unknown_8;
	u32 unknown_c;
	u32 unknown_10;
	u32 unknown_14;
	u32 unknown_18;
	u32 unknown_1c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(unknown_0);
		DEF_FIELD(unknown_4);
		DEF_FIELD(unknown_8);
		DEF_FIELD(unknown_c);
		DEF_FIELD(unknown_10);
		DEF_FIELD(unknown_14);
		DEF_FIELD(unknown_18);
		DEF_FIELD(unknown_1c);
	}
};


struct EnvSamplePointInstance : Instance {
	EnvSamplePointInstance() : Instance(INST_ENV_SAMPLE_POINT, COM_TRANSFORM, TransformMode::POSITION) {}
	
	s32 hero_light = 0;
	s16 music_track = 0;
	u8 hero_colour_r = 0;
	u8 hero_colour_g = 0;
	u8 hero_colour_b = 0;
	
	bool enable_reverb_params = false;
	u8 reverb_type = 0;
	s16 reverb_depth = 0;
	u8 reverb_delay = 0;
	u8 reverb_feedback = 0;
	
	bool enable_fog_params = false;
	u8 fog_near_intensity = 0;
	u8 fog_far_intensity = 0;
	u8 fog_r = 0;
	u8 fog_g = 0;
	u8 fog_b = 0;
	s16 fog_near_dist = 0;
	s16 fog_far_dist = 0;
};

packed_struct(Rgb32,
	/* 0x0 */ u8 r;
	/* 0x1 */ u8 g;
	/* 0x2 */ u8 b;
	/* 0x3 */ u8 pad;
)

packed_struct(Rgb96,
	/* 0x0 */ s32 r;
	/* 0x4 */ s32 g;
	/* 0xc */ s32 b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(r);
		DEF_PACKED_FIELD(g);
		DEF_PACKED_FIELD(b);
	}
)

struct EnvTransitionInstance : Instance {
	EnvTransitionInstance() : Instance(INST_ENV_TRANSITION, COM_TRANSFORM | COM_BOUNDING_SPHERE, TransformMode::MATRIX) {}
	
	bool enable_hero;
	Rgb32 hero_colour_1;
	Rgb32 hero_colour_2;
	s32 hero_light_1;
	s32 hero_light_2;
	bool enable_fog;
	Rgb32 fog_colour_1;
	Rgb32 fog_colour_2;
	f32 fog_near_dist_1;
	f32 fog_near_intensity_1;
	f32 fog_far_dist_1;
	f32 fog_far_intensity_1;
	f32 fog_near_dist_2;
	f32 fog_near_intensity_2;
	f32 fog_far_dist_2;
	f32 fog_far_intensity_2;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(enable_hero);
		DEF_FIELD(hero_colour_1);
		DEF_FIELD(hero_colour_2);
		DEF_FIELD(hero_light_1);
		DEF_FIELD(hero_light_2);
		DEF_FIELD(enable_fog);
		DEF_FIELD(fog_colour_1);
		DEF_FIELD(fog_colour_2);
		DEF_FIELD(fog_near_dist_1);
		DEF_FIELD(fog_near_intensity_1);
		DEF_FIELD(fog_far_dist_1);
		DEF_FIELD(fog_far_intensity_1);
		DEF_FIELD(fog_near_dist_2);
		DEF_FIELD(fog_near_intensity_2);
		DEF_FIELD(fog_far_dist_2);
		DEF_FIELD(fog_far_intensity_2);
	}
};

struct Camera : Instance {
	Camera() : Instance(INST_CAMERA, COM_TRANSFORM | COM_PVARS, TransformMode::POSITION_ROTATION) {}
	s32 type;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(type);
	}
};

struct Cuboid : Instance {
	Cuboid() : Instance(INST_CUBOID, COM_TRANSFORM | COM_CAMERA_COLLISION, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

struct Sphere : Instance {
	Sphere() : Instance(INST_SPHERE, COM_TRANSFORM | COM_CAMERA_COLLISION, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

struct Cylinder : Instance {
	Cylinder() : Instance(INST_CYLINDER, COM_TRANSFORM | COM_CAMERA_COLLISION, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

// There are none of these in any of the games.
struct Pill : Instance {
	Pill() : Instance(INST_PILL, COM_TRANSFORM | COM_CAMERA_COLLISION, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

struct SoundInstance : Instance {
	SoundInstance() : Instance(INST_SOUND, COM_TRANSFORM | COM_PVARS, TransformMode::MATRIX_INVERSE_ROTATION) {}
	s16 o_class;
	s16 m_class;
	f32 range;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(o_class);
		DEF_FIELD(m_class);
		DEF_FIELD(range);
	}
};

struct MobyInstance : Instance {
	MobyInstance() : Instance(INST_MOBY, COM_TRANSFORM | COM_PVARS | COM_DRAW_DISTANCE | COM_COLOUR, TransformMode::POSITION_ROTATION_SCALE) {}
	s8 mission;
	s32 uid;
	s32 bolts;
	s32 o_class;
	s32 update_distance;
	s32 group;
	bool is_rooted;
	f32 rooted_distance;
	s32 occlusion;
	s32 mode_bits;
	s32 light;
	s32 rac1_unknown_4;
	s32 rac1_unknown_8;
	s32 rac1_unknown_c;
	s32 rac1_unknown_10;
	s32 rac1_unknown_14;
	s32 rac1_unknown_18;
	s32 rac1_unknown_1c;
	s32 rac1_unknown_20;
	s32 rac1_unknown_24;
	s32 rac1_unknown_54;
	s32 rac1_unknown_60;
	s32 rac1_unknown_74;
	s32 rac23_unknown_8;
	s32 rac23_unknown_c;
	s32 rac23_unknown_18;
	s32 rac23_unknown_1c;
	s32 rac23_unknown_20;
	s32 rac23_unknown_24;
	s32 rac23_unknown_38;
	s32 rac23_unknown_3c;
	s32 rac23_unknown_4c;
	s32 rac23_unknown_84;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(mission);
		DEF_FIELD(uid);
		DEF_FIELD(bolts);
		DEF_FIELD(o_class);
		DEF_FIELD(update_distance);
		DEF_FIELD(group);
		DEF_FIELD(is_rooted);
		DEF_FIELD(rooted_distance);
		DEF_FIELD(occlusion);
		DEF_FIELD(mode_bits);
		DEF_FIELD(light);
		DEF_FIELD(rac1_unknown_4);
		DEF_FIELD(rac1_unknown_8);
		DEF_FIELD(rac1_unknown_c);
		DEF_FIELD(rac1_unknown_10);
		DEF_FIELD(rac1_unknown_14);
		DEF_FIELD(rac1_unknown_18);
		DEF_FIELD(rac1_unknown_1c);
		DEF_FIELD(rac1_unknown_20);
		DEF_FIELD(rac1_unknown_24);
		DEF_FIELD(rac1_unknown_54);
		DEF_FIELD(rac1_unknown_60);
		DEF_FIELD(rac1_unknown_74);
		DEF_FIELD(rac23_unknown_8);
		DEF_FIELD(rac23_unknown_c);
		DEF_FIELD(rac23_unknown_18);
		DEF_FIELD(rac23_unknown_1c);
		DEF_FIELD(rac23_unknown_20);
		DEF_FIELD(rac23_unknown_24);
		DEF_FIELD(rac23_unknown_4c);
		DEF_FIELD(rac23_unknown_84);
	}
};

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

struct Group {
	s32 id;
	std::vector<u16> members;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(id);
		DEF_FIELD(members);
	}
};

struct GC_54_DL_38 {
	std::vector<s8> first_part;
	std::vector<s64> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
	}
};

struct Path : Instance {
	Path() : Instance(INST_PATH, COM_SPLINE, TransformMode::NONE) {}
};

struct GC_80_DL_64 {
	std::vector<u8> first_part;
	std::vector<u8> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_HEXDUMP(first_part);
		DEF_HEXDUMP(second_part);
	}
};

struct GrindPath : Instance {
	GrindPath() : Instance(INST_GRIND_PATH, COM_SPLINE | COM_BOUNDING_SPHERE, TransformMode::NONE) {}
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(unknown_4);
		DEF_FIELD(wrap);
		DEF_FIELD(inactive);
	}
};

enum AreaPart {
	AREA_PART_PATHS = 0,
	AREA_PART_CUBOIDS = 1,
	AREA_PART_SPHERES = 2,
	AREA_PART_CYLINDERS = 3,
	AREA_PART_NEG_CUBOIDS = 4
};

struct Area {
	s32 id;
	glm::vec4 bounding_sphere;
	s32 last_update_time;
	std::vector<s32> parts[5];
	
	template <typename T>
	void enumerate_fields(T& t) {
		std::vector<s32>& paths = parts[AREA_PART_PATHS];
		std::vector<s32>& cuboids = parts[AREA_PART_CUBOIDS];
		std::vector<s32>& spheres = parts[AREA_PART_SPHERES];
		std::vector<s32>& cylinders = parts[AREA_PART_CYLINDERS];
		std::vector<s32>& negative_cuboids = parts[AREA_PART_NEG_CUBOIDS];
		
		DEF_FIELD(id);
		DEF_FIELD(bounding_sphere);
		DEF_FIELD(last_update_time);
		DEF_FIELD(paths);
		DEF_FIELD(cuboids);
		DEF_FIELD(spheres);
		DEF_FIELD(cylinders);
		DEF_FIELD(negative_cuboids);
	}
};

struct DirectionalLight : Instance {
	DirectionalLight() : Instance(INST_LIGHT, COM_NONE, TransformMode::NONE) {}
	glm::vec4 colour_a;
	glm::vec4 direction_a;
	glm::vec4 colour_b;
	glm::vec4 direction_b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(colour_a);
		DEF_FIELD(direction_a);
		DEF_FIELD(colour_b);
		DEF_FIELD(direction_b);
	}
};

struct TieInstance : Instance {
	TieInstance() : Instance(INST_TIE, COM_TRANSFORM | COM_DRAW_DISTANCE, TransformMode::MATRIX) {}
	s32 o_class;
	s32 occlusion_index;
	s32 directional_lights;
	s32 uid;
	std::vector<u8> ambient_rgbas;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(o_class);
		DEF_FIELD(occlusion_index);
		DEF_FIELD(directional_lights);
		DEF_FIELD(uid);
		DEF_HEXDUMP(ambient_rgbas);
	}
};

struct ShrubInstance : Instance {
	ShrubInstance() : Instance(INST_SHRUB, COM_TRANSFORM | COM_DRAW_DISTANCE | COM_COLOUR, TransformMode::MATRIX) {}
	s32 o_class;
	s32 unknown_5c;
	s32 dir_lights;
	s32 unknown_64;
	s32 unknown_68;
	s32 unknown_6c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(o_class);
		DEF_FIELD(unknown_5c);
		DEF_FIELD(dir_lights);
		DEF_FIELD(unknown_64);
		DEF_FIELD(unknown_68);
		DEF_FIELD(unknown_6c);
	}
};

packed_struct(OcclusionMapping,
	s32 bit_index;
	s32 occlusion_id;
)

struct OcclusionMappings {
	std::vector<OcclusionMapping> tfrag_mappings;
	std::vector<OcclusionMapping> tie_mappings;
	std::vector<OcclusionMapping> moby_mappings;
};

#endif
