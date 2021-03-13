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

#include "project.h"

#include "app.h"
#include "gui.h"
#include "config.h"
#include "fs_includes.h"
#include "formats/texture_archive.h"

// This is true for R&C2 and R&C3.
static const std::size_t TOC_BASE = 0x1f4800;

wrench_project::wrench_project(
		game_iso game_,
		worker_logger& log)
	: game(game_),
	  _id(_next_id++),
	  iso(game.path),
	  toc(read_toc(iso, TOC_BASE)) {
	if(!read_iso_filesystem(_root_directory, iso)) {
		throw stream_format_error("Invalid or missing ISO filesystem!");
	}
	load_tables();
}

void wrench_project::post_load() {
	for(auto& [_, armor] : _armor) {
		for(texture& tex : armor.textures) {
			tex.upload_to_opengl();
		}
	}
}
	
std::string wrench_project::cached_iso_path() const {
	return "cache/debug.iso";
}

level* wrench_project::selected_level() {
	for(auto& level : _levels) {
		if(level.second.get() == _selected_level) {
			return _selected_level;
		}
	}
	return nullptr;
}

std::size_t wrench_project::selected_level_index() {
	for(auto& [index, level] : _levels) {
		if(level.get() == _selected_level) {
			return index;
		}
	}
	return -1; 
}

std::vector<level*> wrench_project::levels() {
	std::vector<level*> result(_levels.size());
	std::transform(_levels.begin(), _levels.end(), result.begin(),
		[](auto& level) { return level.second.get(); });
	return result;
}

level* wrench_project::level_from_index(std::size_t index) {
	if(_levels.find(index) == _levels.end()) {
		return nullptr;
	}
	return _levels.at(index).get();
}

std::map<std::string, std::vector<texture>*> wrench_project::texture_lists(app* a) {
	if(!_game_info) {
		load_gamedb_info(a);
	}
	
	std::map<std::string, std::vector<texture>*> result;
	for(auto& [index, lvl] : _levels) {
		std::string name = level_index_to_name(index);
		result[name + "/Mipmaps"] = &lvl->mipmap_textures;
		result[name + "/Terrain"] = &lvl->terrain_textures;
		result[name + "/Mobies"] = &lvl->moby_textures;
		result[name + "/Ties"] = &lvl->tie_textures;
		result[name + "/Shrubs"] = &lvl->shrub_textures;
		result[name + "/Sprites"] = &lvl->sprite_textures;
		result[name + "/Loading Screen"] = &lvl->loading_screen_textures;
	}
	for(auto& [table_index, wad] : _texture_wads) {
		result[table_index_to_name(table_index)] = &wad;
	}
	for(auto& [table_index, armor] : _armor) {
		result[table_index_to_name(table_index)] = &armor.textures;
	}
	return result;
}

std::map<std::string, model_list> wrench_project::model_lists(app* a) {
	if(!_game_info) {
		load_gamedb_info(a);
	}
	
	std::map<std::string, model_list> result;
	for(auto& [table_index, armor] : _armor) {
		result[table_index_to_name(table_index)] = {
			&armor.models, &armor.textures
		};
	}
	for(auto& [level_index, lvl] : _levels) {
		result[level_index_to_name(level_index) + "/Mobies"] = {
			&lvl->moby_models, &lvl->moby_textures
		};
	}
	return result;
}

void wrench_project::push_command(std::function<void()> apply, std::function<void()> undo) {
	_history_stack.resize(_history_index++);
	_history_stack.emplace_back(undo_redo_command { apply, undo });
	apply();
}

void wrench_project::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[_history_index - 1].undo();
	_history_index--;
}

void wrench_project::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index].apply();
	_history_index++;
}

void wrench_project::clear_undo_history() {
	_history_stack.clear();
	_history_index = 0;
}

void wrench_project::open_level(std::size_t index) {
	if(_levels.find(index) == _levels.end()) {
		// The level is not already open.
		auto lvl = std::make_unique<level>();
		toc_level& header = toc.levels[index];
		sector32 base_offset = iso.read<sector32>(header.main_part.bytes() + 4);
		lvl->read(&iso, header, header.main_part.bytes(), base_offset, base_offset, header.main_part_size.bytes());
		_levels.emplace(index, std::move(lvl));
	}
	_selected_level = _levels.at(index).get();
}

int wrench_project::id() {
	return _id;
}

void wrench_project::write_iso_file() {
	array_stream dest;
	write_iso_filesystem(dest, _root_directory);
	
	for(iso_file_record& file : _root_directory) {
		while(dest.tell() < file.lba.bytes()) {
			dest.write<uint8_t>(0);
		}
		assert(dest.tell() == file.lba.bytes());
		iso.seek(file.lba.bytes());
		stream::copy_n(dest, iso, file.size);
	}
	dest.pad(SECTOR_SIZE, 0);
	
	file_stream output_file("cache/debug.iso", std::ios::out);
	dest.seek(0);
	stream::copy_n(output_file, dest, dest.size());
	
	iso.seek(output_file.size());
	stream::copy_n(output_file, iso, iso.size() - output_file.size());
	
	size_t vol_size = sector32::size_from_bytes(output_file.size()).sectors;
	output_file.write<uint32_t>(0x8050, vol_size);
}

/*
	private
*/

void wrench_project::load_tables() {
	for(std::size_t i = 0; i < toc.tables.size(); i++) {
		toc_table& table = toc.tables[i];
		
		armor_archive armor;
		if(armor.read(iso, table)) {
			_armor.emplace(i, std::move(armor));
			continue;
		}
		
		std::vector<texture> textures = enumerate_pif_textures(iso, table);
		if(textures.size() > 0) {
			_texture_wads[i] = std::move(textures);
			continue;
		}
		
		fprintf(stderr, "warning: File at iso+0x%08lx ignored.\n", table.header.base_offset.bytes());
	}
}

void wrench_project::load_gamedb_info(app* a) {
	for(std::size_t i = 0 ; i < a->game_db.size(); i++) {
		if(a->game_db[i].name == game.game_db_entry) {
			_game_info = a->game_db[i];
			return;
		}
	}
	throw std::runtime_error("Failed to load gamedb info!");
}

std::string wrench_project::table_index_to_name(std::size_t table_index) {
	if(_game_info->tables.find(table_index) == _game_info->tables.end()) {
		return int_to_hex(table_index);
	}
	return _game_info->tables.at(table_index);
}

std::string wrench_project::level_index_to_name(std::size_t level_index) {
	if(_game_info->levels.find(level_index) == _game_info->levels.end()) {
		return int_to_hex(level_index);
	}
	return _game_info->levels.at(level_index);
}

int wrench_project::_next_id = 0;
