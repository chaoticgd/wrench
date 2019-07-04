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
#include "stream.h"
#include "renderer.h"
#include "worker_thread.h"
#include "level.h"

app::app()
	: selection(nullptr),
	  reflector(std::make_unique<gui::inspector_reflector>(&selection)) {}

bool app::has_iso() const {
	return _iso.get() != nullptr;
}

void app::bind_iso(std::function<void(stream&)> callback) {
	if(has_iso()) {
		callback(*_iso.get());
	}
}

void app::open_iso(std::string path) {
	_iso = std::make_unique<iso_stream>(path);
}

bool app::has_level() const {
	return has_iso() && _iso->children_of_type<level>().size() > 0;
}

void app::bind_level(std::function<void(level&)> callback) {
	if(has_level()) {
		callback(*_iso->children_of_type<level>()[0]);
	}
}

void app::bind_level(std::function<void(const level&)> callback) const {
	if(has_level()) {
		callback(*_iso->children_of_type<level>()[0]);
	}
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
