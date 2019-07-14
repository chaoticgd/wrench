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

#include "app.h"

#include "gui.h"
#include "inspector.h"
#include "stream.h"
#include "renderer.h"
#include "worker_thread.h"

iso_adapters::iso_adapters(stream* iso_file, worker_logger& log)
	: level(    iso_file, 0x8d794800, 0x17999dc, "LEVEL4.WAD", log),
	  space_wad(iso_file, 0x7e041800, 0x10fa980, "SPACE.WAD",  log),
	  armor_wad(iso_file, 0x7fa3d800, 0x25d930,  "ARMOR.WAD",  log) {}

app::app()
	: mouse_last(0, 0),
	  mouse_diff(0, 0),
	  selection(nullptr) {}

level* app::get_level() {
	if(_iso_adapters.get() != nullptr) {
		return &_iso_adapters->level;
	} else {
		return nullptr;
	}
}

void app::open_iso(std::string path) {
	_iso.emplace(path);

	using worker_type = worker_thread<std::unique_ptr<iso_adapters>, file_stream*>;
	windows.emplace_back(std::make_unique<worker_type>(
		"ISO Importer", &_iso.value(),
		[](file_stream* iso, worker_logger& log) {
			auto result = std::make_unique<iso_adapters>(iso, log);
			log << "\nISO imported successfully.";
			return result;
		},
		[=](std::unique_ptr<iso_adapters> adapters) {
			_iso_adapters.swap(adapters);
		}
	));

	if(auto view = get_3d_view()) {
		(*view)->reset_camera(*this);
	}
}

const level* app::get_level() const {
	return const_cast<app*>(this)->get_level();
}

bool app::has_camera_control() {
	auto view = get_3d_view();
	if(!view) {
		return false;
	}
	return static_cast<three_d_view*>(*view)->camera_control;
}

std::optional<three_d_view*> app::get_3d_view() {
	for(auto& window : windows) {
		if(dynamic_cast<three_d_view*>(window.get()) != nullptr) {
			return dynamic_cast<three_d_view*>(window.get());
		}
	}
	return {};
}

std::vector<texture_provider*> app::texture_providers() {
	std::vector<texture_provider*> result;
	if(_iso_adapters.get() != nullptr) {
		result.push_back(_iso_adapters->level.get_texture_provider());
		result.push_back(&_iso_adapters->space_wad);
		result.push_back(&_iso_adapters->armor_wad);
	}
	return result;
}
