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

#include "shaders.h"

#include <vector>

Shader::Shader(
	const GLchar* vertex_src, const GLchar* fragment_src, ShaderCallback before, ShaderCallback after)
	: m_id(0)
	, m_vertex_src(vertex_src)
	, m_fragment_src(fragment_src)
	, m_before(before)
	, m_after(after) {}
	
Shader::~Shader()
{
	if (m_id) {
		glDeleteProgram(m_id);
	}
}

void Shader::init()
{
	m_id = link(
		compile(m_vertex_src,   GL_VERTEX_SHADER),
		compile(m_fragment_src, GL_FRAGMENT_SHADER));
}

GLuint Shader::id() const
{
	return m_id;
}

GLuint Shader::link(GLuint vertex, GLuint fragment)
{
	GLuint id = glCreateProgram();
	glAttachShader(id, vertex);
	glAttachShader(id, fragment);
	
	m_before(id);
	glLinkProgram(id);
	m_after(id);

	GLint result;
	int log_length;
	glGetProgramiv(id, GL_LINK_STATUS, &result);
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		std::string message;
		message.resize(log_length);
		glGetProgramInfoLog(id, log_length, NULL, message.data());
		throw std::runtime_error(
			std::string("Failed to link shader!\n") + message);
	}

	glDetachShader(id, vertex);
	glDetachShader(id, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return id;
}

GLuint Shader::compile(const GLchar* src, GLuint type)
{
	GLuint id = glCreateShader(type);

	GLint result;
	int log_length;
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		std::string message;
		message.resize(log_length);
		glGetShaderInfoLog(id, log_length, NULL, message.data());
		throw std::runtime_error(
			std::string("Failed to compile shader!\n") + message);
	}
	
	return id;
}

Shaders::Shaders() :
	textured(
		R"(
			#version 120
			
			uniform mat4 view;
			uniform mat4 projection;
			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec2 uv;
			varying vec4 shading;

			void main() {
				gl_Position = projection * view * inst_matrix * vec4(position, 1);
				uv = vec2(tex_coord.x, -tex_coord.y);
				shading = vec4(vec3(abs(normal.x + normal.y + normal.z) / 10.f), 0.f);
			}
		)",
		R"(
			#version 120
			
			uniform vec4 colour;
			uniform sampler2D sampler;
			varying vec2 uv;
			varying vec4 shading;
			
			void main() {
				gl_FragColor = texture2D(sampler, vec2(uv.x, 1.f - uv.y)) * colour - shading;
			}
		)",
		[&](GLuint id) {
			glBindAttribLocation(id, 0, "inst_matrix");
			glBindAttribLocation(id, 4, "inst_colour");
			glBindAttribLocation(id, 5, "inst_id");
			glBindAttribLocation(id, 6, "position");
			glBindAttribLocation(id, 7, "normal");
			glBindAttribLocation(id, 8, "tex_coord");
		},
		[&](GLuint id) {
			textured_view_matrix = glGetUniformLocation(id, "view");
			textured_projection_matrix = glGetUniformLocation(id, "projection");
			
			textured_colour = glGetUniformLocation(id, "colour");
			textured_sampler = glGetUniformLocation(id, "sampler");
		}
	),
	selection(
		R"(
			#version 120
			
			uniform mat4 view;
			uniform mat4 projection;
			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec4 inst_colour_frag;
			
			void main() {
				gl_Position = projection * view * inst_matrix * vec4(position, 1) - vec4(0, 0, 0.0001, 0);
				inst_colour_frag = inst_colour;
			}
		)",
		R"(
			#version 120
			
			varying vec4 inst_colour_frag;
			
			void main() {
				gl_FragColor = inst_colour_frag;
			}
		)",
		[&](GLuint id) {
			glBindAttribLocation(id, 0, "inst_matrix");
			glBindAttribLocation(id, 4, "inst_colour");
			glBindAttribLocation(id, 5, "inst_id");
			glBindAttribLocation(id, 6, "position");
			glBindAttribLocation(id, 7, "normal");
			glBindAttribLocation(id, 8, "tex_coord");
		},
		[&](GLuint id) {
			selection_view_matrix = glGetUniformLocation(id, "view");
			selection_projection_matrix = glGetUniformLocation(id, "projection");
		}
	),
	icons(
		R"(
			#version 120
			
			uniform mat4 view;
			uniform mat4 projection;
			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec2 uv;

			void main() {
				vec3 cam_right = vec3(view[0][0], view[1][0], view[2][0]);
				vec3 cam_up = vec3(view[0][1], view[1][1], view[2][1]);
				vec3 pos = vec3(inst_matrix[3])
					+ cam_right * position.x
					+ cam_up * position.y;
				vec4 point_pos = projection * view * vec4(pos, 1);
				vec4 centre_pos = projection * view * inst_matrix[3];
				gl_Position = vec4(point_pos.x, point_pos.y, centre_pos.z, centre_pos.w);
				uv = tex_coord;
			}
		)",
		R"(
			#version 120
			
			uniform sampler2D sampler;
			varying vec2 uv;
			
			void main() {
				gl_FragColor = texture2D(sampler, uv);
				if (gl_FragColor.a < 0.001) {
					discard;
				}
			}
		)",
		[&](GLuint id) {
			glBindAttribLocation(id, 0, "inst_matrix");
			glBindAttribLocation(id, 4, "inst_colour");
			glBindAttribLocation(id, 5, "inst_id");
			glBindAttribLocation(id, 6, "position");
			glBindAttribLocation(id, 7, "normal");
			glBindAttribLocation(id, 8, "tex_coord");
		},
		[&](GLuint id) {
			icons_view_matrix = glGetUniformLocation(id, "view");
			icons_projection_matrix = glGetUniformLocation(id, "projection");
			icons_sampler = glGetUniformLocation(id, "sampler");
		}
	),
	pickframe(
		R"(
			#version 120
			
			uniform mat4 view;
			uniform mat4 projection;
			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec4 inst_id_frag;
			
			void main() {
				gl_Position = projection * view * inst_matrix * vec4(position, 1);
				inst_id_frag = inst_id;
			}
		)",
		R"(
			#version 120
			
			varying vec4 inst_id_frag;
			
			void main() {
				gl_FragColor = inst_id_frag;
			}
		)",
		[&](GLuint id) {
			glBindAttribLocation(id, 0, "inst_matrix");
			glBindAttribLocation(id, 4, "inst_colour");
			glBindAttribLocation(id, 5, "inst_id");
			glBindAttribLocation(id, 6, "position");
			glBindAttribLocation(id, 7, "normal");
			glBindAttribLocation(id, 8, "tex_coord");
		},
		[&](GLuint id) {
			pickframe_view_matrix = glGetUniformLocation(id, "view");
			pickframe_projection_matrix = glGetUniformLocation(id, "projection");
		}
	),
	pickframe_icons(
		R"(
			#version 120
			
			uniform mat4 view;
			uniform mat4 projection;
			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec4 inst_id_frag;

			void main() {
				vec3 cam_right = vec3(view[0][0], view[1][0], view[2][0]);
				vec3 cam_up = vec3(view[0][1], view[1][1], view[2][1]);
				vec3 pos = vec3(inst_matrix[3])
					+ cam_right * position.x
					+ cam_up * position.y;
				vec4 point_pos = projection * view * vec4(pos, 1);
				vec4 centre_pos = projection * view * inst_matrix[3];
				gl_Position = vec4(point_pos.x, point_pos.y, centre_pos.z, centre_pos.w);
				inst_id_frag = inst_id;
			}
		)",
		R"(
			#version 120
			
			varying vec4 inst_id_frag;
			
			void main() {
				gl_FragColor = inst_id_frag;
			}
		)",
		[&](GLuint id) {
			glBindAttribLocation(id, 0, "inst_matrix");
			glBindAttribLocation(id, 4, "inst_colour");
			glBindAttribLocation(id, 5, "inst_id");
			glBindAttribLocation(id, 6, "position");
			glBindAttribLocation(id, 7, "normal");
			glBindAttribLocation(id, 8, "tex_coord");
		},
		[&](GLuint id) {
			pickframe_icons_view_matrix = glGetUniformLocation(id, "view");
			pickframe_icons_projection_matrix = glGetUniformLocation(id, "projection");
		}
	)
{}

void Shaders::init()
{
	textured.init();
	selection.init();
	icons.init();
	pickframe.init();
	pickframe_icons.init();
}
