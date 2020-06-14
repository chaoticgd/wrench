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
		std::map<std::string, std::string>& game_paths,
		worker_logger& log,
		std::string game_id_)
	: _project_path(""),
	  _wrench_archive(nullptr),
	  game_id(game_id_),
	  _history_index(0),
	  _selected_level(nullptr),
	  _id(_next_id++),
	  iso(game_id, game_paths.at(game_id), log),
	  toc(read_toc(iso, TOC_BASE)) {
	load_tables();
}

wrench_project::wrench_project(
		std::map<std::string, std::string>& game_paths,
		std::string project_path,
		worker_logger& log)
	: _project_path(project_path),
	  _wrench_archive(ZipFile::Open(project_path)),
	  game_id(read_game_id()),
	  _history_index(0),
	  _id(_next_id++),
	  iso(game_id, game_paths.at(game_id), log, _wrench_archive),
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
	dialog->on_okay([=](app& a, std::string path) {
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

std::map<std::string, std::vector<texture>*> wrench_project::texture_lists() {
	std::map<std::string, std::vector<texture>*> result;
	for(auto& lvl : _levels) {
		result[lvl.first + "/Terrain"] = &lvl.second->terrain_textures;
		result[lvl.first + "/Ties"] = &lvl.second->tie_textures;
		//result[lvl.first + "/Mobies"] = &lvl.second->moby_textures;
		result[lvl.first + "/Sprites"] = &lvl.second->sprite_textures;
	}
	for(auto& wad : _texture_wads) {
		result[int_to_hex(wad.first)] = &wad.second;
	}
	for(auto& armor : _armor) {
		result["ARMOR.WAD"] = (&armor.textures);
	}
	return result;
}

std::map<std::string, std::vector<game_model>*> wrench_project::model_lists() {
	std::map<std::string, std::vector<game_model>*> result;
	for(auto& armor : _armor) {
		result["ARMOR.WAD"] = &armor.models;
	}
	for(auto& lvl : _levels) {
		result[lvl.first + "/Mobies"] = &lvl.second->moby_models;
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

	std::stringstream game_id_stream;
	game_id_stream << game_id;
	root->CreateEntry("game_id")->SetCompressionStream(game_id_stream);

	iso.save_patches_to_and_close(root, path);
}

/*
	private
*/

void wrench_project::load_tables() {
	for(toc_table& table : toc.tables) {
		armor_archive armor;
		if(armor.read(iso, table)) {
			_armor.push_back(armor);
			continue;
		}
		
		std::vector<texture> textures = enumerate_fip_textures(iso, table);
		if(textures.size() > 0) {
			_texture_wads[table.header.base_offset.bytes()] = textures;
			continue;
		}
		
		fprintf(stderr, "warning: File at iso+0x%08x ignored.\n", table.header.base_offset.bytes());
	}
}

std::string wrench_project::read_game_id() {
	auto entry = _wrench_archive->GetEntry("game_id");
	auto stream = entry->GetDecompressionStream();
	std::string result;
	std::getline(*stream, result);
	return result;
}

int wrench_project::_next_id = 0;
