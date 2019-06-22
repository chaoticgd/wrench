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

std::string moby::get_class_name() {
	if(class_names.find(class_num) != class_names.end()) {
		return class_names.at(class_num);
	}
	return std::to_string(class_num);
}

const std::map<uint16_t, const char*> moby::class_names {
	{ 0x1f4, "crate" },
	{ 0x2f6, "swingshot_grapple" },
	{ 0x323, "swingshot_swinging" }
};
