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

#ifndef FORMATS_LEVEL_IMPL_H
#define FORMATS_LEVEL_IMPL_H

#include <memory>
#include <optional>
#include <stdint.h>

#include "../util.h"
#include "../stream.h"
#include "../worker_logger.h"
#include "toc.h"
#include "wad.h"
#include "tfrag.h"
#include "racpak.h"
#include "texture.h"
#include "game_model.h"
#include "level_types.h"

# /*
#	Read LEVEL*.WAD files.
# */

struct level_code_segment {
	level_code_segment_header header;
	std::vector<uint8_t> bytes;
};

struct game_string {
	uint32_t id;
	uint32_t secondary_id;
	std::string str;
};

struct entity_id {
	std::size_t value;
	
	bool operator<(const entity_id& rhs) const {
		return value < rhs.value;
	}
	
	bool operator==(const entity_id& rhs) const {
		return value == rhs.value;
	}
};

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

struct spline_entity final : entity {
	std::vector<glm::vec4> vertices;
};

struct thing_7c_2 {
	std::vector<glm::vec4> vertices;
};

class level {
public:
	level(iso_stream* iso, toc_level index);
	level(const level& rhs) = delete;
	
	void write();
private:
	void read_world_segment();
	void write_world_segment();
	
	void read_moby_models(std::size_t asset_offset, level_asset_header asset_header);
	void read_textures(std::size_t asset_offset, level_asset_header asset_header);
	void read_tfrags();
	
	void read_hud_banks(iso_stream* iso);
	void read_loading_screen_textures(iso_stream* iso);
	
public:

	stream* moby_stream();
	
	// World segment
	world_properties properties;
	std::vector<world_property_thing> property_things;
	std::vector<world_directional_light> directional_lights;
	std::vector<world_thing_8> thing_8s;
	std::vector<world_thing_c> thing_cs;
	std::array<std::vector<game_string>, 7> game_strings;
	world_thing_14 thing_14;
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
	std::vector<spline_entity> splines;
	std::vector<world_thing_7c_1> thing_7c_1s;
	std::vector<thing_7c_2> thing_7c_2s;
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
	uint32_t thing_98_header_8; // Not sure what this is.
	uint32_t thing_98_header_c; // Not sure what this is.
	
	template <typename T, typename F>
	void for_each(F callback) {
		for_each_if_is_base_of<T, tie_entity>(callback, ties);
		for_each_if_is_base_of<T, shrub_entity>(callback, shrubs);
		for_each_if_is_base_of<T, moby_entity>(callback, mobies);
		for_each_if_is_base_of<T, spline_entity>(callback, splines);
	}
	
	template<typename Base, typename Derived, typename F>
	void for_each_if_is_base_of(F callback, std::vector<Derived>& entities) {
		if constexpr(std::is_base_of_v<Base, Derived>) {
			for(auto& ent : entities) {
				callback(ent);
			}
		}
	}
	
	void clear_selection();
	std::vector<entity_id> selected_entity_ids();
	
	// Asset segment
	std::map<uint32_t, std::size_t> moby_class_to_model;
	std::vector<moby_model> moby_models;
	std::vector<texture> mipmap_textures;
	std::vector<texture> terrain_textures;
	std::vector<texture> moby_textures;
	std::vector<texture> tie_textures;
	std::vector<texture> shrub_textures;
	std::vector<texture> sprite_textures;
	std::vector<tfrag> tfrags;
	
	level_code_segment code_segment;
	
	std::vector<texture> loading_screen_textures;
	
private:
	toc_level _index;
	level_file_header _file_header;
	level_primary_header _primary_header;
	proxy_stream _file;
	
	stream* _world_segment;
	std::size_t _next_entity_id = 1;
	
	stream* _asset_segment;
	
	std::optional<trace_stream> _world_segment_tracepoint;
	std::optional<trace_stream> _asset_segment_tracepoint;
};

// We're using packed structs so we can't pass around references to fields so
// instead of std::swap we have to use this macro.
#define SWAP_PACKED(inmem, packed) \
	{ \
		auto p = packed; \
		packed = inmem; \
		inmem = p; \
	}

// Swaps data between the on-disc and in-memory representation of entities.
// These functions can hence be used for both reading and writing.
void swap_tie(tie_entity& l, world_tie& r);
void swap_shrub(shrub_entity& l, world_shrub& r);
void swap_moby(moby_entity& l, world_moby& r);
void swap_trigger(trigger_entity& l, world_trigger& r);

#endif
