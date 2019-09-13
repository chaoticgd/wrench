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

#include "model.h"
#include "command.h"
#include "texture.h"
#include "imgui_includes.h"
#include "reflection/refolder.h"

# /*
#	Virtual base classes for levels and game objects (shrubs, mobies, ...).
#	
#	Mobies represent moving objects in the game world.
#	Shrubs are decorative objects e.g. plants or small rocks.
#	
# */

class game_object;
class point_object;
class tie;
class shrub;
class spline;
class moby;

#ifndef INSPECTABLE_DEF
#define INSPECTABLE_DEF
struct inspectable { virtual ~inspectable() = default; };
#endif

class level {
public:
	level();
	virtual ~level() = default;

	// Game data
	virtual texture_provider* get_texture_provider() = 0;
	const texture_provider* get_texture_provider() const;

	std::vector<game_object*> game_objects();
	std::vector<const game_object*> game_objects() const;
	std::vector<point_object*> point_objects();
	std::vector<const point_object*> point_objects() const;

	virtual std::vector<tie*> ties() = 0;
	std::vector<const tie*> ties() const;

	virtual std::vector<shrub*> shrubs() = 0;
	std::vector<const shrub*> shrubs() const;

	virtual std::vector<spline*> splines() = 0;
	std::vector<const spline*> splines() const;

	virtual std::map<int32_t, moby*> mobies() = 0;
	std::map<int32_t, const moby*> mobies() const;

	virtual std::map<std::string, std::map<uint32_t, std::string>> game_strings() = 0;

	// Selection
	std::vector<std::size_t> selection;
	bool is_selected(const game_object* obj) const;

	// Undo/redo
	template <typename T, typename... T_constructor_args>
	void emplace_command(T_constructor_args... args);
	void undo();
	void redo();

	// Inspector
	template <typename... T>
	void reflect(T... callbacks);

private:
	std::size_t _history_index;
	std::vector<std::unique_ptr<command>> _history_stack;
};

class game_object : public inspectable {
public:
	virtual std::size_t base() const = 0;

	template <typename... T>
	void reflect(T... callbacks);
};

class point_object : public game_object {
public:
	virtual glm::vec3 position() const = 0;
	virtual void set_position(glm::vec3 rotation_) = 0;

	virtual glm::vec3 rotation() const = 0;
	virtual void set_rotation(glm::vec3 rotation_) = 0;

	virtual std::string label() const = 0;

	template <typename... T>
	void reflect(T... callbacks);
};

class tie : public point_object {
public:
	template <typename... T>
	void reflect(T... callbacks);
	
	virtual const model& object_model() const = 0;
};

class shrub : public point_object {
public:
	template <typename... T>
	void reflect(T... callbacks);
};

class spline : public game_object {
public:
	template <typename... T>
	void reflect(T... callbacks);
	
	virtual std::vector<glm::vec3> points() const = 0;
};

class moby : public point_object {
public:
	virtual ~moby() = default;

	virtual int32_t uid() const = 0;
	virtual void set_uid(int32_t uid_) = 0;

	virtual uint16_t class_num() const = 0;
	virtual void set_class_num(uint16_t class_num_) = 0;

	virtual std::string class_name() const = 0;

	virtual const model& object_model() const = 0;

	template <typename... T>
	void reflect(T... callbacks);
};

template <typename T, typename... T_constructor_args>
void level::emplace_command(T_constructor_args... args) {
	_history_stack.resize(_history_index++);
	_history_stack.emplace_back(std::make_unique<T>(args...));
	auto& cmd = _history_stack[_history_index - 1];
	cmd->inject_level_pointer(this);
	cmd->apply();
}

template <typename... T>
void level::reflect(T... callbacks) {
	if(selection.size() == 1) {
		for(game_object* object : game_objects()) {
			if(object->base() == selection[0]) {
				object->reflect(callbacks...);
				break;
			}
		}
	}
}

template <typename... T>
void game_object::reflect(T... callbacks) {
	if(auto obj = dynamic_cast<point_object*>(this)) {
		obj->reflect(callbacks...);
	}
}

template <typename... T>
void point_object::reflect(T... callbacks) {
	ImGui::Columns(1);
	ImGui::Text("Point Object");
	ImGui::Columns(2);

	rf::reflector r(this, callbacks...);
	r.visit_m("Position", &moby::position,  &moby::set_position);
	r.visit_m("Rotation", &moby::rotation,  &moby::set_rotation);

	if(auto obj = dynamic_cast<tie*>(this)) {
		obj->reflect(callbacks...);
	}
	
	if(auto obj = dynamic_cast<shrub*>(this)) {
		obj->reflect(callbacks...);
	}
	
	if(auto obj = dynamic_cast<spline*>(this)) {
		obj->reflect(callbacks...);
	}

	if(auto obj = dynamic_cast<moby*>(this)) {
		obj->reflect(callbacks...);
	}
}

template <typename... T>
void tie::reflect(T... callbacks) {
	ImGui::Columns(1);
	ImGui::Text("Tie");
	ImGui::Columns(2);
}

template <typename... T>
void shrub::reflect(T... callbacks) {
	ImGui::Columns(1);
	ImGui::Text("Shrub");
	ImGui::Columns(2);
}

template <typename... T>
void spline::reflect(T... callbacks) {
	ImGui::Columns(1);
	ImGui::Text("Spline");
	ImGui::Columns(2);
}

template <typename... T>
void moby::reflect(T... callbacks) {
	ImGui::Columns(1);
	ImGui::Text("Moby");
	ImGui::Columns(2);

	rf::reflector r(this, callbacks...);
	r.visit_m("UID",      &moby::uid,       &moby::set_uid);
	r.visit_m("Class",    &moby::class_num, &moby::set_class_num);
}

#endif
