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
#include "world.h"
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

enum class level_type {
	RAC23, RAC2_68, RAC4
};

struct level_file_header {
	level_type type;
	uint32_t base_offset;
	uint32_t level_number;
	uint32_t primary_header_offset;
	uint32_t world_segment_offset;
};

struct level_code_segment {
	level_code_segment_header header;
	std::vector<uint8_t> bytes;
};

struct level_primary_header {
	uint32_t code_segment_offset;
	uint32_t code_segment_size;
	uint32_t asset_header;
	uint32_t asset_header_size;
	uint32_t tex_pixel_data_base;
	uint32_t hud_header_offset;
	uint32_t hud_bank_0_offset;
	uint32_t hud_bank_0_size;
	uint32_t hud_bank_1_offset;
	uint32_t hud_bank_1_size;
	uint32_t hud_bank_2_offset;
	uint32_t hud_bank_2_size;
	uint32_t hud_bank_3_offset;
	uint32_t hud_bank_3_size;
	uint32_t hud_bank_4_offset;
	uint32_t hud_bank_4_size;
	uint32_t asset_wad;
	uint32_t loading_screen_textures_offset;
	uint32_t loading_screen_textures_size;
};

class level {
public:
	level(iso_stream* iso, toc_level index);
	level(const level& rhs) = delete;
	
	void write();
	
	static level_file_header read_file_header(stream* src, std::size_t offset);
	
	world_segment world;
	
	template <typename T, typename F>
	void for_each(F callback) {
		for_each_if_is_base_of<T, tie_entity>(callback, world.ties);
		for_each_if_is_base_of<T, shrub_entity>(callback, world.shrubs);
		for_each_if_is_base_of<T, moby_entity>(callback, world.mobies);
		for_each_if_is_base_of<T, trigger_entity>(callback, world.triggers);
		for_each_if_is_base_of<T, regular_spline_entity>(callback, world.splines);
		for_each_if_is_base_of<T, grindrail_spline_entity>(callback, world.grindrails);
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
	
private:
	void read_moby_models(std::size_t asset_offset, level_asset_header asset_header);
	void read_textures(std::size_t asset_offset, level_asset_header asset_header);
	void read_tfrags();
	
	void read_hud_banks(iso_stream* iso);
	void read_loading_screen_textures(iso_stream* iso);
	
public:

	stream* moby_stream();
	
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
	
	stream* _asset_segment;
	
	std::optional<trace_stream> _world_segment_tracepoint;
	std::optional<trace_stream> _asset_segment_tracepoint;
};

void swap_primary_header_rac23(level_primary_header& l, level_primary_header_rac23& r);
void swap_primary_header_rac4(level_primary_header& l, level_primary_header_rac4& r);

#endif
