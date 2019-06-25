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

#include <iomanip>
#include <sstream>
#include <iostream>
#include <functional>

#include "menu.h"
#include "window.h"

void gui::render(app& a) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	a.if_level([&a](const level_impl& lvl) {
		// Draw floating text over each moby showing its class name.
		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
		for(const auto& [uid, moby] : lvl.mobies()) {
			if(moby->last_drawn_pos.z > 0 && moby->last_drawn_pos.z < 1) {
				ImVec2 position(
					(1 + moby->last_drawn_pos.x) * a.window_width / 2.0,
					(1 - moby->last_drawn_pos.y) * a.window_height / 2.0
				);
				static const int colour = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
				std::string class_name = moby->get_class_name();
				draw_list->AddText(position, colour, class_name.c_str());
			}
		}
	});

	render_menu_bar(a);

	for(auto& current_window : a.windows) {
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
	ImGui::BeginMainMenuBar();
	if(ImGui::BeginMenu("File")) {
		if(ImGui::BeginMenu("Import")) {
			if(ImGui::MenuItem("R&C2 Level (.wad)")) {
				file_import_rc2_level(a);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

void gui::file_import_rc2_level(app& a) {
	auto path_input = std::make_unique<string_input>("Enter File Path");
	path_input->on_okay([](app& a, std::string path) {
		a.import_level(path);
	});
	a.windows.emplace_back(std::move(path_input));
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
	a.if_level([](level& lvl, const level_impl& lvl_impl) {
		ImVec2 size = ImGui::GetWindowSize();
		size.x -= 16;
		size.y -= 64;

		ImGui::Text("UID  Class             Name");

		ImGui::PushItemWidth(-1);
		ImGui::ListBoxHeader("##nolabel", size);
		for(const auto& [uid, moby] : lvl_impl.mobies()) {
			std::stringstream row;
			row << std::setfill(' ') << std::setw(4) << std::dec << uid << " ";
			row << std::setfill(' ') << std::setw(16) << std::hex << moby->get_class_name() << " ";
			row << moby->name;

			bool is_selected = lvl.selection.find(uid) != lvl.selection.end();
			if(ImGui::Selectable(row.str().c_str(), is_selected)) {
				lvl.selection = { uid };
			}
		}
		ImGui::ListBoxFooter();
		ImGui::PopItemWidth();
	});
}

/*
	inspector
*/

const char* gui::inspector::title_text() const {
	return "Inspector";
}

ImVec2 gui::inspector::initial_size() const {
	return ImVec2(250, 500);
}

void gui::inspector::render(app& a) {
	if(!a.has_level()) {
		ImGui::Text("<no level open>");
		return;
	}

	a.if_level([](const level_impl& lvl) {
		if(lvl.selection.size() < 1) {
			ImGui::Text("<no selection>");
			return;
		} else if(lvl.selection.size() > 1) {
			ImGui::Text("<multiple mobies selected>");
			return;
		}

		moby* selected = lvl.mobies().at(*lvl.selection.begin()).get();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 80);

		int i = 0;
		auto begin_property = [&i](const char* name) {
			ImGui::PushID(i++);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", name);
			ImGui::NextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::PushItemWidth(-1);
		};

		auto end_property = []() {	
			ImGui::NextColumn();
			ImGui::PopID();
			ImGui::PopItemWidth();
		};

		selected->reflect(
			[=](const char* name, rf::property<uint16_t> p) {
				begin_property(name);
				int value = p.get();
				if(ImGui::InputInt("##nolabel", &value)) {
					p.set(value);
				}
				end_property();
			},
			[=](const char* name, rf::property<uint32_t> p) {
				begin_property(name);
				int value = p.get();
				if(ImGui::InputInt("##nolabel", &value)) {
					p.set(value);
				}
				end_property();
			},
			[=](const char* name, rf::property<glm::vec3> p) {
				begin_property(name);
				float components[] = { p.get().x, p.get().y, p.get().z };
				if(ImGui::InputFloat3("##nolabel", components)) {
					p.set(glm::vec3(components[0], components[1], components[2]));
				}
				end_property();
			},
			[=](const char* name, rf::property<std::string> p) {
				begin_property(name);
				std::string value = p.get();
				if(ImGui::InputText("##nolabel", &value)) {
					p.set(value);
				}
				end_property();
			}
		);

		ImGui::PopStyleVar();
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
	a.if_level([](level& lvl) {
		glm::vec3 cam_pos = lvl.camera_position;
		ImGui::Text("Camera Position:\n\t%.3f, %.3f, %.3f",
			cam_pos.x, cam_pos.y, cam_pos.z);
		glm::vec2 cam_rot = lvl.camera_rotation;
		ImGui::Text("Camera Rotation:\n\tPitch=%.3f, Yaw=%.3f",
			cam_rot.x, cam_rot.y);
		ImGui::Text("Camera Control (Z to toggle):\n\t%s",
			lvl.camera_control ? "On" : "Off");
		if(ImGui::Button("Reset Camera")) {
			lvl.reset_camera();
		}
	});
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
