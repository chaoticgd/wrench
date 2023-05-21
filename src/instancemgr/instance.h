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

#ifndef INSTANCEMGR_INSTANCE_H
#define INSTANCEMGR_INSTANCE_H

#include <map>
#include <glm/glm.hpp>

#include <core/util.h>

#define GENERATED_INSTANCE_TYPE_ENUM
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_TYPE_ENUM

struct Instances;
struct WtfNode;
struct WtfWriter;

struct InstanceId {
	InstanceType type;
	s32 value;
	
	friend auto operator<=>(const InstanceId& lhs, const InstanceId& rhs) = default;
};

static constexpr const InstanceId NULL_INSTANCE_ID = {
	INST_NONE, -1
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

enum class TransformMode {
	NONE,
	MATRIX,
	MATRIX_AND_INVERSE,
	MATRIX_INVERSE_ROTATION,
	POSITION,
	POSITION_ROTATION,
	POSITION_ROTATION_SCALE
};

class TransformComponent {
public:
	TransformComponent() : _mode(TransformMode::NONE) {}
	TransformComponent(TransformMode mode) : _mode(mode) {}

	const glm::mat4& matrix() const;
	const glm::mat3x4& inverse_matrix() const;
	const glm::vec3& pos() const;
	const glm::vec3& rot() const;
	const f32& scale() const;
	void set_from_matrix(const glm::mat4& new_matrix, const glm::mat3x4* new_inverse_matrix = nullptr, const glm::vec3* new_rot = nullptr);
	void set_from_pos_rot_scale(const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.f, 0.f, 0.f), f32 scale = 1.f);
	
	void read(const WtfNode* src);
	void write(WtfWriter* dest) const;
private:
	TransformMode _mode;
	glm::mat4 _matrix = glm::mat4(1.f);
	glm::mat3x4 _inverse_matrix = glm::mat3x4(1.f);
	glm::vec3 _rot = glm::vec3(0.f);
	f32 _scale = 1.f;
};

using GlobalPvarPointers = std::vector<std::pair<s32, s32>>;

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
	
	const TransformComponent& transform() const;
	TransformComponent& transform();
	
	const std::vector<u8>& pvars() const;
	std::vector<u8>& pvars();
	// These last two pvar members are only used during reading/writing!
	s32 temp_pvar_index() const;
	s32& temp_pvar_index();
	const GlobalPvarPointers& temp_global_pvar_pointers() const;
	GlobalPvarPointers& temp_global_pvar_pointers();
	
	const glm::vec3& colour() const;
	glm::vec3& colour();
	
	f32 draw_distance() const;
	f32& draw_distance();
	
	const std::vector<glm::vec4>& spline() const;
	std::vector<glm::vec4>& spline();
	
	const glm::vec4& bounding_sphere() const;
	glm::vec4& bounding_sphere();
	
	const CameraCollisionParams& camera_collision() const;
	CameraCollisionParams& camera_collision();
	
	void read_common(const WtfNode* src);
	void begin_write(WtfWriter* dest) const;
	void end_write(WtfWriter* dest) const;
	
protected:
	Instance(InstanceType type, u32 components_mask)
		: _id({type, -1})
		, _components_mask(components_mask) {}
	Instance(InstanceType type, u32 components_mask, TransformMode transform_mode)
		: _id({type, -1})
		, _components_mask(components_mask)
		, _transform(transform_mode) {}
private:
	InstanceId _id;
	u32 _components_mask;
	TransformComponent _transform;
	std::vector<u8> _pvars;
	s32 _pvar_index = -1; // Only used during reading/writing!
	GlobalPvarPointers _global_pvar_pointers; // Only used when writing!
	glm::vec3 _colour = glm::vec3(0.f, 0.f, 0.f);
	f32 _draw_distance = 0.f;
	std::vector<glm::vec4> _spline;
	glm::vec4 _bounding_sphere = glm::vec4(0.f);
	CameraCollisionParams _camera_collision;
};

const char* instance_type_to_string(InstanceType type);

#define GENERATED_INSTANCE_TYPES
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_TYPES

struct Group {
	s32 id;
	std::vector<u16> members;
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

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

template <typename ThisInstance>
class InstanceList {
private:
	std::vector<ThisInstance> _instances;
	std::map<s32, s32> _id_to_index;
public:
	InstanceList<ThisInstance>& operator=(std::vector<ThisInstance>&& rhs) {
		_instances = rhs;
		for(s32 i = 0; i < (s32) _instances.size(); i++) {
			_id_to_index[_instances[i].id().value] = i;
		}
		return *this;
	}
	
	ThisInstance& operator[](s32 index) { return _instances.at(index); }
	const ThisInstance& operator[](s32 index) const { return _instances.at(index); }
	s32 size() const { return (s32) _instances.size(); }
	bool empty() const { return _instances.size() == 0; }
	
	typename std::vector<ThisInstance>::iterator begin() { return _instances.begin(); }
	typename std::vector<ThisInstance>::const_iterator begin() const { return _instances.begin(); }
	typename std::vector<ThisInstance>::iterator end() { return _instances.end(); }
	typename std::vector<ThisInstance>::const_iterator end() const { return _instances.end(); }
	
	ThisInstance* from_id(s32 id) {
		auto index = _id_to_index.find(id);
		if(index != _id_to_index.end()) {
			return _instances.at(index->second);
		} else {
			return nullptr;
		}
	}
	
	std::vector<ThisInstance> release() {
		_id_to_index.clear();
		return std::move(_instances);
	}
	
	ThisInstance& create(s32 id = -1) {
		if(id == -1) {
			id = next_id();
		}
		_id_to_index[id] = (s32) _instances.size();
		ThisInstance& inst = _instances.emplace_back();
		inst.set_id_value(id);
		return inst;
	}
	
	s32 next_id() const {
		s32 id = 0;
		for(const ThisInstance& inst : _instances) {
			id = std::max(id, inst.id().value);
		}
		return id;
	}
};

#endif
