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
#include <glm/glm.hpp>

#include "formats/iso.h"

class level;
class window;
class three_d_view;

class inspector_reflector;

class app {
public:
	app();

	std::vector<std::unique_ptr<window>> windows;

	glm::vec2 mouse_last;
	glm::vec2 mouse_diff;
	std::set<int> keys_down;

	int window_width, window_height;

	bool has_iso() const;
	// This ensures that the ISO object is only accessed if it exists.
	void bind_iso(std::function<void(stream&)> callback);
	void open_iso(std::string path);

	bool has_level() const;
	void bind_level(std::function<void(level&)> callback);
	void bind_level(std::function<void(const level&)> callback) const;

	bool has_camera_control();
	std::optional<three_d_view*> get_3d_view();

	stream* selection;
	std::unique_ptr<inspector_reflector> reflector;

private:
	std::unique_ptr<iso_stream> _iso;
};

#endif
