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

// Represents a level currently loaded into wrench. A mostly opaque type,
// allowing only for undo stack manipulation among some other odds and ends.
// Only command objects should access the (non const) level_impl type directly
// if we are to have any hope of implementing a working undo/redo system!
class level {
public:
	void apply_command(std::unique_ptr<command> action);

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
};

class level_impl : public level {
public:
	level_impl();

	void add_moby(uint32_t uid, std::unique_ptr<moby> m) { _mobies[uid].swap(m); }
	const std::map<uint32_t, std::unique_ptr<moby>>& mobies() const { return _mobies; }

private:
	std::map<uint32_t, std::unique_ptr<moby>> _mobies;
};

#endif
