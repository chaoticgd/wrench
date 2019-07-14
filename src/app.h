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

#include <any>
#include <set>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <glm/glm.hpp>

#include "level.h"
#include "stream.h"
#include "worker_logger.h"
#include "formats/level_impl.h"
#include "formats/texture_impl.h"

class window;
class three_d_view;

class inspector_reflector;

class iso_adapters {
public:
	iso_adapters(stream* iso_file, worker_logger& log);

	level_impl  level;
	fip_scanner space_wad;
	fip_scanner armor_wad;
};

class app {
public:
	app();

	std::vector<std::unique_ptr<window>> windows;

	template <typename T, typename... T_constructor_args>
	void emplace_window(T_constructor_args... args);

	glm::vec2 mouse_last;
	glm::vec2 mouse_diff;
	std::set<int> keys_down;

	int window_width, window_height;

	void open_iso(std::string path);

	level* get_level();
	const level* get_level() const;

	bool has_camera_control();
	std::optional<three_d_view*> get_3d_view();

	std::vector<texture_provider*> texture_providers();

	std::any selection;

private:
	std::optional<file_stream> _iso;
	std::unique_ptr<iso_adapters> _iso_adapters;
};

template <typename T, typename... T_constructor_args>
void app::emplace_window(T_constructor_args... args) {
	windows.emplace_back(std::make_unique<T>(args...));
}

#endif
