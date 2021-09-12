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

#ifndef FORMATS_LEVEL_IMPL_H
#define FORMATS_LEVEL_IMPL_H

#include <memory>
#include <optional>
#include <stdint.h>

#include "../../core/level.h"
#include "../util.h"
#include "../stream.h"
#include "../iso_stream.h"
#include "../fs_includes.h"
#include "../worker_logger.h"
#include "../level_file_types.h"
#include "tfrag.h"
#include "tcol.h"
#include "texture.h"
#include "game_model.h"
#include "shrub.h"
#include "level_types.h"

enum class level_type : uint32_t {
	RAC23 = 0x60, RAC2_68 = 0x68, RAC4 = 0xc68
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
	byte_range instances_wad;
};

class level {
public:
	level() {}
	level(const level& rhs) = delete;
	level(level&& rhs) = default;
	
	void open(fs::path json_path, Json& json);
	void save();
	
	fs::path path;
	Game game;
	
	Gameplay& gameplay() { return _gameplay; }
	
private:
	void read_primary(fs::path bin_path);

	void read_moby_models(std::size_t asset_offset, level_asset_header asset_header);
	void read_shrub_models(std::size_t asset_offset, level_asset_header asset_header);
	void read_textures(std::size_t asset_offset, level_asset_header asset_header, bool is_deadlocked);
	void read_tfrags();
	void read_tcol(std::size_t asset_offset, level_asset_header asset_header);
	
public:
	// Asset segment
	std::map<uint32_t, std::size_t> moby_class_to_model;
	std::map<uint32_t, std::size_t> shrub_class_to_model;
	std::vector<moby_model> moby_models;
	std::vector<shrub_model> shrub_models;
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
	Gameplay _gameplay;

	level_primary_header _primary_header;
	std::optional<file_stream> _primary;
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

void swap_primary_header_rac23(level_primary_header& l, level_primary_header_rac23& r);
void swap_primary_header_rac4(level_primary_header& l, level_primary_header_rac4& r);

#endif
