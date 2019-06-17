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

#include "moby.h"

moby::moby(uint32_t uid)
	: selected(false),
	  _uid(uid) {}

glm::vec3 moby::position() const {
	return _position;
}

void moby::set_position(glm::vec3 position) {
	_position = position;
}

glm::vec3 moby::rotation() const {
	return _rotation;
}

void moby::set_rotation(glm::vec3 rotation) {
	_rotation = rotation;
}
