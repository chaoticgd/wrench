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

struct InstanceLink {};

#define DEF_INSTANCE(inst_type, inst_type_uppercase, inst_variable, link_type) \
	struct link_type : InstanceLink { \
		s32 id; \
		link_type() : id(-1) {} \
		link_type(s32 i) : id(i) {} \
		friend auto operator<=>(const link_type& lhs, const link_type& rhs) = default; \
	}; \
	typedef std::vector<link_type> link_type##s;
#define GENERATED_INSTANCE_MACRO_CALLS
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_MACRO_CALLS
#undef DEF_INSTANCE

struct Instances;
struct WtfNode;
struct WtfWriter;

struct InstanceId
{
	InstanceType type;
	s32 value;
	
	friend auto operator<=>(const InstanceId& lhs, const InstanceId& rhs) = default;
};

static constexpr const InstanceId NULL_INSTANCE_ID = {
	INST_NONE, -1
};

enum InstanceComponent : u32
{
	COM_NONE = 0,
	COM_TRANSFORM = (1 << 1),
	COM_CLASS = (1 << 2),
	COM_PVARS = (1 << 3),
	COM_COLOUR = (1 << 4),
	COM_DRAW_DISTANCE = (1 << 5),
	COM_SPLINE = (1 << 6),
	COM_CAMERA_COLLISION = (1 << 7)
};

enum class TransformMode
{
	NONE,
	MATRIX,
	MATRIX_INVERSE,
	MATRIX_AND_INVERSE,
	MATRIX_INVERSE_ROTATION,
	POSITION,
	POSITION_ROTATION,
	POSITION_ROTATION_SCALE
};

class TransformComponent
{
public:
	TransformComponent() : m_mode(TransformMode::NONE) {}
	TransformComponent(TransformMode mode) : m_mode(mode) {}

	const glm::mat4& matrix() const;
	const glm::mat4& inverse_matrix() const;
	const glm::vec3& pos() const;
	const glm::vec3& rot() const;
	const f32& scale() const;
	void set_from_matrix(const glm::mat4* new_matrix, const glm::mat4* new_inverse_matrix = nullptr, const glm::vec3* new_rot = nullptr);
	void set_from_pos_rot_scale(const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.f, 0.f, 0.f), f32 scale = 1.f);
	
	void read(const WtfNode* src);
	void write(WtfWriter* dest) const;
private:
	TransformMode m_mode;
	glm::mat4 m_matrix = glm::mat4(1.f);
	glm::mat4 m_inverse_matrix = glm::mat4(1.f);
	glm::vec3 m_rot = glm::vec3(0.f);
	f32 m_scale = 1.f;
};

enum class PvarPointerType
{
	NULLPTR, // Null pointer.
	RELATIVE, // Pointer relative to the beginning of the pvar structure.
	SHARED // Pointer to a structure in the shared data section.
};

struct PvarPointer
{
	s32 offset = -1;
	PvarPointerType type = PvarPointerType::NULLPTR;
	s32 shared_data_id = -1;
	
	friend auto operator<=>(const PvarPointer& lhs, const PvarPointer& rhs) = default;
};

struct PvarComponent
{
	std::vector<u8> data;
	std::vector<PvarPointer> pointers; // Must always be sorted!
	mutable s32 temp_pvar_index = -1;
	
	void read(const WtfNode* src);
	void write(WtfWriter* dest) const;
	void validate() const;
};

using GlobalPvarPointers = std::vector<std::pair<s32, s32>>;

struct CameraCollisionParams
{
	bool enabled = false;
	s32 flags = 0;
	s32 i_value = 0;
	f32 f_value = 0.f;
	
	friend auto operator<=>(const CameraCollisionParams& lhs, const CameraCollisionParams& rhs) = default;
};

struct Instance
{
	virtual ~Instance() {}
	
	InstanceId id() const { return m_id; }
	void set_id_value(s32 value) { verify_fatal(m_id.value == -1); m_id.value = value; }
	InstanceType type() const { return m_id.type; }
	u32 components_mask() const { return m_components_mask; }
	bool has_component(InstanceComponent component) const { return (m_components_mask & component) == component; }
	bool selected = false;
	bool referenced_by_selected = false;
	bool is_dragging = false;
	glm::mat4 drag_preview_matrix = glm::mat4(1.f);
	
	const TransformComponent& transform() const;
	TransformComponent& transform();
	
	s32 o_class() const;
	s32& o_class();
	
	const PvarComponent& pvars() const;
	PvarComponent& pvars();
	
	const glm::vec3& colour() const;
	glm::vec3& colour();
	
	f32 draw_distance() const;
	f32& draw_distance();
	
	const std::vector<glm::vec4>& spline() const;
	std::vector<glm::vec4>& spline();
	
	const CameraCollisionParams& camera_collision() const;
	CameraCollisionParams& camera_collision();
	
	void read_common(const WtfNode* src);
	void begin_write(WtfWriter* dest) const;
	void end_write(WtfWriter* dest) const;
	
protected:
	Instance(InstanceType type, u32 components_mask)
		: m_id({type, -1})
		, m_components_mask(components_mask) {}
	Instance(InstanceType type, u32 components_mask, TransformMode transform_mode)
		: m_id({type, -1})
		, m_components_mask(components_mask)
		, m_transform(transform_mode) {}
private:
	InstanceId m_id;
	u32 m_components_mask;
	TransformComponent m_transform;
	s32 m_o_class = -1;
	PvarComponent m_pvars;
	glm::vec3 m_colour = glm::vec3(0.f, 0.f, 0.f);
	f32 m_draw_distance = 0.f;
	std::vector<glm::vec4> m_spline;
	glm::vec4 m_bounding_sphere = glm::vec4(0.f);
	CameraCollisionParams m_camera_collision;
};

const char* instance_type_to_string(InstanceType type);

#define GENERATED_INSTANCE_TYPES
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_TYPES

enum MobyModeBits1
{
	MOBY_MB1_HAS_SUB_VARS = 0x20
};

template <typename ThisInstance>
class InstanceList
{
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
			return &_instances.at(index->second);
		} else {
			return nullptr;
		}
	}
	
	s32 id_to_index(s32 id) const {
		auto index = _id_to_index.find(id);
		if(index != _id_to_index.end()) {
			return index->second;
		} else {
			return -1;
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
