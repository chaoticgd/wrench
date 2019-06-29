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

#include "level.h"
	
bool level::undo() {
	if(_history_index <= 0) {
		return false;
	}
	_history[--_history_index]->undo();
	return true;
}

bool level::redo() {
	if(_history_index >= _history.size()) {
		return false;
	}
	_history[_history_index++]->apply();
	return true;
}

void level::reset_camera() {
	const auto& mobies = static_cast<level_impl*>(this)->mobies();
	glm::vec3 sum(0, 0, 0);
	for(const auto& [uid, moby] : mobies) {
		sum += moby->position();
	}
	camera_position = sum / static_cast<float>(mobies.size());
	camera_rotation = glm::vec3(0, 0, 0);
}

bool level::is_selected(uint32_t uid) const {
	return selection.find(uid) != selection.end();
}

level::level()
	: camera_control(false) {}

level_impl::level_impl() {}
