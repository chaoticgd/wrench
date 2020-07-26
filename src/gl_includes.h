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

#ifdef WRENCH_EDITOR

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glad/include/glad/glad.h>

struct gl_buffer {
	GLuint id = 0;
	gl_buffer() {}
	gl_buffer(const gl_buffer&) = delete;
	gl_buffer(gl_buffer&& rhs) : id(rhs.id) { rhs.id = 0; }
	~gl_buffer() { if(id != 0) glDeleteBuffers(1, &id); }
	GLuint& operator()() { return id; }
	const GLuint& operator()() const { return id; }
};

struct gl_texture {
	GLuint id = 0;
	gl_texture() {}
	gl_texture(const gl_texture&) = delete;
	gl_texture(gl_texture&& rhs) : id(rhs.id) { rhs.id = 0; }
	~gl_texture() { if(id != 0) glDeleteTextures(1, &id); }
	GLuint& operator()() { return id; }
	const GLuint& operator()() const { return id; }
};

#endif
#endif
