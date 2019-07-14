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
	: selection(nullptr) {}

level* app::get_level() {
	if(_level) {
		return &_level.value();
	} else {
		return nullptr;
	}
}

void app::open_iso(std::string path) {
	_iso.emplace(path);
	_level.emplace(&_iso.value(), 0x8d794800, 0x17999dc, "LEVEL4.WAD");

	_space_wad.emplace(&_iso.value(), 0x7e041800, 0x10fa980, "SPACE.WAD");
	_armor_wad.emplace(&_iso.value(), 0x7fa3d800, 0x25d930, "ARMOR.WAD");
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
	if(auto lvl = get_level()) {
		result.push_back(lvl->get_texture_provider());
	}
	if(_space_wad) {
		result.push_back(&_space_wad.value());
	}
	if(_armor_wad) {
		result.push_back(&_armor_wad.value());
	}
	return result;
}
