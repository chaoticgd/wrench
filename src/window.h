#ifndef WINDOW_H
#define WINDOW_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

class window {
public:
	window();
	~window();

	GLFWwindow* get();
private:
	GLFWwindow* _window;
};

#endif
