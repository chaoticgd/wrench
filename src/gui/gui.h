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
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_glfw.h>

#include <core/util.h>
#include <core/stream.h>

namespace gui {

GLFWwindow* startup(const char* window_title, s32 width, s32 height, bool maximized = false);
void run_frame(GLFWwindow* window, void (*update_func)(f32));
void shutdown(GLFWwindow* window);

}

struct GlBuffer {
	GLuint id = 0;
	GlBuffer() {}
	GlBuffer(const GlBuffer&) = delete;
	GlBuffer(GlBuffer&& rhs) : id(rhs.id) { rhs.id = 0; }
	~GlBuffer() { if(id != 0) glDeleteBuffers(1, &id); }
	GLuint& operator()() { return id; }
	const GLuint& operator()() const { return id; }
	GlBuffer& operator=(GlBuffer&& rhs) { id = rhs.id; rhs.id = 0; return *this; }
};

struct GlTexture {
	GLuint id = 0;
	GlTexture() {}
	GlTexture(const GlTexture&) = delete;
	GlTexture(GlTexture&& rhs) : id(rhs.id) { rhs.id = 0; }
	~GlTexture() { if(id != 0) glDeleteTextures(1, &id); }
	GLuint& operator()() { return id; }
	const GLuint& operator()() const { return id; }
	GlTexture& operator=(GlTexture&& rhs) { id = rhs.id; rhs.id = 0; return *this; }
	void upload(const u8* data, s32 width, s32 height);
	void destroy() { if(id != 0) { glDeleteTextures(1, &id); id = 0; } }
};

extern FileInputStream g_guiwad;

#endif
