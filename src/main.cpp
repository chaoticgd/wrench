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

#include <chrono>

#include "gl_includes.h"
#include "command_line.h"
#include "app.h"
#include "gui.h"
#include "renderer.h"

# /*
#	Setup code, the main loop, and GLFW stuff.
# */

void update_camera(app* a);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(int argc, char** argv) {

	std::string project_path;

	po::options_description desc("A level editor for the Ratchet & Clank games");
	desc.add_options()
		("project,p", po::value<std::string>(&project_path),
			"Open the specified project (.wrench) file.");

	if(!parse_command_line_args(argc, argv, desc)) {
		return 0;
	}

	app a;

	if(!glfwInit()) {
		throw std::runtime_error("Cannot load GLFW.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	a.glfw_window = glfwCreateWindow(1280, 720, "Wrench Editor", NULL, NULL);
	if(a.glfw_window == nullptr) {
		throw std::runtime_error("Cannot create GLFW window.");
	}

	glfwMakeContextCurrent(a.glfw_window);
	glfwSwapInterval(1);

	if(glewInit() != GLEW_OK) {
		throw std::runtime_error("Cannot load GLEW.");
	}

	glfwSetWindowUserPointer(a.glfw_window, &a);
	glfwSetKeyCallback(a.glfw_window, key_callback);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = true;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(a.glfw_window, true);
	ImGui_ImplOpenGL3_Init("#version 120");

	a.init_gui_scale();
	a.update_gui_scale();
	a.renderer.shaders.init();

	if(project_path != "") {
		a.open_project(project_path);
	}
	
	a.windows.emplace_back(std::make_unique<view_3d>(&a));
	a.windows.emplace_back(std::make_unique<gui::texture_browser>());
	a.windows.emplace_back(std::make_unique<gui::model_browser>());
	a.windows.emplace_back(std::make_unique<gui::project_tree>());
	a.windows.emplace_back(std::make_unique<gui::moby_list>());
	a.windows.emplace_back(std::make_unique<gui::inspector>());
	a.windows.emplace_back(std::make_unique<gui::viewport_information>());
	a.windows.emplace_back(std::make_unique<gui::tools>());

	auto last_frame_time = std::chrono::steady_clock::now();

	while(!glfwWindowShouldClose(a.glfw_window)) {
		glfwPollEvents();
		update_camera(&a);

		gui::render(a);

		ImGui::Render();
		glfwMakeContextCurrent(a.glfw_window);
		glfwGetFramebufferSize(a.glfw_window, &a.window_width, &a.window_height);

		glViewport(0, 0, a.window_width, a.window_height);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(a.glfw_window);
		glfwSwapBuffers(a.glfw_window);
		
		auto frame_time = std::chrono::steady_clock::now();
		a.delta_time = std::chrono::duration_cast<std::chrono::microseconds>
			(frame_time - last_frame_time).count();
		last_frame_time = frame_time;
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(a.glfw_window);
	glfwTerminate();
}

void update_camera(app* a) {
	// Rotation
	double xpos, ypos;
	glfwGetCursorPos(a->glfw_window, &xpos, &ypos);
	
	glm::vec2 mouse_cur = glm::vec2(xpos, ypos);
	glm::vec2 mouse_diff = mouse_cur - a->mouse_last;
	a->mouse_last = mouse_cur;

	if(!a->renderer.camera_control) {
		return;
	}

	static const auto constrain = [](float* ptr, float min, float max, bool shouldFlip) {
		if (*ptr < min)
			*ptr = (shouldFlip) ? max : min;
		if (*ptr > max)
			*ptr = (shouldFlip) ? min : max;
	};

	static const float minPitch = glm::radians(-89.f), maxPitch = glm::radians(89.f);
	static const float minYaw = glm::radians(-180.f), maxYaw = glm::radians(180.f);
	
	a->renderer.camera_rotation.y += mouse_diff.x * 0.0005;
	a->renderer.camera_rotation.x -= mouse_diff.y * 0.0005;

	constrain(&a->renderer.camera_rotation.y, minYaw, maxYaw, true);
	constrain(&a->renderer.camera_rotation.x, minPitch, maxPitch, false);	// Position
	
	float dist = 2;
	float dx = std::sin(a->renderer.camera_rotation.y) * dist;
	float dz = std::cos(a->renderer.camera_rotation.y) * dist;

	auto is_down = [=](int key) {
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
	a->renderer.camera_position += movement;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	app* a = static_cast<app*>(glfwGetWindowUserPointer(window));

	if(action == GLFW_PRESS && key == GLFW_KEY_Z) {
		a->renderer.camera_control = !a->renderer.camera_control;
		glfwSetInputMode(window, GLFW_CURSOR,
			a->renderer.camera_control ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
}
