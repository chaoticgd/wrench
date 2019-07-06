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

#include "gui.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <functional>

#include "menu.h"
#include "window.h"
#include "renderer.h"
#include "inspector.h"

void gui::render(app& a) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	render_menu_bar(a);

	for(auto& current_window : a.windows) {
		if(current_window.get() == nullptr) {
			continue;
		}
		ImGui::SetNextWindowSize(current_window->initial_size(), ImGuiCond_Appearing);
		std::string title =
			std::string(current_window->title_text()) +
			"##" + std::to_string(current_window->id());
	 	ImGui::SetNextWindowSize(current_window->initial_size(), ImGuiCond_FirstUseEver);
		if(ImGui::Begin(title.c_str())) {
			current_window->render(a);
		}
		ImGui::End();
	}
}

void gui::render_menu_bar(app& a) {
	bool no_more_undo_levels = false;
	bool no_more_redo_levels = false;

	ImGui::BeginMainMenuBar();
	if(ImGui::BeginMenu("File")) {
		if(ImGui::BeginMenu("New")) {
			if(ImGui::MenuItem("R&C2 PAL Project")) {
				file_new_project(a);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	/*if(ImGui::BeginMenu("Edit")) {
		a.if_level([&no_more_undo_levels, &no_more_redo_levels](level& lvl) {
			if(ImGui::MenuItem("Undo")) {
				no_more_undo_levels = !lvl.undo();
			}
			if(ImGui::MenuItem("Redo")) {
				no_more_redo_levels = !lvl.redo();
			}
		});
		ImGui::EndMenu();
	}*/
	if(ImGui::BeginMenu("Windows")) {
		render_menu_bar_window_toggle<iso_tree>(a);
		render_menu_bar_window_toggle<three_d_view>(a, &a);
		render_menu_bar_window_toggle<moby_list>(a);
		render_menu_bar_window_toggle<inspector<inspector_reflector>>(a, a.reflector.get());
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<string_viewer>(a);
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();

	if(no_more_undo_levels || no_more_redo_levels) {
		std::string message =
			std::string("Nothing to ") +
			(no_more_undo_levels ? "undo" : "redo") + ".";
		auto window = std::make_unique<message_box>("Error", message);
		a.windows.emplace_back(std::move(window));
	}
}

void gui::file_new_project(app& a) {
	auto path_input = std::make_unique<string_input>("Enter Game ISO Path");
	path_input->on_okay([](app& a, std::string path) {
		a.open_iso(path);
	});
	a.windows.emplace_back(std::move(path_input));
}

/*
	iso_tree
*/

const char* gui::iso_tree::title_text() const {
	return "ISO Tree";
}

ImVec2 gui::iso_tree::initial_size() const {
	return ImVec2(400, 800);
}

void gui::iso_tree::render(app& a) {
	a.bind_iso([&a](stream& root) {
		ImGui::Columns(2);

		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Resource Path");
		ImGui::NextColumn();
		ImGui::Text("----");
		ImGui::NextColumn();
		ImGui::Text("-------------");
		ImGui::NextColumn();

		render_tree_node(a, &root, 0);
	});
}

void gui::iso_tree::render_tree_node(app& a, stream* node, int depth) {
	ImGui::PushID(*reinterpret_cast<int*>(&node));

	// This is rather hacky. I should clean it up later.
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
	bool node_open = ImGui::TreeNodeEx("", flags);
	ImGui::SameLine();
	std::string button_text = node->display_name;
	if(node == a.selection) {
		button_text = std::string("*** ") + button_text;
	} 
	if(ImGui::Button(button_text.c_str())) {
		a.selection = node;
	}
	ImGui::NextColumn();

	ImGui::Text("%s", node->resource_path().c_str());
	ImGui::NextColumn();

	if(node_open) {
		if(!node->is_populated) {
			node->populate(&a);
		}
		for(auto& child : node->children()) {
			render_tree_node(a, child, depth + 1);
		}
		ImGui::TreePop();
	}
	ImGui::PopID();
}

/*
	moby_list
*/

const char* gui::moby_list::title_text() const {
	return "Moby List";
}

ImVec2 gui::moby_list::initial_size() const {
	return ImVec2(250, 500);
}

void gui::moby_list::render(app& a) {
	a.bind_level([](const level& lvl) {
		ImVec2 size = ImGui::GetWindowSize();
		size.x -= 16;
		size.y -= 64;

		ImGui::Text("UID  Class");

		ImGui::PushItemWidth(-1);
		ImGui::ListBoxHeader("##nolabel", size);
		for(const auto& [uid, moby] : lvl.mobies()) {
			std::stringstream row;
			row << std::setfill(' ') << std::setw(4) << std::dec << uid << " ";
			row << std::setfill(' ') << std::setw(16) << std::hex << moby->class_name() << " ";

			//bool is_selected = lvl.selection.find(uid) != lvl.selection.end();
			//if(ImGui::Selectable(row.str().c_str(), is_selected)) {
			//	// lvl.selection = { uid };
			//}
		}
		ImGui::ListBoxFooter();
		ImGui::PopItemWidth();
	});
}

/*
	viewport_information
*/

const char* gui::viewport_information::title_text() const {
	return "Viewport Information";
}

ImVec2 gui::viewport_information::initial_size() const {
	return ImVec2(250, 150);
}

void gui::viewport_information::render(app& a) {
	if(auto view = a.get_3d_view()) {

		glm::vec3 cam_pos = (*view)->camera_position;
		ImGui::Text("Camera Position:\n\t%.3f, %.3f, %.3f",
			cam_pos.x, cam_pos.y, cam_pos.z);
		glm::vec2 cam_rot = (*view)->camera_rotation;
		ImGui::Text("Camera Rotation:\n\tPitch=%.3f, Yaw=%.3f",
			cam_rot.x, cam_rot.y);
		ImGui::Text("Camera Control (Z to toggle):\n\t%s",
			(*view)->camera_control ? "On" : "Off");

		if(ImGui::Button("Reset Camera")) {
			(*view)->reset_camera(a);
		}
	}
}

/*
	string_viewer
*/

const char* gui::string_viewer::title_text() const {
	return "String Viewer";
}

ImVec2 gui::string_viewer::initial_size() const {
	return ImVec2(500, 400);
}

void gui::string_viewer::render(app& a) {
	/*a.bind_level([=, &a](const level& lvl) {

		ImGui::Columns(2);

		ImGui::SetColumnWidth(0, 64);
		if(ImGui::Button("Export")) {
			auto string_exporter = std::make_unique<string_input>("Enter Export Path");
			string_exporter->on_okay([=](app& a, std::string path) {
				a.if_level([=](const level_impl& lvl) {
					auto lang = std::find_if(lvl.strings.begin(), lvl.strings.end(),
						[=](auto& ptr) { return ptr.first == _selected_language; });
					if(lang == lvl.strings.end()) {
						return;
					}
					std::ofstream out_file(path);
					for(auto& [id, string] : lang->second) {
						out_file << std::hex << id << ": " << string << "\n";
					}
				});
			});
			a.windows.emplace_back(std::move(string_exporter));
		}

		ImGui::NextColumn();

		for(auto& language : lvl.strings) {
			if(ImGui::Button(language.first.c_str())) {
				_selected_language = language.first;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::Columns(1);

		auto lang = std::find_if(lvl.strings.begin(), lvl.strings.end(),
			[=](auto& ptr) { return ptr.first == _selected_language; });
		if(lang == lvl.strings.end()) {
			return;
		}

		ImGui::BeginChild(1);
		for(auto& string : lang->second) {
			ImGui::Text("%x: %s", string.first, string.second.c_str());
		}
		ImGui::EndChild();
	});*/
}

/*
	message_box
*/

gui::message_box::message_box(const char* title, std::string message)
	: _title(title), _message(message) {}

const char* gui::message_box::title_text() const {
	return _title;
}

ImVec2 gui::message_box::initial_size() const {
	return ImVec2(300, 200);
}

void gui::message_box::render(app& a) {
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 16;
	size.y -= 64;
	ImGui::PushItemWidth(-1);
	ImGui::InputTextMultiline("##nolabel", &_message, size, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();
	if(ImGui::Button("Close")) {
		close(a);
	}
}

/*
	string_input
*/

gui::string_input::string_input(const char* title)
	: _title_text(title) {
	_buffer.resize(1024);
}

const char* gui::string_input::title_text() const {
	return _title_text;
}

ImVec2 gui::string_input::initial_size() const {
	return ImVec2(400, 100);
}

void gui::string_input::render(app& a) {
	ImGui::InputText("", _buffer.data(), 1024);
	bool pressed = ImGui::Button("Okay");
	if(pressed) {
		_callback(a, std::string(_buffer.begin(), _buffer.end()));
	}
	pressed |= ImGui::Button("Cancel");
	if(pressed) {
		close(a);
	}
}

void gui::string_input::on_okay(std::function<void(app&, std::string)> callback) {
	_callback = callback;
}
