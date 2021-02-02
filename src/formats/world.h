/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef FORMATS_WORLD_H
#define FORMATS_WORLD_H

#include <array>
#include <functional>

#include "level_types.h"

struct game_string {
	uint32_t id;
	uint32_t secondary_id;
	std::string str;
};

struct entity_id {
	std::size_t value;
	
	bool operator<(const entity_id& rhs) const { return value < rhs.value; }
	bool operator==(const entity_id& rhs) const { return value == rhs.value; }
	bool operator!=(const entity_id& rhs) const { return value != rhs.value; }
};

static const entity_id NULL_ENTITY_ID { 0 };

struct entity {
	virtual ~entity() {}
	entity_id id;
	bool selected;
};

struct matrix_entity : public entity {
	glm::mat4 local_to_world;
};

struct euler_entity : public entity {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::mat4 local_to_world_cache;
	glm::mat4 local_to_clip_cache;
};

struct tie_entity final : public matrix_entity {
	uint32_t unknown_0;
	uint32_t unknown_4;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint32_t unknown_50;
	int32_t  uid;
	uint32_t unknown_58;
	uint32_t unknown_5c;
};

struct shrub_entity final : public matrix_entity {
	uint32_t unknown_0;
	float    unknown_4;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint32_t unknown_50;
	uint32_t unknown_54;
	uint32_t unknown_58;
	uint32_t unknown_5c;
	uint32_t unknown_60;
	uint32_t unknown_64;
	uint32_t unknown_68;
	uint32_t unknown_6c;
};

struct moby_entity final : public euler_entity {
	uint32_t size;
	int32_t unknown_4;
	uint32_t unknown_8;
	uint32_t unknown_c;
	int32_t  uid;
	uint32_t unknown_14;
	uint32_t unknown_18;
	uint32_t unknown_1c;
	uint32_t unknown_20;
	uint32_t unknown_24;
	uint32_t class_num;
	float    scale;
	uint32_t unknown_30;
	uint32_t unknown_34;
	uint32_t unknown_38;
	uint32_t unknown_3c;
	int32_t unknown_58;
	uint32_t unknown_5c;
	uint32_t unknown_60;
	uint32_t unknown_64;
	int32_t  pvar_index;
	uint32_t unknown_6c;
	uint32_t unknown_70;
	uint32_t unknown_74;
	uint32_t unknown_78;
	uint32_t unknown_7c;
	uint32_t unknown_80;
	int32_t unknown_84;
};

struct trigger_entity final : matrix_entity {
	glm::mat4 matrix_reloaded;
};

struct spline_entity : entity {
	std::vector<glm::vec4> vertices;
};

struct regular_spline_entity final : spline_entity {};

struct grindrail_spline_entity final : spline_entity {
	glm::vec4 special_point;
	uint8_t unknown_10[0x10];
};

class world_segment {
public:
	world_segment() {}

	world_properties properties;
	std::vector<world_property_thing> property_things;
	std::vector<world_directional_light> directional_lights;
	std::vector<world_thing_8> thing_8s;
	std::vector<world_thing_c> thing_cs;
	std::array<std::vector<game_string>, 8> game_strings;
	std::vector<uint32_t> thing_30s;
	std::vector<tie_entity> ties;
	std::vector<uint32_t> thing_38_1s;
	std::vector<uint8_t> thing_38_2s;
	std::vector<uint32_t> thing_3cs;
	std::vector<shrub_entity> shrubs;
	std::vector<uint32_t> thing_44_1s;
	std::vector<uint8_t> thing_44_2s;
	std::vector<uint32_t> thing_48s;
	std::vector<moby_entity> mobies;
	uint32_t max_moby_count;
	std::vector<uint32_t> thing_50_1s;
	std::vector<uint8_t> thing_50_2s;
	std::vector<uint8_t> thing_54_1s;
	std::vector<uint64_t> thing_54_2s;
	std::vector<world_thing_58> thing_58s;
	std::vector<std::vector<uint8_t>> pvars;
	std::vector<world_thing_64> thing_64s;
	std::vector<trigger_entity> triggers;
	std::vector<world_thing_6c> thing_6cs;
	std::vector<world_thing_70> thing_70s;
	std::vector<uint32_t> thing_74s; // Unknown type.
	std::vector<regular_spline_entity> splines;
	std::vector<grindrail_spline_entity> grindrails;
	std::vector<uint8_t> thing_80_1;
	std::vector<uint8_t> thing_80_2;
	std::vector<world_thing_84> thing_84s;
	std::vector<uint8_t> thing_88;
	std::vector<world_thing_8c> thing_8cs;
	std::vector<world_thing_90> thing_90_1s;
	std::vector<world_thing_90> thing_90_2s;
	std::vector<world_thing_90> thing_90_3s;
	std::vector<std::vector<uint8_t>> thing_94s;
	std::vector<world_thing_98> thing_98_1s;
	std::vector<uint32_t> thing_98_2s;
	std::array<uint32_t, 5> thing_98_part_offsets;

	stream* backing;
	void read_rac23();
	void read_rac4();
	
private:
	template <typename T_1, typename T_2 = char, typename T_3 = char>
	void read_table( // Defined in world.cpp.
		uint32_t offset,
		std::vector<T_1>* first,
		std::vector<T_2>* second = nullptr,
		std::vector<T_3>* third = nullptr,
		bool align = false);
	std::vector<game_string> read_language(uint32_t offset);
	std::vector<uint32_t> read_u32_list(uint32_t offset);
	template <typename T_in_mem, typename T_on_disc>
	std::vector<T_in_mem> read_entity_table( // Defined in world.cpp.
		uint32_t offset, std::function<void(T_in_mem&, T_on_disc&)> swap_ent);
	std::vector<std::vector<uint8_t>> read_pvars(uint32_t table_offset, uint32_t data_offset);
	template <typename T>
	void read_terminated_array(std::vector<T>& dest, uint32_t offset); // Defined in world.cpp.
	template <typename T> // Either regular_spline_entity or grindrail_spline_entity.
	std::vector<T> read_splines( // Defined in world.cpp.
		uint32_t table_offset,
		uint32_t table_count,
		uint32_t data_offset);
	
public:
	void write_rac2();
	
private:
	std::size_t _next_entity_id = 1;
};

// Swaps data between the on-disc and in-memory representation of entities.
// These functions can hence be used for both reading and writing.
void swap_tie(tie_entity& l, world_tie& r);
void swap_shrub(shrub_entity& l, world_shrub& r);
void swap_moby_rac23(moby_entity& l, world_moby_rac23& r);
void swap_moby_rac4(moby_entity& l, world_moby_rac4& r);
void swap_trigger(trigger_entity& l, world_trigger& r);

#endif
