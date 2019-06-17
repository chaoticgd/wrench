#include "command_line.h"
#include "gui.h"
#include "app.h"
#include "renderer.h"
#include "formats/level_data.h"

void update_camera_movement(app* a);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

int main(int argc, char** argv) {
	if(!parse_command_line_args(argc, argv)) {
		return 0;
	}

	app a;
	a.tools.emplace_back(std::make_unique<gui::moby_list>());
	a.tools.emplace_back(std::make_unique<gui::inspector>());
	a.tools.emplace_back(std::make_unique<gui::viewport_information>());
	file_stream lvl("lvl");
	a.set_level(import_level(lvl));

	shader_programs shaders;
	glfwSetWindowUserPointer(a.main_window.get(), &a);
	glfwSetKeyCallback(a.main_window.get(), key_callback);
	glfwSetCursorPosCallback(a.main_window.get(), cursor_position_callback);

	while(!glfwWindowShouldClose(a.main_window.get())) {
		glfwPollEvents();
		update_camera_movement(&a);

		gui::render(a);

		ImGui::Render();
		int display_w, display_h;
		glfwMakeContextCurrent(a.main_window.get());
		glfwGetFramebufferSize(a.main_window.get(), &display_w, &display_h);

		glViewport(0, 0, display_w, display_h);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_current_level(a, shaders);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(a.main_window.get());
		glfwSwapBuffers(a.main_window.get());
	}

}

void update_camera_movement(app* a) {
	if(!a->get_level().camera_control) {
		return;
	}

	float dist = glm::distance(glm::vec2(0, 0), a->mouse_diff);
	float dx = std::sin(a->get_level().camera_rotation.y) * dist;
	float dz = std::cos(a->get_level().camera_rotation.y) * dist;

	auto is_down = [=](int key) {
		return a->keys_down.find(key) != a->keys_down.end();
	};

	glm::vec3 movement;
	if(is_down(GLFW_KEY_W)) {
		movement.x -= dx;
		movement.z += dz;
	}
	if(is_down(GLFW_KEY_S)) {
		movement.x += dx;
		movement.z -= dz;
	}
	if(is_down(GLFW_KEY_A)) {
		movement.x += dz;
		movement.z += dx;
	}
	if(is_down(GLFW_KEY_D)) {
		movement.x -= dz;
		movement.z -= dx;
	}
	if(is_down(GLFW_KEY_SPACE)) {
		movement.y -= dist;
	}
	if(is_down(GLFW_KEY_LEFT_SHIFT)) {
		movement.y += dist;
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
