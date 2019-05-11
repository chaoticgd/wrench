#include "command_line.h"
#include "gui.h"
#include "app.h"

int main(int argc, char** argv) {

	if(!parse_command_line_args(argc, argv)) {
		return 0;
	}

	app a;
	while(!glfwWindowShouldClose(a.main_window.get())) {
		glfwPollEvents();

		gui::render(a);

		ImGui::Render();
		int display_w, display_h;
		glfwMakeContextCurrent(a.main_window.get());
		glfwGetFramebufferSize(a.main_window.get(), &display_w, &display_h);

		glViewport(0, 0, display_w, display_h);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwMakeContextCurrent(a.main_window.get());
		glfwSwapBuffers(a.main_window.get());
	}

}
