#include "window.h"

#include <stdexcept>
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

window::window() {
	if(!glfwInit()) {
		throw std::runtime_error("Cannot load GLFW.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	_window = glfwCreateWindow(1280, 720, "Wrench", NULL, NULL);
	if(_window == nullptr) {
		throw std::runtime_error("Cannot create GLFW window.");
	}

	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);

	if(glewInit() != GLEW_OK) {
		throw std::runtime_error("Cannot load GLEW.");
	}

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

window::~window() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(_window);
	glfwTerminate();
}

GLFWwindow* window::get() {
	return _window;
}
