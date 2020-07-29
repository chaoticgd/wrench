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
#include <time.h>
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

#include "stream.h"
#include "game_db.h"
#include "project.h"
#include "renderer.h"
#include "worker_logger.h"
#include "formats/texture.h"
#include "formats/level_impl.h"

# /*
#	Represents the current state of the program including the currently open
#	project, configuration and more.
# */

struct GLFWwindow;

class window;
class view_3d;

enum class tool_type {
	picker, selection, translate
};

struct tool {
	tool_type type;
	gl_texture icon;
};

class app {
public:
	app();

	std::vector<std::unique_ptr<window>> windows;

	template <typename T, typename... T_constructor_args>
	T* emplace_window(T_constructor_args... args);
	
	std::vector<tool> tools;
	std::size_t active_tool_index = 0;
	tool& active_tool() { return tools.at(active_tool_index); }
	
	glm::vec2 mouse_last;

	GLFWwindow* glfw_window;
	int window_width, window_height;
	
	gl_renderer renderer;
	
	int64_t delta_time;
	
	glm::vec3 translate_tool_displacement;

	void new_project(game_iso game);
	void open_project(std::string path);

	std::string project_path();

	wrench_project* get_project();
	const wrench_project* get_project() const;
	
	level* get_level();
	const level* get_level() const;

	bool has_camera_control();

	void init_gui_scale();
	void update_gui_scale();

	const std::vector<gamedb_game> game_db;

private:
	std::atomic_bool _lock_project; // Prevent race conditions while creating/loading a project.
	std::unique_ptr<wrench_project> _project;
	
	std::vector<float> _gui_scale_parameters;
};

struct config {
	std::string emulator_path;
	std::vector<game_iso> game_isos;
	float gui_scale;
	bool vsync;
	struct {
		bool stream_tracing;
	} debug;
	
	static config& get();
	void read(app& a);
	void write();
};

gl_texture load_icon(std::string path);

template <typename T, typename... T_constructor_args>
T* app::emplace_window(T_constructor_args... args) {
	auto window = std::make_unique<T>(args...);
	T* result = window.get();
	windows.emplace_back(std::move(window));
	return result;
}

#endif
