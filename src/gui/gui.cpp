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
#include <imgui/backends/imgui_impl_glfw.h>
#include <engine/compression.h>

static void imgui_glfw_new_frame();

static std::chrono::steady_clock::time_point last_frame_time;
static f32 delta_time;
static std::vector<std::vector<u8>> font_buffers;

GLFWwindow* gui::startup(const char* window_title, s32 width, s32 height, bool maximized, GlfwCallbacks* callbacks) {
	verify(glfwInit(), "Failed to load GLFW.");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
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
	ImGui_ImplGlfw_InitForOpenGL(window, callbacks == nullptr);
	if(callbacks != nullptr) {
		glfwSetWindowFocusCallback(window, ImGui_ImplGlfw_WindowFocusCallback);
		glfwSetCursorEnterCallback(window, ImGui_ImplGlfw_CursorEnterCallback);
		glfwSetCursorPosCallback(window, ImGui_ImplGlfw_CursorPosCallback);
		glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
		glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
		glfwSetKeyCallback(window, callbacks->key_callback);
		glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
		glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);
	}
	ImGui_ImplOpenGL3_Init("#version 120");
	
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5, 0.5);
	style.TabRounding = 2.f;
	style.ScrollbarRounding = 2.f;
	style.Colors[ImGuiCol_MenuBarBg] = style.Colors[ImGuiCol_WindowBg];
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.15f, 0.15f, 0.15f, 1.f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.f);
	
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

ImFont* gui::load_font(SectorRange range, f32 size, f32 multiply) {
	std::vector<u8> compressed_font = g_guiwad.read_multiple<u8>(range.offset.bytes(), range.size.bytes());
	std::vector<u8>& decompressed_font = font_buffers.emplace_back();
	decompress_wad(decompressed_font, compressed_font);
	
	ImFontConfig font_cfg;
	font_cfg.FontDataOwnedByAtlas = false;
	font_cfg.RasterizerMultiply = multiply;
	
	ImGuiIO& io = ImGui::GetIO();
	return io.Fonts->AddFontFromMemoryTTF(decompressed_font.data(), decompressed_font.size(), size, &font_cfg);
}


void gui::shutdown(GLFWwindow* window) {
	glfwDestroyWindow(window);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	
	font_buffers.clear();
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
