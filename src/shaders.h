#ifndef SHADERS_H
#define SHADERS_H

#include <stdexcept>
#include <functional>
#include <GL/glew.h>

class shader_program {
public:
	using shader_callback = std::function<void(GLuint id)>;

	shader_program(const GLchar* vertex_src, const GLchar* fragment_src, shader_callback after);
	~shader_program();

	GLuint id();

private:
	static GLuint link(GLuint vertex, GLuint fragment);
	static GLuint compile(const GLchar* src, GLuint type);

	bool _ready;
	GLuint _id;
	const GLchar* _vertex_src;
	const GLchar* _fragment_src;
	shader_callback _after;
};

struct shader_programs {
	shader_programs();

	shader_program solid_colour;
	GLint solid_colour_transform;
	GLint solid_colour_rgb;
};

#endif
