#include "gui.h"

#include <iostream>

#include "menu.h"

void gui::render(app& a) {

	static const std::initializer_list<menu> menu_items = {
		{
			"File", {
				{ "New", file_new }
			}
		}
	};

	new_frame();

	ImGui::BeginMainMenuBar();
	for(auto& menu : menu_items) {
		menu.render(a);
	}
	ImGui::EndMainMenuBar();
}

void gui::file_new(app& a) {
	std::cout << "test\n";
}

void gui::new_frame() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}
