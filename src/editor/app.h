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

#include <editor/tools.h>
#include <editor/renderer.h>
#include <editor/fs_includes.h>
#include <editor/gui/window.h>
#include <editor/formats/level_impl.h>

struct GLFWwindow;

class app {
public:
	app() {}

	std::vector<std::unique_ptr<window>> windows;

	template <typename T, typename... T_constructor_args>
	T* emplace_window(T_constructor_args... args);
	
	std::vector<std::unique_ptr<tool>> tools;
	std::size_t active_tool_index = 0;
	tool& active_tool() { return *tools.at(active_tool_index).get(); }
	
	glm::vec2 mouse_last { 0, 0 };

	GLFWwindow* glfw_window;
	int window_width, window_height;
	
	RenderSettings render_settings;
	
	int64_t delta_time = 0;
	
	level* get_level();
	const level* get_level() const;
	
private:
	std::optional<level> _lvl;

public:

	bool has_camera_control();

	void init_gui_scale();
	void update_gui_scale();
};

GlTexture load_icon(std::string path);

template <typename T, typename... T_constructor_args>
T* app::emplace_window(T_constructor_args... args) {
	auto window = std::make_unique<T>(args...);
	T* result = window.get();
	windows.emplace_back(std::move(window));
	return result;
}

#endif
