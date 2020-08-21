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

shader_program::shader_program(const GLchar* vertex_src, const GLchar* fragment_src, shader_callback after)
	: _id(0), _vertex_src(vertex_src), _fragment_src(fragment_src), _after(after) {}
	
shader_program::~shader_program() {
	glDeleteProgram(_id);
}

void shader_program::init() {
	_id = link(
		compile(_vertex_src,   GL_VERTEX_SHADER),
		compile(_fragment_src, GL_FRAGMENT_SHADER));
	_after(_id);
}

GLuint shader_program::id() const {
	return _id;
}

GLuint shader_program::link(GLuint vertex, GLuint fragment) {
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

GLuint shader_program::compile(const GLchar* src, GLuint type) {
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

			uniform mat4 transform;
			attribute vec3 position_model_space;

			void main() {
				gl_Position = transform * vec4(position_model_space, 1);
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
			solid_colour_transform = glGetUniformLocation(id, "transform");
			solid_colour_rgb       = glGetUniformLocation(id, "rgb");
		}
	),
	solid_colour_batch(
		R"(
			#version 120

			attribute mat4 local_to_clip;
			attribute vec3 position_model_space;

			void main() {
				gl_Position = local_to_clip * vec4(position_model_space, 1);
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
			solid_colour_batch_rgb = glGetUniformLocation(id, "rgb");
			
			glBindAttribLocation(id, 0, "local_to_clip");
		}
	),
	textured(
		R"(
			#version 120

			attribute mat4 local_to_clip;
			attribute vec3 position_model_space;
			attribute vec2 uv;
			varying vec2 uv_frag;

			void main() {
				gl_Position = local_to_clip * vec4(position_model_space, 1);
				uv_frag = uv * vec2(8.f, 8.f);
			}
		)",
		R"(
			#version 120

			uniform sampler2D sampler;
			varying vec2 uv_frag;

			void main() {
				gl_FragColor = texture2D(sampler, uv_frag);
			}
		)",
		[&](GLuint id) {
			textured_sampler = glGetUniformLocation(id, "sampler");
			
			glBindAttribLocation(id, 0, "local_to_clip");
			glBindAttribLocation(id, 4, "position_model_space");
			glBindAttribLocation(id, 5, "uv");
		}
	)
{}

void shader_programs::init() {
	solid_colour.init();
	textured.init();
}
