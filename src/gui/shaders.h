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

class Shader
{
public:
	using ShaderCallback = std::function<void(GLuint id)>;

	Shader(const GLchar* vertex_src, const GLchar* fragment_src, ShaderCallback before, ShaderCallback after);
	~Shader();

	void init();
	
	GLuint id() const;

private:
	GLuint link(GLuint vertex, GLuint fragment);
	GLuint compile(const GLchar* src, GLuint type);

	GLuint m_id;
	const GLchar* m_vertex_src;
	const GLchar* m_fragment_src;
	ShaderCallback m_before;
	ShaderCallback m_after;
};

struct Shaders
{
	Shaders();

	void init();
	
	Shader textured;
	GLint textured_view_matrix;
	GLint textured_projection_matrix;
	GLint textured_colour;
	GLint textured_sampler;
	
	Shader selection;
	GLint selection_view_matrix;
	GLint selection_projection_matrix;
	Shader icons;
	GLint icons_view_matrix;
	GLint icons_projection_matrix;
	GLint icons_sampler;
	Shader pickframe;
	GLint pickframe_view_matrix;
	GLint pickframe_projection_matrix;
	Shader pickframe_icons;
	GLint pickframe_icons_view_matrix;
	GLint pickframe_icons_projection_matrix;
};

#endif
