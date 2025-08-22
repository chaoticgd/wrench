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

#ifndef WRENCHGUI_GUI_H
#define WRENCHGUI_GUI_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/include/glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <nfd.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <core/util.h>
#include <core/stream.h>

namespace gui {

struct GlfwCallbacks
{
	void (*key_callback)(GLFWwindow* window, int key, int scancode, int action, int mods);
};

GLFWwindow* startup(
	const char* window_title,
	s32 width,
	s32 height,
	bool maximized = false,
	GlfwCallbacks* callbacks = nullptr);
void run_frame(GLFWwindow* window, void (*update_func)(f32));
void shutdown(GLFWwindow* window);
ImFont* load_font(SectorRange range, f32 size, f32 multiply = 1.f);
bool input_folder_path(std::string* output_path, const char* id, const nfdchar_t* default_path);

}

struct GlBuffer
{
	GLuint id = 0;
	GlBuffer() {}
	GlBuffer(const GlBuffer&) = delete;
	GlBuffer(GlBuffer&& rhs) : id(rhs.id) { rhs.id = 0; }
	~GlBuffer() { destroy(); }
	GlBuffer& operator=(const GlBuffer& rhs) = delete;
	GlBuffer& operator=(GlBuffer&& rhs) { destroy(); id = rhs.id; rhs.id = 0; return *this; }
	void destroy() { if(id != 0) { glDeleteBuffers(1, &id); id = 0; } }
};

struct GlTexture
{
	GLuint id = 0;
	GlTexture() {}
	GlTexture(const GlTexture&) = delete;
	GlTexture(GlTexture&& rhs) : id(rhs.id) { rhs.id = 0; }
	~GlTexture() { destroy(); }
	GlTexture& operator=(const GlTexture& rhs) = delete;
	GlTexture& operator=(GlTexture&& rhs) { destroy(); id = rhs.id; rhs.id = 0; return *this; }
	void upload(const u8* data, s32 width, s32 height);
	void destroy() { if(id != 0) { glDeleteTextures(1, &id); id = 0; } }
};

extern FileInputStream g_guiwad;

#endif
