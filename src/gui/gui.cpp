/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "gui.h"

#include <chrono>

static std::chrono::steady_clock::time_point last_frame_time;
static f32 delta_time;

GLFWwindow* gui::startup(const char* window_title, s32 width, s32 height, bool maximized) {
	verify(glfwInit(), "Failed to load GLFW.");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	if(maximized) {
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	}

	GLFWwindow* window = glfwCreateWindow(width, height, window_title, NULL, NULL);
	verify(window, "Failed to create GLFW window.");

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // vsync

	verify(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress), "Cannot load GLAD.");
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigDockingWithShift = true;
	io.IniFilename = nullptr; // Disable loading/saving ImGui layout.
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 120");
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5, 0.5);
	
	last_frame_time = std::chrono::steady_clock::now();
	
	g_guiwad.open("data/gui.wad");
	
	return window;
}

void gui::run_frame(GLFWwindow* window, void (*update_func)(f32)) {
	glfwPollEvents();
	
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	update_func(delta_time);
	
	ImGui::Render();
	glfwMakeContextCurrent(window);
	
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	
	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
	glfwMakeContextCurrent(window);
	glfwSwapBuffers(window);
	
	auto frame_time = std::chrono::steady_clock::now();
	delta_time = std::chrono::duration_cast<std::chrono::microseconds>
		(frame_time - last_frame_time).count() / 1.e6;
	last_frame_time = frame_time;
}

void gui::shutdown(GLFWwindow* window) {
	glfwDestroyWindow(window);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
}

void GlTexture::upload(const u8* data, s32 width, s32 height) {
	if(id != 0) {
		glDeleteTextures(1, &id);
	}
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

FileInputStream g_guiwad;
