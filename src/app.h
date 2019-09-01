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
#include <atomic>
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

#include "level.h"
#include "stream.h"
#include "project.h"
#include "worker_logger.h"
#include "formats/level_impl.h"
#include "formats/texture_impl.h"

# /*
#	Represents the current state of the program including the currently open
#	project, configuration and more.
# */

class window;
class view_3d;

class inspector_reflector;

struct settings_data {
	std::string emulator_path;
	std::map<std::string, std::string> game_paths;
	float gui_scale;
};

class app {
public:
	app();

	std::vector<std::unique_ptr<window>> windows;
	settings_data settings;

	template <typename T, typename... T_constructor_args>
	T* emplace_window(T_constructor_args... args);

	glm::vec2 mouse_last;
	glm::vec2 mouse_diff;
	std::set<int> keys_down;

	int window_width, window_height;

	void new_project(std::string game_id);
	void open_project(std::string path);
	void save_project(bool save_as);

	std::string project_path();

	wrench_project* get_project();
	const wrench_project* get_project() const;
	
	level* get_level();
	const level* get_level() const;

	bool has_camera_control();
	view_3d* get_3d_view();

	std::any this_any;

	void read_settings();
	void save_settings();

	void run_emulator();

	void init_gui_scale();
	void update_gui_scale();

	template <typename... T>
	void reflect(T... callbacks);

private:
	std::atomic_bool _lock_project; // Prevent race conditions while creating/loading a project.
	std::unique_ptr<wrench_project> _project;
	std::vector<float> _gui_scale_parameters;
};

template <typename T, typename... T_constructor_args>
T* app::emplace_window(T_constructor_args... args) {
	auto window = std::make_unique<T>(args...);
	T* result = window.get();
	windows.emplace_back(std::move(window));
	return result;
}

template <typename... T>
void app::reflect(T... callbacks) {
	if(auto lvl = get_level()) {
		lvl->reflect(callbacks...);
	}
}

#endif
