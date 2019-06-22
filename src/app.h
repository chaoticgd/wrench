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

#ifndef APP_H
#define APP_H

#include <set>
#include <vector>
#include <memory>
#include <functional>

#include "level.h"

class tool;

struct app {
	std::vector<std::unique_ptr<tool>> tools;

	glm::vec2 mouse_last;
	glm::vec2 mouse_diff;
	std::set<int> keys_down;

	int window_width, window_height;

	bool has_level() const;

	// This ensures that the level object is only accessed if it exists.
	void if_level(std::function<void(level&)> callback);
	void if_level(std::function<void(const level_impl&)> callback) const;
	void if_level(std::function<void(level&, const level_impl&)> callback) const;
	void import_level(std::string path);

private:
	std::unique_ptr<level_impl> _level;
};

#endif
