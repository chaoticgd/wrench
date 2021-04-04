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
#include "../iso_stream.h"
#include "../fs_includes.h"
#include "../worker_logger.h"
#include "../level_file_types.h"
#include "world.h"
#include "wad.h"
#include "tfrag.h"
#include "tcol.h"
#include "texture.h"
#include "game_model.h"
#include "level_types.h"

# /*
#	Read LEVEL*.WAD files.
# */

enum class level_type : uint32_t {
	RAC23 = 0x60, RAC2_68 = 0x68, RAC4 = 0xc68
};

struct level_file_header {
	level_type type;
	sector32 base_offset;
	uint32_t level_number;
	uint32_t unknown_c;
	sector_range primary_header;
	sector_range sound_bank_1;
	sector_range world_segment;
	sector_range unknown_28;
	sector_range unknown_30;
	sector_range unknown_38;
	sector_range unknown_40;
	sector_range sound_bank_2;
	sector_range sound_bank_3;
	sector_range sound_bank_4;
};

struct level_code_segment {
	level_code_segment_header header;
	std::vector<uint8_t> bytes;
};

struct level_primary_header {
	byte_range unknown_0;
	byte_range code_segment;
	byte_range asset_header;
	byte_range small_textures;
	byte_range hud_header;
	byte_range hud_bank_0;
	byte_range hud_bank_1;
	byte_range hud_bank_2;
	byte_range hud_bank_3;
	byte_range hud_bank_4;
	byte_range asset_wad;
	byte_range loading_screen_textures;
};

class level {
public:
	level() {}
	level(const level& rhs) = delete;
	level(level&& rhs) = default;
	
	void read(stream& src, fs::path path_);
	
	void read_file_header(stream& file);
	
	fs::path path;
	level_file_header file_header;
	level_file_info info; // Stores information derived from the magic identifier e.g. header size.
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
	
	template <typename T>
	T* entity_from_id(entity_id id) {
		T* result = nullptr;
		for_each<T>([&](T& ent) {
			if(ent.id == id) {
				result = &ent;
			}
		});
		return result;
	}
	
private:
	void read_moby_models(std::size_t asset_offset, level_asset_header asset_header);
	void read_textures(std::size_t asset_offset, level_asset_header asset_header);
	void read_tfrags();
	void read_tcol(std::size_t asset_offset, level_asset_header asset_header);
	
	void read_hud_banks(stream* file);
	void read_loading_screen_textures(stream* file);
	
public:
	void write(array_stream& dest);

	stream* moby_stream();
	
	// Asset segment
	std::map<uint32_t, std::size_t> moby_class_to_model;
	std::vector<moby_model> moby_models;
	std::vector<texture> mipmap_textures;
	std::vector<texture> tfrag_textures;
	std::vector<texture> moby_textures;
	std::vector<texture> tie_textures;
	std::vector<texture> shrub_textures;
	std::vector<texture> sprite_textures;
	std::vector<tfrag> tfrags;
	std::vector<tcol> baked_collisions;
	
	level_code_segment code_segment;
	
	std::vector<texture> loading_screen_textures;
	
private:
	level_primary_header _primary_header;
	std::optional<array_stream> _file;
	std::optional<simple_wad_stream> _world_segment;
	std::optional<simple_wad_stream> _asset_segment;

public:
	void push_command(std::function<void(level&)> apply, std::function<void(level&)> undo);
	void undo();
	void redo();
private:
	struct undo_redo_command {
		std::function<void(level& lvl)> apply;
		std::function<void(level& lvl)> undo;
	};

	std::size_t _history_index = 0;
	std::vector<undo_redo_command> _history_stack;
};

class command_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

void swap_level_file_header_rac23(level_file_header& l, level_file_header_rac23& r);
void swap_level_file_header_rac2_68(level_file_header& l, level_file_header_rac2_68& r);
void swap_level_file_header_rac4(level_file_header& l, level_file_header_rac4& r);

void swap_primary_header_rac23(level_primary_header& l, level_primary_header_rac23& r);
void swap_primary_header_rac4(level_primary_header& l, level_primary_header_rac4& r);

#endif
