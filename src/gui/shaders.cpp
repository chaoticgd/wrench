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

Shader::Shader(const GLchar* vertex_src, const GLchar* fragment_src, ShaderCallback before, ShaderCallback after)
	: _id(0), _vertex_src(vertex_src), _fragment_src(fragment_src), _before(before), _after(after) {}
	
Shader::~Shader() {
	if(_id) {
		glDeleteProgram(_id);
	}
}

void Shader::init() {
	_id = link(
		compile(_vertex_src,   GL_VERTEX_SHADER),
		compile(_fragment_src, GL_FRAGMENT_SHADER));
}

GLuint Shader::id() const {
	return _id;
}

GLuint Shader::link(GLuint vertex, GLuint fragment) {
	GLuint id = glCreateProgram();
	glAttachShader(id, vertex);
	glAttachShader(id, fragment);
	
	_before(id);
	glLinkProgram(id);
	_after(id);

	GLint result;
	int log_length;
	glGetProgramiv(id, GL_LINK_STATUS, &result);
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0) {
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

GLuint Shader::compile(const GLchar* src, GLuint type) {
	GLuint id = glCreateShader(type);

	GLint result;
	int log_length;
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0) {
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

			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec2 uv;
			varying vec4 shading;

			void main() {
				gl_Position = inst_matrix * vec4(position, 1);
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
			textured_colour = glGetUniformLocation(id, "colour");
			textured_sampler = glGetUniformLocation(id, "sampler");
		}
	),
	selection(
		R"(
			#version 120

			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec4 inst_colour_frag;

			void main() {
				gl_Position = inst_matrix * vec4(position, 1) - vec4(0, 0, 0.0001, 0);
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
		[&](GLuint id) {}
	),
	pickframe(
		R"(
			#version 120

			attribute mat4 inst_matrix;
			attribute vec4 inst_colour;
			attribute vec4 inst_id;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec4 inst_id_frag;

			void main() {
				gl_Position = inst_matrix * vec4(position, 1);
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
		[&](GLuint id) {}
	)
{}

void Shaders::init() {
	textured.init();
	selection.init();
	pickframe.init();
}
