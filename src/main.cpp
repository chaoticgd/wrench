#include "command_line.h"
#include "window.h"

int main(int argc, char** argv) {
	if(!parse_command_line_args(argc, argv)) {
		return 0;
	}

	window w;
	while(!glfwWindowShouldClose(w.get())) {

	}
}
