#include "shaders.h"

#include <vector>

shader_program::shader_program(const GLchar* vertex_src, const GLchar* fragment_src, shader_callback after)
	: _ready(false), _id(0), _vertex_src(vertex_src), _fragment_src(fragment_src), _after(after) {}
	
shader_program::~shader_program() {
	if(_ready) {
		glDeleteProgram(_id);
	}
}

GLuint shader_program::id() {
	if(!_ready) {
		_id = link(
			compile(_vertex_src,   GL_VERTEX_SHADER),
			compile(_fragment_src, GL_FRAGMENT_SHADER));
		_after(_id);
		_ready = true;
	}
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

shader_programs::shader_programs()
	: solid_colour(
		R"(
			#version 330 core

			uniform mat4 transform;
			layout(location = 0) in vec3 position_model_space;

			void main() {
				gl_Position = transform * vec4(position_model_space, 1);
			}
		)",
		R"(
			#version 330 core

			uniform vec3 rgb;
			out vec3 colour;

			void main() {
				colour = rgb;
			}
		)",
		[=](GLuint id) {
			solid_colour_transform = glGetUniformLocation(id, "transform");
			solid_colour_rgb       = glGetUniformLocation(id, "rgb");
		}
	) {}
