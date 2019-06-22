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

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "command_line.h"
#include "gui.h"
#include "app.h"
#include "renderer.h"
#include "formats/level_data.h"

void update_camera_movement(app* a);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

int main(int argc, char** argv) {

	std::string level_path;

	po::options_description desc("A level editor for the Ratchet & Clank games");
	desc.add_options()
		("import,i", po::value<std::string>(&level_path),
			"Import the specified WAD level file.");

	if(!parse_command_line_args(argc, argv, desc)) {
		return 0;
	}

	app a;
	a.tools.emplace_back(std::make_unique<gui::moby_list>());
	a.tools.emplace_back(std::make_unique<gui::inspector>());
	a.tools.emplace_back(std::make_unique<gui::viewport_information>());

	if(!glfwInit()) {
		throw std::runtime_error("Cannot load GLFW.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Wrench", NULL, NULL);
	if(window == nullptr) {
		throw std::runtime_error("Cannot create GLFW window.");
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if(glewInit() != GLEW_OK) {
		throw std::runtime_error("Cannot load GLEW.");
	}

	glfwSetWindowUserPointer(window, &a);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");

	shader_programs shaders;

	if(level_path != "") {
		a.import_level(level_path);
	}

	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		update_camera_movement(&a);

		gui::render(a);

		ImGui::Render();
		glfwMakeContextCurrent(window);
		glfwGetFramebufferSize(window, &a.window_width, &a.window_height);

		glViewport(0, 0, a.window_width, a.window_height);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_current_level(a, shaders);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(window);
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void update_camera_movement(app* a) {
	if(!a->has_level() || !a->get_level().camera_control) {
		return;
	}

	float dist = glm::distance(glm::vec2(0, 0), a->mouse_diff) * 2;
	float dx = std::sin(a->get_level().camera_rotation.y) * dist;
	float dz = std::cos(a->get_level().camera_rotation.y) * dist;

	auto is_down = [=](int key) {
		return a->keys_down.find(key) != a->keys_down.end();
	};

	glm::vec3 movement;
	if(is_down(GLFW_KEY_W)) {
		movement.x += dx;
		movement.y -= dz;
	}
	if(is_down(GLFW_KEY_S)) {
		movement.x -= dx;
		movement.y += dz;
	}
	if(is_down(GLFW_KEY_A)) {
		movement.x -= dz;
		movement.y -= dx;
	}
	if(is_down(GLFW_KEY_D)) {
		movement.x += dz;
		movement.y += dx;
	}
	if(is_down(GLFW_KEY_SPACE)) {
		movement.z += dist;
	}
	if(is_down(GLFW_KEY_LEFT_SHIFT)) {
		movement.z -= dist;
	}
	a->get_level().camera_position += movement;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

	app& a = *static_cast<app*>(glfwGetWindowUserPointer(window));
	if(!a.has_level()) {
		return;
	}
	level& lvl = a.get_level();

	if(action == GLFW_PRESS) {
		a.keys_down.insert(key);
	} else if(action == GLFW_RELEASE) {
		a.keys_down.erase(key);
	}

	if(action == GLFW_PRESS && key == GLFW_KEY_Z) {
		lvl.camera_control = !lvl.camera_control;
		glfwSetInputMode(window, GLFW_CURSOR,
			lvl.camera_control ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	app& a = *static_cast<app*>(glfwGetWindowUserPointer(window));
	if(!a.has_level()) {
		return;
	}
	level& lvl = a.get_level();

	a.mouse_diff = glm::vec2(xpos, ypos) - a.mouse_last;
	a.mouse_last = glm::vec2(xpos, ypos);

	if(lvl.camera_control) {
		lvl.camera_rotation.y += a.mouse_diff.x * 0.0005;
		lvl.camera_rotation.x += a.mouse_diff.y * 0.0005;
	}
}
