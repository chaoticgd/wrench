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

#ifndef MOBY_H
#define MOBY_H

#include <map>
#include <glm/glm.hpp>

#include "reflection/refolder.h"

class moby {
public:
	moby(uint32_t uid);

	glm::vec3 position() const;
	void set_position(glm::vec3 position);

	glm::vec3 rotation() const;
	void set_rotation(glm::vec3 rotation);

	std::string name;
	bool selected;

	std::string get_class_name();
	uint16_t class_num;

	static const std::map<uint16_t, const char*> class_names;

	// Stores the last position where the moby was drawn in camera space.
	// Used for drawing text over the top of mobies in the 3D view.
	glm::vec3 last_drawn_pos;

private:
	uint32_t _uid;

	glm::vec3 _position;
	glm::vec3 _rotation;

public:
	template <typename... T>
	void reflect(T... callbacks) {
		rf::reflector r(this, callbacks...);
		r.visit_f("UID",      [=]() { return _uid; }, [](uint32_t) {});
		r.visit_r("Class",    class_num);
		r.visit_r("Name",     name);
		r.visit_m("Position", &moby::position, &moby::set_position);
		r.visit_m("Rotation", &moby::rotation, &moby::set_rotation);
	}
};

#endif
