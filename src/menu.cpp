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
