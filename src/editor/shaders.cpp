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

Shader::Shader(const GLchar* vertex_src, const GLchar* fragment_src, shader_callback after)
	: _id(0), _vertex_src(vertex_src), _fragment_src(fragment_src), _after(after) {}
	
Shader::~Shader() {
	glDeleteProgram(_id);
}

void Shader::init() {
	_id = link(
		compile(_vertex_src,   GL_VERTEX_SHADER),
		compile(_fragment_src, GL_FRAGMENT_SHADER));
	_after(_id);
}

GLuint Shader::id() const {
	return _id;
}

GLuint Shader::link(GLuint vertex, GLuint fragment) {
	GLuint id = glCreateProgram();
	glAttachShader(id, vertex);
	glAttachShader(id, fragment);
	glLinkProgram(id);

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

shader_programs::shader_programs() :
	solid_colour(
		R"(
			#version 120

			attribute mat4 local_to_clip;
			attribute vec3 position;

			void main() {
				gl_Position = local_to_clip * vec4(position, 1);
			}
		)",
		R"(
			#version 120

			uniform vec4 rgb;

			void main() {
				gl_FragColor = rgb;
			}
		)",
		[&](GLuint id) {
			solid_colour_rgb = glGetUniformLocation(id, "rgb");
			
			glBindAttribLocation(id, 0, "local_to_clip");
			glBindAttribLocation(id, 1, "position");
		}
	),
	textured(
		R"(
			#version 120

			attribute mat4 local_to_clip;
			attribute vec3 position;
			attribute vec3 normal;
			attribute vec2 tex_coord;
			varying vec2 uv_frag;

			void main() {
				gl_Position = local_to_clip * vec4(position, 1);
				uv_frag = tex_coord;
			}
		)",
		R"(
			#version 120

			uniform sampler2D sampler;
			varying vec2 uv_frag;

			void main() {
				gl_FragColor = vec4(1,0,0,1);texture2D(sampler, uv_frag);
			}
		)",
		[&](GLuint id) {
			textured_sampler = glGetUniformLocation(id, "sampler");
			
			glBindAttribLocation(id, 0, "local_to_clip");
			glBindAttribLocation(id, 4, "position");
			glBindAttribLocation(id, 5, "normal");
			glBindAttribLocation(id, 6, "tex_coord");
		}
	)
{}

void shader_programs::init() {
	solid_colour.init();
	textured.init();
}
