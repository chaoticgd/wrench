/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include <cmath>
#include <cstdint>
#include <nfd.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <functional>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <gui/config.h>
#include "../icons.h"
#include "../util.h"
#include "../renderer.h"
#include "../unwindows.h"
#include "window.h"

void gui::render(app& a) {
	float menu_height = render_menu_bar(a);
	render_tools(a, menu_height);

	begin_docking();

	for(auto& current_window : a.windows) {
		if(current_window.get() == nullptr) {
			continue;
		}
		
		bool has_padding = current_window->has_padding();
		if(!has_padding) {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		}
		
		std::string title = current_window->title_text();
		if(!current_window->is_unique()) {
			title += "##" + std::to_string(current_window->id());
		}
		
	 	ImGui::SetNextWindowSize(current_window->initial_size(), ImGuiCond_FirstUseEver);
		if(ImGui::Begin(title.c_str())) {
			current_window->render(a);
		}
		ImGui::End();
		
		if(!has_padding) {
			ImGui::PopStyleVar();
		}
	}
	
	static bool is_first_frame = true;
	if(is_first_frame) {
		gui::create_dock_layout(a);
		is_first_frame = false;
	}
	
	ImGui::End(); // docking
}

void gui::create_dock_layout(const app& a) {
	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1.f, 1.f));

	ImGuiID left_centre, right;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 8.f / 10.f, &left_centre, &right);
	
	ImGuiID left, centre;
	ImGui::DockBuilderSplitNode(left_centre, ImGuiDir_Left, 2.f / 10.f, &left, &centre);
	
	ImGuiID inspector, middle_right;
	ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 1.f / 2.f, &inspector, &middle_right);
	
	ImGuiID mobies, viewport_info;
	ImGui::DockBuilderSplitNode(middle_right, ImGuiDir_Up, 1.f / 2.f, &mobies, &viewport_info);
	
	ImGui::DockBuilderDockWindow("Project Tree", left);
	ImGui::DockBuilderDockWindow("3D View", centre);
	ImGui::DockBuilderDockWindow("Inspector", inspector);
	ImGui::DockBuilderDockWindow("Mobies", mobies);
	ImGui::DockBuilderDockWindow("Viewport Information", viewport_info);

	ImGui::DockBuilderFinish(dockspace_id);
}

void gui::begin_docking() {
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	
	// Make room for the tools.
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 pos = viewport->Pos;
	ImVec2 size = viewport->Size;
	pos.x += 55 * g_config.ui.scale;
	size.x -= 55 * g_config.ui.scale;
	
	ImGui::SetNextWindowPos(pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	static bool p_open;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("dock_space", &p_open, window_flags);
	ImGui::PopStyleVar();
	
	ImGui::PopStyleVar(2);

	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

float gui::render_menu_bar(app& a) {
	static alert_box message_box("Information");
	message_box.render();
	
	
	ImGui::BeginMainMenuBar();
	if(ImGui::BeginMenu("File")) {
		ImGui::EndMenu();
	}
	
	static alert_box save_error_box("Error Saving Project");
	
	static alert_box undo_error_box("Undo Error");
	static alert_box redo_error_box("Redo Error");
	undo_error_box.render();
	redo_error_box.render();
	
	if(ImGui::BeginMenu("Edit")) {
		if(auto lvl = a.get_level()) {
			if(ImGui::MenuItem("Undo")) {
				try {
					lvl->history.undo();
				} catch(UndoRedoError& error) {
					undo_error_box.open(error.what());
				}
			}
			if(ImGui::MenuItem("Redo")) {
				try {
					lvl->history.redo();
				} catch(UndoRedoError& error) {
					redo_error_box.open(error.what());
				}
			}
		} else {
			ImGui::Text("<no level>");
		}
		ImGui::EndMenu();
	}
	
	if(ImGui::BeginMenu("View")) {
		if(ImGui::MenuItem("Reset Camera")) {
			reset_camera(&a);
		}
		if(ImGui::BeginMenu("Visibility")) {
			ImGui::Checkbox("Ties", &a.render_settings.draw_ties);
			ImGui::Checkbox("Shrubs", &a.render_settings.draw_shrubs);
			ImGui::Checkbox("Mobies", &a.render_settings.draw_mobies);
			ImGui::Checkbox("Cuboids", &a.render_settings.draw_cuboids);
			ImGui::Checkbox("Spheres", &a.render_settings.draw_spheres);
			ImGui::Checkbox("Cylinders", &a.render_settings.draw_cylinders);
			ImGui::Checkbox("Paths", &a.render_settings.draw_paths);
			ImGui::Checkbox("Grind Paths", &a.render_settings.draw_grind_paths);
			ImGui::Checkbox("Tfrags", &a.render_settings.draw_tfrags);
			ImGui::Checkbox("Collision", &a.render_settings.draw_collision);
			ImGui::Separator();
			ImGui::Checkbox("Selected Moby Normals", &a.render_settings.draw_selected_moby_normals);
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	
	static alert_box emu_error_box("Error");
	emu_error_box.render();
	
	if(ImGui::BeginMenu("Windows")) {
		render_menu_bar_window_toggle<view_3d>(a);
		render_menu_bar_window_toggle<moby_list>(a);
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<Inspector>(a);
		ImGui::EndMenu();
	}
	
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	
	if(ImGui::BeginTabBar("layouts")) {
		if(ImGui::BeginTabItem("Asset Browser")) {
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Level Editor")) {
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	float menu_bar_height = ImGui::GetWindowSize().y;
	ImGui::EndMainMenuBar();
	return menu_bar_height;
}

void gui::render_tools(app& a, float menu_bar_height) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGuiViewport* view = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(-1, menu_bar_height - 1));
	
	ImGui::SetNextWindowSize(ImVec2(56 * g_config.ui.scale, view->Size.y));
	ImGui::Begin("Tools", nullptr,
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove);
	ImGui::PopStyleVar();
	
	for(std::size_t i = 0 ; i < a.tools.size(); i++) {
		bool active = i == a.active_tool_index;
		if(!active) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		}

		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) a.tools[i]->icon(), ImVec2(32*g_config.ui.scale, 32*g_config.ui.scale),
						ImVec2(0, 0), ImVec2(1, 1), -1);
		if(!active) {
			ImGui::PopStyleColor();
		}
		if(clicked) {
			a.active_tool_index = i;
		}
	}
	
	ImGui::End();
}

/*
	moby_list
*/

const char* gui::moby_list::title_text() const {
	return "Mobies";
}

ImVec2 gui::moby_list::initial_size() const {
	return ImVec2(250, 500);
}

void gui::moby_list::render(app& a) {
	if(a.get_level() == nullptr) {
		ImGui::Text("<no level>");
		return;
	}

	Level& lvl = *a.get_level();
	
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 16;
	size.y -= 64;
	ImGui::Text("     UID                Class");
	ImGui::PushItemWidth(-1);
	if(ImGui::ListBoxHeader("##mobylist", size)) {
		for(MobyInstance& inst : opt_iterator(lvl.gameplay().moby_instances)) {
			std::stringstream row;
			row << std::setfill(' ') << std::setw(8) << std::dec << inst.uid << " ";
			row << std::setfill(' ') << std::setw(20) << std::hex << inst.o_class << " ";
			
			if(ImGui::Selectable(row.str().c_str(), inst.selected)) {
				lvl.gameplay().clear_selection();
				inst.selected = true;
			}
		}
		ImGui::ListBoxFooter();
	}
	ImGui::PopItemWidth();
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
	ImGui::Text("Frame Time (ms):\n\t%.2f\n", a.delta_time / 1000.f);
	glm::vec3 cam_pos = a.render_settings.camera_position;
	ImGui::Text("Camera Position:\n\t%.3f, %.3f, %.3f",
		cam_pos.x, cam_pos.y, cam_pos.z);
	glm::vec2 cam_rot = a.render_settings.camera_rotation;
	ImGui::Text("Camera Rotation:\n\tPitch=%.3f, Yaw=%.3f",
		cam_rot.x, cam_rot.y);
	ImGui::Text("Camera Control (Z to toggle):\n\t%s",
		a.render_settings.camera_control ? "On" : "Off");
}

/*
	alert_box
*/

void gui::alert_box::render() {
	if(_is_open) {
		ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
		ImGui::Begin(_title);
		
		ImVec2 size = ImGui::GetWindowSize();
		size.x -= 16;
		size.y -= 64;
		
		ImGui::PushItemWidth(-1);
		ImGui::InputTextMultiline("##message", &_text, size, ImGuiInputTextFlags_ReadOnly);
		ImGui::PopItemWidth();
		if(ImGui::Button("Close")) {
			_is_open = false;
		}
		ImGui::End();
	}
}

void gui::alert_box::open(std::string new_text) {
	_is_open = true;
	_text = new_text;
}

std::optional<std::string> gui::prompt_box::prompt() {
	if(ImGui::Button(_button_text)) {
		open();
	}
	return render();
}

std::optional<std::string> gui::prompt_box::render() {
	std::optional<std::string> result;
	if(_is_open) {
		ImGui::SetNextWindowSize(ImVec2(400, 100));
		ImGui::Begin(_title);
		ImGui::InputText("##input", &_text);
		if(ImGui::Button("Okay")) {
			_is_open = false;
			result = _text;
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			_is_open = false;
		}
		ImGui::End();
	}
	return result;
}

void gui::prompt_box::open() {
	_is_open = true;
	_text = "";
}

gui::hex_dump::hex_dump(uint8_t* data, std::size_t size_in_bytes) {
	// TODO: Make to_hex_dump work on individual bytes.
	_lines = to_hex_dump((uint32_t*) data, 0, size_in_bytes / 4);
}
	
const char* gui::hex_dump::title_text() const {
	return "Hex Dump";
}

ImVec2 gui::hex_dump::initial_size() const {
	return ImVec2(300, 200);
}

void gui::hex_dump::render(app& a) {
	for(std::string& line : _lines) {
		ImGui::Text("%s", line.c_str());
	}
	
	if(ImGui::Button("Close")) {
		close(a);
	}
}
