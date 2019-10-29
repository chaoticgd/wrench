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

wrench_project::wrench_project(std::string iso_path, worker_logger& log, std::string game_id)
	: _project_path(""),
	  _wratch_archive(nullptr),
	  _game_id(game_id),
	  _history_index(0),
	  _selected_level(nullptr),
	  _id(_next_id++),
	  iso(game_id, iso_path, log) {}

wrench_project::wrench_project(std::string iso_path, std::string project_path, worker_logger& log)
	: _project_path(project_path),
	  _wratch_archive(ZipFile::Open(project_path)),
	  _game_id(read_game_id()),
	  _history_index(0),
	  _id(_next_id++),
	  iso(_game_id, iso_path, log, _wratch_archive) {
	ZipFile::SaveAndClose(_wratch_archive, project_path);
	_wratch_archive = nullptr;
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

void wrench_project::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[--_history_index]->undo(this);
}

void wrench_project::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index++]->apply(this);
}

/*
	views
*/

std::vector<std::string> wrench_project::available_view_types() {
	std::vector<std::string> result;
	for(auto& group : _views.at(_game_id)) {
		result.push_back(group.first);
	}
	return result;
}

std::vector<std::string> wrench_project::available_views(std::string group) {
	std::vector<std::string> result;
	for(auto& view : _views.at(_game_id).at(group)) {
		result.push_back(view.first);
	}
	return result;
}

void wrench_project::select_view(std::string group, std::string view) {
	_next_view_name = view;
	_views.at(_game_id).at(group).at(view)(this);
}

racpak* wrench_project::open_archive(std::size_t offset, std::size_t size) {
	if(_archives.find(offset) == _archives.end()) {
		_archives.emplace(offset, std::make_unique<racpak>(&iso, offset, size));
	}
	
	return _archives.at(offset).get();
}

void wrench_project::open_texture_archive(std::size_t offset, std::size_t size) {
	if(_texture_wads.find(offset) != _texture_wads.end()) {
		// The archive is already open.
		return;
	}
	
	racpak* archive = open_archive(offset, size);
	worker_logger log;
	_texture_wads.emplace(offset, std::make_unique<racpak_fip_scanner>
		(&iso, archive, _next_view_name, log));
}

void wrench_project::open_level(std::size_t offset, std::size_t size) {
	if(_levels.find(offset) == _levels.end()) {
		// The level is not already open.
		racpak* archive = open_archive(offset, size);
		worker_logger log;
		_levels.emplace(offset, std::make_unique<level>
			(&iso, archive, _next_view_name, log));
	}
	_selected_level = _levels.at(offset).get();
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
	game_id_stream << _game_id;
	root->CreateEntry("game_id")->SetCompressionStream(game_id_stream);

	iso.save_patches_to_and_close(root, _project_path);
}

std::string wrench_project::read_game_id() {
	auto entry = _wratch_archive->GetEntry("game_id");
	auto stream = entry->GetDecompressionStream();
	std::string result;
	std::getline(*stream, result);
	return result;
}

const std::map<std::string, wrench_project::game_view> wrench_project::_views {
	{"rc2pal", {
		{"Textures", {
			{"HUD.WAD",   [](wrench_project* p) { p->open_texture_archive(0x7360f800, 0x242b30f); }},
			{"BONUS.WAD", [](wrench_project* p) { p->open_texture_archive(0x75a3b000, 0x1e49ea5); }},
			{"SPACE.WAD", [](wrench_project* p) { p->open_texture_archive(0x7e041800, 0x10fa980); }},
			{"ARMOR.WAD", [](wrench_project* p)
				{ p->_armor = std::make_optional<armor_archive>(&p->iso, 0x7fa3d800, 0x25d930); }}
		}},
		{"Levels", {
			{"00 Aranos",             [](wrench_project* p) { p->open_level(0x7fc9b800, 0xec5a80 ); }},
			{"01 Oozla",              [](wrench_project* p) { p->open_level(0x81be9000, 0x14dea44); }},
			{"02 Maktar",             [](wrench_project* p) { p->open_level(0x85104800, 0x13c73f8); }},
			{"03 Endako",             [](wrench_project* p) { p->open_level(0x89d86000, 0x10d9060); }},
			{"04 Barlow",             [](wrench_project* p) { p->open_level(0x8d794800, 0x17999dc); }},
			{"05 Feltzin",            [](wrench_project* p) { p->open_level(0x92064800, 0xff02d0 ); }},
			{"06 Notak",              [](wrench_project* p) { p->open_level(0x93bd9000, 0x12893e0); }},
			{"07 Siberius",           [](wrench_project* p) { p->open_level(0x96690800, 0x1545530); }},
			{"08 Tabora",             [](wrench_project* p) { p->open_level(0x99685000, 0x13ac830); }},
			{"09 Dobbo",              [](wrench_project* p) { p->open_level(0x9e8a6000, 0x11f8ae0); }},
			{"10 Hrugis",            [](wrench_project* p) { p->open_level(0xa2317800, 0xdc0fe3 ); }},
			{"11 Joba",              [](wrench_project* p) { p->open_level(0xa3fdb000, 0x155e2a0); }},
			{"12 Todano",            [](wrench_project* p) { p->open_level(0xaa301000, 0x10c5a90); }},
			{"13 Boldan",            [](wrench_project* p) { p->open_level(0xae9f0000, 0x112bab0); }},
			{"14 Aranos 2",          [](wrench_project* p) { p->open_level(0xb228b800, 0x10947a0); }},
			{"15 Gorn",              [](wrench_project* p) { p->open_level(0xb59e0800, 0xd77dc0 ); }},
			{"16 Snivelak",          [](wrench_project* p) { p->open_level(0xb7243000, 0x1188be0); }},
			{"17 Smolg",             [](wrench_project* p) { p->open_level(0xbafab800, 0xfff020 ); }},
			{"18 Damosel",           [](wrench_project* p) { p->open_level(0xbde52800, 0x10e52c0); }},
			{"19 Grelbin",           [](wrench_project* p) { p->open_level(0xc05cf000, 0x18052d8); }},
			{"20 Yeedil",            [](wrench_project* p) { p->open_level(0xc59e2800, 0x1469a5c); }},
			{"21 Insomniac Museum",  [](wrench_project* p) { p->open_level(0xcaeba800, 0xcc30e0 ); }},
			{"22 Disposal Facility", [](wrench_project* p) { p->open_level(0xccc5a000, 0xd02530 ); }},
			{"23 Damosel Orbit",     [](wrench_project* p) { p->open_level(0xce839000, 0xc0b9d0 ); }},
			{"24 Ship Shack",        [](wrench_project* p) { p->open_level(0xcfb68000, 0x7fc176 ); }},
			{"25 Wupash",            [](wrench_project* p) { p->open_level(0xd0751000, 0x843c5f ); }},
			{"26 Jamming Array",     [](wrench_project* p) { p->open_level(0xd164e800, 0xa81290 ); }}
		}}
	}},
	{"rc3pal", {
		{"Textures", {
			{"ARMOR.WAD", [](wrench_project* p)
				{ p->_armor = std::make_optional<armor_archive>(&p->iso, 0x56da6000, 0x0); }}
		}},
		{"Levels", {
			{"00 Veldin",                  [](wrench_project* p) { p->open_level(0x92229000, 0x0); }},
			{"01 Florana",                 [](wrench_project* p) { p->open_level(0x9349d800, 0x0); }},
			{"02 Phoenix",                 [](wrench_project* p) { p->open_level(0x946c4800, 0x0); }},
			{"03 Marcadia",                [](wrench_project* p) { p->open_level(0x9576d000, 0x0); }},
			{"04 Daxx",                    [](wrench_project* p) { p->open_level(0x96bea800, 0x0); }},
			{"05 Phoenix (Under Attack)",  [](wrench_project* p) { p->open_level(0x97de8000, 0x0); }},
			{"06 Annihilation Nation",     [](wrench_project* p) { p->open_level(0x98d51000, 0x0); }},
			{"07 Aquatos",                 [](wrench_project* p) { p->open_level(0x9a048000, 0x0); }},
			{"08 Aquatos 2",               [](wrench_project* p) { p->open_level(0x9b2d3000, 0x0); }},
			{"09 Zeldrin",                 [](wrench_project* p) { p->open_level(0x9c7b2000, 0x0); }},
			{"10 Obani Moons",             [](wrench_project* p) { p->open_level(0x9dbc0800, 0x0); }},
			{"11 Blackwater",              [](wrench_project* p) { p->open_level(0x9ed0e800, 0x0); }},
			{"12 LEVEL12",                 [](wrench_project* p) { p->open_level(0x9fe31800, 0x0); }},
			{"13 LEVEL13",                 [](wrench_project* p) { p->open_level(0xa0ea3800, 0x0); }},
			{"14 Metropolis",              [](wrench_project* p) { p->open_level(0xa1fd4800, 0x0); }},
			{"15 Crash Site",              [](wrench_project* p) { p->open_level(0xa30d5000, 0x0); }},
			{"16 LEVEL16",                 [](wrench_project* p) { p->open_level(0xa41e5000, 0x0); }},
			{"17 LEVEL17",                 [](wrench_project* p) { p->open_level(0xa5362800, 0x0); }},
			{"18 Final Boss",              [](wrench_project* p) { p->open_level(0xa69fc000, 0x0); }},
			// There's something between these two.
			{"20 LEVEL20",                 [](wrench_project* p) { p->open_level(0xa881f000, 0x0); }},
			{"21 LEVEL21",                 [](wrench_project* p) { p->open_level(0xa98d9800, 0x0); }},
			{"22 LEVEL22",                 [](wrench_project* p) { p->open_level(0xaa921800, 0x0); }},
			{"23 LEVEL23",                 [](wrench_project* p) { p->open_level(0xabda5000, 0x0); }},
			{"24 LEVEL24",                 [](wrench_project* p) { p->open_level(0xaced4000, 0x0); }},
			{"25 Aquatos Sewers",          [](wrench_project* p) { p->open_level(0xadcb9800, 0x0); }},
			{"26 LEVEL26",                 [](wrench_project* p) { p->open_level(0xaebb6000, 0x0); }},
			{"27 Vid-Comic 1",             [](wrench_project* p) { p->open_level(0xafcca000, 0x0); }},
			{"28 Vid-Comic 2",             [](wrench_project* p) { p->open_level(0xb09a8000, 0x0); }},
			{"29 Vid-Comic 3",             [](wrench_project* p) { p->open_level(0xb15d0000, 0x0); }},
			{"30 Vid-Comic 4",             [](wrench_project* p) { p->open_level(0xb2349800, 0x0); }},
			{"31 Vid-Comic 5",             [](wrench_project* p) { p->open_level(0xb300d000, 0x0); }},
			{"32 Vid-Comic 6",             [](wrench_project* p) { p->open_level(0xb3ba7000, 0x0); }},
			{"33 Vid-Comic 7",             [](wrench_project* p) { p->open_level(0xb47e6800, 0x0); }},
			{"34 LEVEL34",                 [](wrench_project* p) { p->open_level(0xb5535000, 0x0); }},
			{"35 LEVEL35",                 [](wrench_project* p) { p->open_level(0xb5fe6000, 0x0); }},
			{"36 LEVEL36",                 [](wrench_project* p) { p->open_level(0xb70c8800, 0x0); }},
			{"37 LEVEL37",                 [](wrench_project* p) { p->open_level(0xb818b000, 0x0); }},
			{"38 LEVEL38",                 [](wrench_project* p) { p->open_level(0xb930d800, 0x0); }},
			{"39 LEVEL39",                 [](wrench_project* p) { p->open_level(0xba524000, 0x0); }},
			{"40 LEVEL40",                 [](wrench_project* p) { p->open_level(0xbb627800, 0x0); }},
			{"41 LEVEL41",                 [](wrench_project* p) { p->open_level(0xbc7ac800, 0x0); }},
			{"42 LEVEL42",                 [](wrench_project* p) { p->open_level(0xbd25f000, 0x0); }},
			{"43 MP Aquatos Sewers",       [](wrench_project* p) { p->open_level(0xbde82800, 0x0); }},
			{"44 LEVEL44",                 [](wrench_project* p) { p->open_level(0xbe98f000, 0x0); }},
			{"45 MP Bakisi Isles",         [](wrench_project* p) { p->open_level(0xbf612800, 0x0); }},
			{"46 LEVEL46",                 [](wrench_project* p) { p->open_level(0xc0626000, 0x0); }},
			{"47 LEVEL47",                 [](wrench_project* p) { p->open_level(0xc169e800, 0x0); }},
			{"48 LEVEL48",                 [](wrench_project* p) { p->open_level(0xc27f4800, 0x0); }},
			{"49 MP Metropolis",           [](wrench_project* p) { p->open_level(0xc38d2800, 0x0); }},
			{"50 LEVEL50",                 [](wrench_project* p) { p->open_level(0xc49a2000, 0x0); }}
		}}
	}}
};

int wrench_project::_next_id = 0;
