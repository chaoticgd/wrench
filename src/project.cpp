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
#include "build.h"

iso_views::iso_views(stream* iso_file, worker_logger& log)
	: space_wad(iso_file, 0x7e041800, 0x10fa980, "SPACE.WAD",  log),
	  armor_wad(iso_file, 0x7fa3d800, 0x25d930,  "ARMOR.WAD",  log) {
	levels[4] = std::make_unique<level_impl>(iso_file, 0x8d794800, 0x17999dc, "LEVEL4.WAD", log);
}

wrench_project::wrench_project(std::string iso_path, worker_logger& log, std::string game_id)
	: _wratch_path(""),
	  _wratch_archive(nullptr),
	  _game_id(game_id),
	  _iso(game_id, iso_path, log),
	  views(&_iso, log) {}

wrench_project::wrench_project(std::string iso_path, std::string wratch_path, worker_logger& log)
	: _wratch_path(wratch_path),
	  _wratch_archive(ZipFile::Open(wratch_path)),
	  _game_id(read_game_id()),
	  _iso(_game_id, iso_path, log, _wratch_archive),
	  views(&_iso, log) {
	ZipFile::SaveAndClose(_wratch_archive, wratch_path);
	_wratch_archive = nullptr;
}

void wrench_project::save(app* a) {
	if(_wratch_path == "") {
		save_as(a);
	} else {
		save_to(_wratch_path);
	}
}

void wrench_project::save_as(app* a) {
	auto dialog = a->emplace_window<gui::string_input>("Save Project");
	dialog->on_okay([=](app& a, std::string path) {
		_wratch_path = path;
		save_to(path);
	});
}

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
	game_id_stream << _game_id;
	root->CreateEntry("game_id")->SetCompressionStream(game_id_stream);

	_iso.commit();
	_iso.save_patches(root, _wratch_path); // Also closes the archive.
}

std::string wrench_project::read_game_id() {
	auto entry = _wratch_archive->GetEntry("game_id");
	auto stream = entry->GetDecompressionStream();
	std::string result;
	std::getline(*stream, result);
	return result;
}
