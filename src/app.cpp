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

app::app()
	: selection(nullptr),
	  reflector(std::make_unique<inspector_reflector>(&selection)) {}

level* app::get_level() {
	if(_level) {
		return &_level.value();
	} else {
		return nullptr;
	}
}

void app::open_iso(std::string path) {
	_iso.emplace<file_stream>(path);
	level_impl impl(&_iso.value(), 0x8d794800, 0x17999dc); // LEVEL4.WAD
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
