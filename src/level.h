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

#include "command.h"
#include "moby.h"

class level_impl;
class selection_adapter;

using string_table = std::vector<std::pair<uint32_t, std::string>>;

// Represents a level currently loaded into wrench. A mostly opaque type,
// allowing only for undo stack manipulation among some other odds and ends.
// Only command objects should access the (non const) level_impl type directly
// if we are to have any hope of implementing a working undo/redo system!
class level {
public:
	template <typename T_sub_type, typename... T_constructor_args>
	void emplace_command(T_constructor_args... args);

	bool undo();
	bool redo();

	void reset_camera();
	bool camera_control;
	glm::vec3 camera_position;
	glm::vec2 camera_rotation;

	bool is_selected(uint32_t uid) const;
	std::set<uint32_t> selection;

protected:
	level();

	std::vector<std::unique_ptr<command>> _history;
	std::size_t _history_index;

public:

	// Used by the inspector.
	template <typename... T>
	void reflect(T... callbacks);
};

class level_impl : public level {
public:
	level_impl();

	void add_moby(uint32_t uid, std::unique_ptr<moby> m) { _mobies[uid].swap(m); }
	const std::map<uint32_t, std::unique_ptr<moby>>& mobies() const { return _mobies; }

	glm::vec3 ship_position;
	std::vector<std::pair<std::string, string_table>> strings;

private:
	std::map<uint32_t, std::unique_ptr<moby>> _mobies;
};

template <typename T_sub_type, typename... T_constructor_args>
void level::emplace_command(T_constructor_args... args) {
	auto cmd = std::unique_ptr<command>(new T_sub_type(args...));
	cmd->inject_level_pointer(static_cast<level_impl*>(this));
	cmd->apply();
	_history.resize(_history_index + 1);
	_history[_history_index++].swap(cmd);
}

template <typename... T>
void level::reflect(T... callbacks) {
	if(selection.size() > 0) {
		auto& mobies = static_cast<level_impl*>(this)->mobies();
		mobies.at(*selection.begin())->reflect(callbacks...);
	}
}

#endif
