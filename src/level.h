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

#ifndef LEVEL_H
#define LEVEL_H

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "reflection/refolder.h"
#include "texture.h"

class game_object;
class point_object;
class moby;

class level {
public:
	virtual ~level() = default;

	virtual texture_provider* get_texture_provider() = 0;
	const texture_provider* get_texture_provider() const;

	std::vector<const point_object*> point_objects() const;

	virtual std::map<uint32_t, moby*> mobies() = 0;
	std::map<uint32_t, const moby*> mobies() const;

	virtual std::map<std::string, std::map<uint32_t, std::string>> game_strings() = 0;

	std::vector<game_object*> selection;

	bool is_selected(const game_object* obj) const;

	template <typename... T>
	void reflect(T... callbacks);
};

class game_object {
public:
	virtual std::string label() const = 0;

	template <typename... T>
	void reflect(T... callbacks);
};

class point_object : public game_object {
public:
	virtual glm::vec3 position() const = 0;
};

class moby : public point_object {
public:
	virtual ~moby() = default;

	virtual uint32_t uid() const = 0;
	virtual void set_uid(uint32_t uid_) = 0;

	virtual uint16_t class_num() const = 0;
	virtual void set_class_num(uint16_t class_num_) = 0;

	virtual glm::vec3 position() const = 0;
	virtual void set_position(glm::vec3 rotation_) = 0;

	virtual glm::vec3 rotation() const = 0;
	virtual void set_rotation(glm::vec3 rotation_) = 0;

	virtual std::string class_name() const = 0;

	template <typename... T>
	void reflect(T... callbacks);
};

template <typename... T>
void level::reflect(T... callbacks) {
	if(selection.size() == 1) {
		selection[0]->reflect(callbacks...);
	}
}

template <typename... T>
void game_object::reflect(T... callbacks) {
	if(auto obj = dynamic_cast<moby*>(this)) {
		obj->reflect(callbacks...);
	}
}

template <typename... T>
void moby::reflect(T... callbacks) {
	rf::reflector r(this, callbacks...);
	r.visit_m("UID",      &moby::uid,       &moby::set_uid);
	r.visit_m("Class",    &moby::class_num, &moby::set_class_num);
	r.visit_m("Position", &moby::position,  &moby::set_position);
	r.visit_m("Rotation", &moby::rotation,  &moby::set_rotation);
}

#endif
