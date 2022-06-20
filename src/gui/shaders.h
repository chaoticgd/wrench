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

#ifndef GUI_SHADERS_H
#define GUI_SHADERS_H

#include <stdexcept>
#include <functional>

#include <gui/gui.h>

class Shader {
public:
	using ShaderCallback = std::function<void(GLuint id)>;

	Shader(const GLchar* vertex_src, const GLchar* fragment_src, ShaderCallback before, ShaderCallback after);
	~Shader();

	void init();
	
	GLuint id() const;

private:
	GLuint link(GLuint vertex, GLuint fragment);
	GLuint compile(const GLchar* src, GLuint type);

	GLuint _id;
	const GLchar* _vertex_src;
	const GLchar* _fragment_src;
	ShaderCallback _before;
	ShaderCallback _after;
};

struct Shaders {
	Shaders();

	void init();
	
	Shader textured;
	GLint textured_colour;
	GLint textured_sampler;
	
	Shader selection;
	
	Shader pickframe;
};

#endif
