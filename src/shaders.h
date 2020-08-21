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

#ifndef SHADERS_H
#define SHADERS_H

#include <stdexcept>
#include <functional>

#include "gl_includes.h"

# /*
#	A class that can compile OpenGL shaders along with a number of shaders used
#	by the 3D view.
# */

class shader_program {
public:
	using shader_callback = std::function<void(GLuint id)>;

	shader_program(const GLchar* vertex_src, const GLchar* fragment_src, shader_callback after);
	~shader_program();

	void init(); // Called after a valid OpenGL context is setup.

	GLuint id() const;

private:
	static GLuint link(GLuint vertex, GLuint fragment);
	static GLuint compile(const GLchar* src, GLuint type);

	GLuint _id;
	const GLchar* _vertex_src;
	const GLchar* _fragment_src;
	shader_callback _after;
};

struct shader_programs {
	shader_programs();

	void init(); // Called after a valid OpenGL context is setup.

	shader_program solid_colour;
	GLint solid_colour_transform;
	GLint solid_colour_rgb;
	
	shader_program solid_colour_batch;
	GLint solid_colour_batch_rgb;
	
	shader_program textured;
	GLint textured_sampler;
};

#endif
