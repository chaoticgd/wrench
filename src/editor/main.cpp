/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include <chrono>

#include <toolwads/wads.h>
#include <gui/commands.h>
#include "fs_includes.h"
#include "gl_includes.h"
#include "command_line.h"
#include "app.h"
#include "gui/gui.h"
#include "renderer.h"

static void run_wrench(GLFWwindow* window, const std::string& game_path, const std::string& mod_path);
static void update(f32 delta_time);
static void init_gui(app& a, GLFWwindow** window);
static void update_camera(app* a);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

static app* app_ptr;

int main(int argc, char** argv) {
	if(argc != 3) {
		fprintf(stderr, "usage: %s <game path> <mod path>\n", argv[0]);
		return 1;
	}
	
	gui::setup_bin_paths(argv[0]);
	
	std::string game_path = argv[1];
	std::string mod_path = argv[2];
	
	
	GLFWwindow* window = gui::startup("Wrench Editor", 1280, 720, true);
	run_wrench(window, game_path, mod_path);
	gui::shutdown(window);
}


static void run_wrench(GLFWwindow* window, const std::string& game_path, const std::string& mod_path) {
	app a;
	app_ptr = &a;
	
	a.glfw_window = window;
	
	a.game_bank = &a.asset_forest.mount<LooseAssetBank>(game_path, false);
	a.mod_bank = &a.asset_forest.mount<LooseAssetBank>(mod_path, true);
	
	gui::load_font(wadinfo.gui.fonts[0], 18, 1.2f);
	
	init_renderer();
	
	a.tools = enumerate_tools();
	
	a.windows.emplace_back(std::make_unique<view_3d>());
	a.windows.emplace_back(std::make_unique<gui::moby_list>());
	a.windows.emplace_back(std::make_unique<gui::viewport_information>());
	a.windows.emplace_back(std::make_unique<Inspector>());
	
	while(!glfwWindowShouldClose(a.glfw_window)) {
		gui::run_frame(window, update);
	}
	
	shutdown_renderer();
}

static void update(f32 delta_time) {
	gui::render(*app_ptr);
}

static void update_camera(app* a) {
	// Rotation
	double xpos, ypos;
	glfwGetCursorPos(a->glfw_window, &xpos, &ypos);
	
	glm::vec2 mouse_cur = glm::vec2(xpos, ypos);
	glm::vec2 mouse_diff = mouse_cur - a->mouse_last;
	a->mouse_last = mouse_cur;
	
	if(!a->render_settings.camera_control) {
		return;
	}
	
	static const auto constrain = [](float* ptr, float min, float max, bool should_flip) {
		if (*ptr < min)
			*ptr = (should_flip) ? max : min;
		if (*ptr > max)
			*ptr = (should_flip) ? min : max;
	};
	
	static const float min_pitch = glm::radians(-89.f), max_pitch = glm::radians(89.f);
	static const float min_yaw = glm::radians(-180.f),  max_yaw = glm::radians(180.f);
	
	a->render_settings.camera_rotation.y += mouse_diff.x * 0.0005;
	a->render_settings.camera_rotation.x -= mouse_diff.y * 0.0005;
	
	constrain(&a->render_settings.camera_rotation.y, min_yaw, max_yaw, true);
	constrain(&a->render_settings.camera_rotation.x, min_pitch, max_pitch, false);
	
	// Position
	float dist = 2;
	float dx = std::sin(a->render_settings.camera_rotation.y) * dist;
	float dz = std::cos(a->render_settings.camera_rotation.y) * dist;
	
	auto is_down = [&](int key) {
		return glfwGetKey(a->glfw_window, key);
	};
	
	glm::vec3 movement(0, 0, 0);
	
	static constexpr float magic_movement = 0.0001f;
	
	if(is_down(GLFW_KEY_W)) {
		movement.x -= dz * a->delta_time * magic_movement;
		movement.y += dx * a->delta_time * magic_movement;
	}
	if(is_down(GLFW_KEY_S)) {
		movement.x += dz * a->delta_time * magic_movement;
		movement.y -= dx * a->delta_time * magic_movement;
	}
	if(is_down(GLFW_KEY_A)) {
		movement.x -= dx * a->delta_time * magic_movement;
		movement.y -= dz * a->delta_time * magic_movement;
	}
	if(is_down(GLFW_KEY_D)) {
		movement.x += dx * a->delta_time * magic_movement;
		movement.y += dz * a->delta_time * magic_movement;
	}
	if(is_down(GLFW_KEY_SPACE)) {
		movement.z += dist * a->delta_time * magic_movement;
	}
	if(is_down(GLFW_KEY_LEFT_SHIFT)) {
		movement.z -= dist * a->delta_time * magic_movement;
	}
	a->render_settings.camera_position += movement;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	app* a = static_cast<app*>(glfwGetWindowUserPointer(window));
	
	if(action == GLFW_PRESS && key == GLFW_KEY_Z) {
		a->render_settings.camera_control = !a->render_settings.camera_control;
		glfwSetInputMode(window, GLFW_CURSOR,
			a->render_settings.camera_control ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
}
