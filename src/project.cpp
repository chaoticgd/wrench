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

#include <ZipLib/ZipArchive.h>
#include <ZipLib/ZipFile.h>

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
	: _project_path(""),
	  _wrench_archive(nullptr),
	  game(game_),
	  _history_index(0),
	  _selected_level(nullptr),
	  _id(_next_id++),
	  iso(game.md5, game.path, log),
	  toc(read_toc(iso, TOC_BASE)) {
	load_tables();
}

wrench_project::wrench_project(
		std::vector<game_iso> games,
		std::string project_path,
		worker_logger& log)
	: _project_path(project_path),
	  _wrench_archive(ZipFile::Open(project_path)),
	  game(read_game_type(games)),
	  _history_index(0),
	  _id(_next_id++),
	  iso(game.md5, game.path, log, _wrench_archive),
	  toc(read_toc(iso, TOC_BASE)) {
	ZipFile::SaveAndClose(_wrench_archive, project_path);
	_wrench_archive = nullptr;
	load_tables();
}

std::string wrench_project::project_path() const {
	return _project_path;
}
	
std::string wrench_project::cached_iso_path() const {
	return iso.cached_iso_path();
}

void wrench_project::save(app* a, std::function<void()> on_done) {
	if(_project_path == "") {
		save_as(a, on_done);
	} else {
		save_to(_project_path);
		on_done();
	}
}

void wrench_project::save_as(app* a, std::function<void()> on_done) {
	auto dialog = a->emplace_window<gui::string_input>("Save Project");
	dialog->on_okay([this, on_done](app& a, std::string path) {
		_project_path = path;
		save_to(path);
		on_done();
	});
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
		result[name + "/Ties"] = &lvl->tie_textures;
		//result[name + "/Mobies"] = &lvl->moby_textures;
		result[name + "/Sprites"] = &lvl->sprite_textures;
	}
	for(auto& [table_index, wad] : _texture_wads) {
		result[table_index_to_name(table_index)] = &wad;
	}
	for(auto& [table_index, armor] : _armor) {
		result[table_index_to_name(table_index)] = &armor.textures;
	}
	return result;
}

std::map<std::string, std::vector<game_model>*> wrench_project::model_lists(app* a) {
	if(!_game_info) {
		load_gamedb_info(a);
	}
	
	std::map<std::string, std::vector<game_model>*> result;
	for(auto& [table_index, armor] : _armor) {
		result[table_index_to_name(table_index)] = &armor.models;
	}
	for(auto& [level_index, lvl] : _levels) {
		result[level_index_to_name(level_index) + "/Mobies"] = &lvl->moby_models;
	}
	return result;
}

void wrench_project::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[_history_index - 1]->undo(this);
	_history_index--;
}

void wrench_project::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index]->apply(this);
	_history_index++;
}

void wrench_project::open_level(std::size_t index) {
	if(_levels.find(index) == _levels.end()) {
		// The level is not already open.
		_levels.emplace(index, std::make_unique<level>(&iso, toc.levels[index]));
	}
	_selected_level = _levels.at(index).get();
}

int wrench_project::id() {
	return _id;
}

void wrench_project::save_to(std::string path) {
	if(fs::exists(path)) {
		fs::remove(path + ".old");
		fs::rename(path, path + ".old");
	}

	auto root = ZipArchive::Create();

	std::stringstream version_stream;
	version_stream << WRENCH_VERSION_STR;
	root->CreateEntry("application_version")->SetCompressionStream(version_stream);

	std::stringstream game_md5_stream;
	game_md5_stream << game.md5;
	root->CreateEntry("game_md5")->SetCompressionStream(game_md5_stream);

	iso.save_patches_to_and_close(root, path);
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
		
		std::vector<texture> textures = enumerate_fip_textures(iso, table);
		if(textures.size() > 0) {
			_texture_wads[i] = textures;
			continue;
		}
		
		fprintf(stderr, "warning: File at iso+0x%08x ignored.\n", table.header.base_offset.bytes());
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

game_iso wrench_project::read_game_type(std::vector<game_iso> games) {
	auto entry = _wrench_archive->GetEntry("game_md5");
	if(entry == nullptr) {
		throw std::runtime_error("Wrench project does not contain game_md5 file!");
	}
	auto stream = entry->GetDecompressionStream();
	std::string result;
	std::getline(*stream, result);
	std::optional<game_iso> game;
	for(game_iso current : games) {
		if(current.md5 == result) {
			game = current;
		} 
	}
	if(!game) {
		throw std::runtime_error("Unknown game hash!");
	}
	return *game;
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
