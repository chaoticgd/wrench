#include "menu.h"

#include "gui.h"

menu::menu(const char* name, std::function<void(app&)> callback)
	: _name(name), _callback(callback) {}

menu::menu(const char* name, std::initializer_list<menu> children)
	: _name(name), _children(children) {}

void menu::render(app& a) const {
	if(_children.size() <= 0) {
		if(ImGui::MenuItem(_name, "")) {
			_callback(a);
		}
	} else {
		if(ImGui::BeginMenu(_name)) {
			for(auto& child : _children) {
				child.render(a);
			}
			ImGui::EndMenu();
		}
	}
}
