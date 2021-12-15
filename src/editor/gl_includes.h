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

#ifndef GL_INCLUDES_H
#define GL_INCLUDES_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/include/glad/glad.h>

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
};

#define M_PI 3.14159265358979323846

#endif
