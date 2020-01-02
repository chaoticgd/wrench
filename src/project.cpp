/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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
#include <boost/filesystem.hpp>

#include "app.h"
#include "gui.h"
#include "config.h"

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
	  iso(game_id, game_paths.at(game_id), log) {}

wrench_project::wrench_project(
		std::map<std::string, std::string>& game_paths,
		std::string project_path,
		worker_logger& log)
	: _project_path(project_path),
	  _wrench_archive(ZipFile::Open(project_path)),
	  game_id(read_game_id()),
	  _history_index(0),
	  _id(_next_id++),
	  iso(game_id, game_paths.at(game_id), log, _wrench_archive) {
	ZipFile::SaveAndClose(_wrench_archive, project_path);
	_wrench_archive = nullptr;
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

std::vector<level*> wrench_project::levels() {
	std::vector<level*> result(_levels.size());
	std::transform(_levels.begin(), _levels.end(), result.begin(),
		[](auto& level) { return level.second.get(); });
	return result;
}

level* wrench_project::level_at(std::size_t offset) {
	if(_levels.find(offset) == _levels.end()) {
		return nullptr;
	}
	return _levels.at(offset).get();
}

std::vector<texture_provider*> wrench_project::texture_providers() {
	std::vector<texture_provider*> result;
	for(auto& level : _levels) {
		result.push_back(level.second->get_texture_provider());
	}
	for(auto& wad : _texture_wads) {
		result.push_back(wad.second.get());
	}
	if(_armor) {
		result.push_back(&(*_armor));
	}
	return result;
}

std::map<std::string, std::vector<game_model>*> wrench_project::model_lists() {
	std::map<std::string, std::vector<game_model>*> result;
	if(_armor) {
		result["Armor"] = &_armor->models;
	}
	for(auto& lvl : _levels) {
		result[std::to_string(lvl.first)] = &lvl.second->models;
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

void wrench_project::open_file(gamedb_file file) {
	switch(file.type) {
		case gamedb_file_type::TEXTURES:
			open_texture_archive(file);
			break;
		case gamedb_file_type::ARMOR:
			_armor = std::make_optional<armor_archive>(&iso, file.offset, file.size);
			break;
		case gamedb_file_type::LEVEL:
			open_level(file);
			break;
	}
}

racpak* wrench_project::open_archive(gamedb_file file) {
	if(_archives.find(file.offset) == _archives.end()) {
		_archives.emplace(file.offset, std::make_unique<racpak>(&iso, file.offset, file.size));
	}
	
	return _archives.at(file.offset).get();
}

void wrench_project::open_texture_archive(gamedb_file file) {
	if(_texture_wads.find(file.offset) != _texture_wads.end()) {
		// The archive is already open.
		return;
	}
	
	racpak* archive = open_archive(file);
	worker_logger log;
	_texture_wads.emplace(file.offset, std::make_unique<racpak_fip_scanner>
		(&iso, archive, file.name, log));
}

void wrench_project::open_level(gamedb_file file) {
	if(_levels.find(file.offset) == _levels.end()) {
		// The level is not already open.
		_levels.emplace(file.offset, std::make_unique<level>
			(&iso, file.offset, file.size, file.name));
	}
	_selected_level = _levels.at(file.offset).get();
}

int wrench_project::id() {
	return _id;
}

/*
	private
*/

void wrench_project::save_to(std::string path) {
	if(boost::filesystem::exists(path)) {
		boost::filesystem::remove(path + ".old");
		boost::filesystem::rename(path, path + ".old");
	}

	auto root = ZipArchive::Create();

	std::stringstream version_stream;
	version_stream << WRENCH_VERSION_STR;
	root->CreateEntry("application_version")->SetCompressionStream(version_stream);

	std::stringstream game_id_stream;
	game_id_stream << game_id;
	root->CreateEntry("game_id")->SetCompressionStream(game_id_stream);

	iso.save_patches_to_and_close(root, _project_path);
}

std::string wrench_project::read_game_id() {
	auto entry = _wrench_archive->GetEntry("game_id");
	auto stream = entry->GetDecompressionStream();
	std::string result;
	std::getline(*stream, result);
	return result;
}

int wrench_project::_next_id = 0;
