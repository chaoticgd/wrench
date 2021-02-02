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
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <stdlib.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>

#include "util.h"
#include "config.h"
#include "window.h"
#include "renderer.h"
#include "worker_thread.h"
#include "formats/bmp.h"

#include "unwindows.h"

void gui::render(app& a) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

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
	
	if(config::get().request_open_settings_dialog) {
		config::get().request_open_settings_dialog = false;
		a.emplace_window<gui::settings>();
	}
	
	ImGui::End(); // docking
}

void gui::create_dock_layout(const app& a) {
	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1.f, 1.f));

	ImGuiID centre, right;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 8.f / 10.f, &centre, &right);
	
	ImGuiID inspector, middle_right;
	ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 1.f / 2.f, &inspector, &middle_right);
	
	ImGuiID mobies, viewport_info;
	ImGui::DockBuilderSplitNode(middle_right, ImGuiDir_Up, 1.f / 2.f, &mobies, &viewport_info);
	
	ImGui::DockBuilderDockWindow("3D View", centre);
	ImGui::DockBuilderDockWindow("Texture Browser", centre);
	ImGui::DockBuilderDockWindow("Model Browser", centre);
	ImGui::DockBuilderDockWindow("Stream Viewer", centre);
	ImGui::DockBuilderDockWindow("Documentation", centre);
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
	pos.x += 55;
	size.x -= 55;
	
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
	ImGui::BeginMainMenuBar();
	
	bool save_in_place = false;
	static prompt_box save_as_box("Save As", "Enter New Path");
	
	static alert_box export_complete_box("Export Complete");
	export_complete_box.render();
	
	if(ImGui::BeginMenu("File")) {
		if(ImGui::BeginMenu("New")) {
			for(const game_iso& game : config::get().game_isos) {
				if(ImGui::MenuItem(game.path.c_str())) {
					a.new_project(game);
				}
			}
			ImGui::EndMenu();
		}
		if(ImGui::MenuItem("Open")) {
			auto dialog = a.emplace_window<file_dialog>
				("Open Project (.wrench)", file_dialog::open, std::vector<std::string> { ".wrench" });
			dialog->on_okay([&a](std::string path) {
				a.open_project(path);
			});
		}
		if(ImGui::MenuItem("Save")) {
			save_in_place = true;
		}
		if(ImGui::MenuItem("Save As")) {
			save_as_box.open();
		}
		if(ImGui::BeginMenu("Export")) {
			if(level* lvl = a.get_level()) {
				if(ImGui::MenuItem("Mobyseg (debug)")) {
					file_stream dump_file("mobyseg.bin", std::ios::out | std::ios::trunc);
					stream* src = lvl->moby_stream();
					src->seek(0);
					stream::copy_n(dump_file, *src, src->size());
				}
				if(ImGui::MenuItem("Code segment")) {
					std::stringstream name;
					name << "codeseg";
					name << "_" << std::hex << lvl->code_segment.header.base_address;
					name << "_" << std::hex << lvl->code_segment.header.unknown_4;
					name << "_" << std::hex << lvl->code_segment.header.unknown_8;
					name << "_" << std::hex << lvl->code_segment.header.entry_offset;
					name << ".bin";
					
					file_stream dump_file(name.str(), std::ios::out | std::ios::trunc);
					dump_file.write_v(lvl->code_segment.bytes);
					
					std::stringstream message;
					message << "The code segment for the current level has been written to\n\t\"";
					message << name.str() << "\"\n";
					message << "relative to the main Wrench directory.\n";
					message << "\n";
					message << "Base address: " << std::hex << lvl->code_segment.header.base_address << "\n";
					message << "Unknown (0x4): " << std::hex << lvl->code_segment.header.unknown_4 << "\n";
					message << "Unknown (0x8): " << std::hex << lvl->code_segment.header.unknown_8 << "\n";
					message << "Entry point: " << std::hex << lvl->code_segment.header.entry_offset << "\n";
					export_complete_box.open(message.str());
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	
	static alert_box save_error_box("Error Saving Project");
	
	if(auto project = a.get_project()) {
		auto save_as_new_path = save_as_box.render();
		if(save_as_new_path) {
			project->set_project_path(*save_as_new_path);
		}
		if(save_as_new_path || save_in_place) {
			try {
				project->save();
				auto window_title = std::string("Wrench Editor - [") + project->project_path() + "]";
				glfwSetWindowTitle(a.glfw_window, window_title.c_str());
			} catch(stream_error& err) {
				std::stringstream error_message;
				error_message << err.what() << "\n";
				error_message << err.stack_trace;
				save_error_box.open(error_message.str());
			}
		}
	}
	
	static alert_box undo_error_box("Undo Error");
	static alert_box redo_error_box("Redo Error");
	undo_error_box.render();
	redo_error_box.render();
	
	if(ImGui::BeginMenu("Edit")) {
		if(auto project = a.get_project()) {
			if(ImGui::MenuItem("Undo")) {
				try {
					project->undo();
				} catch(command_error& error) {
					undo_error_box.open(error.what());
				}
			}
			if(ImGui::MenuItem("Redo")) {
				try {
					project->redo();
				} catch(command_error& error) {
					redo_error_box.open(error.what());
				}
			}
		}
		ImGui::EndMenu();
	}
	
	if(ImGui::BeginMenu("View")) {
		if(ImGui::MenuItem("Reset Camera")) {
			a.renderer.reset_camera(&a);
		}
		if(ImGui::BeginMenu("View Mode")) {
			if(ImGui::RadioButton("Wireframe", a.renderer.mode == view_mode::WIREFRAME)) {
				a.renderer.mode = view_mode::WIREFRAME;
			}
			if(ImGui::RadioButton("Textured Polygons", a.renderer.mode == view_mode::TEXTURED_POLYGONS)) {
				a.renderer.mode = view_mode::TEXTURED_POLYGONS;
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Visibility")) {
			ImGui::Checkbox("Ties", &a.renderer.draw_ties);
			ImGui::Checkbox("Shrubs", &a.renderer.draw_shrubs);
			ImGui::Checkbox("Mobies", &a.renderer.draw_mobies);
			ImGui::Checkbox("Triggers", &a.renderer.draw_triggers);
			ImGui::Checkbox("Splines", &a.renderer.draw_splines);
			ImGui::Checkbox("Grind Rails", &a.renderer.draw_grind_rails);
			ImGui::Checkbox("Tfrags", &a.renderer.draw_tfrags);
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	
	static alert_box emu_error_box("Error");
	emu_error_box.render();
	
	if(ImGui::BeginMenu("Emulator")) {
		if(ImGui::MenuItem("Run")) {
			if(auto project = a.get_project()) {
				if(auto lvl = a.get_level()) {
					lvl->write();
				}
				project->iso.commit(); // Recompress WAD segments.
				
				if(fs::is_regular_file(config::get().emulator_path)) {
					std::string emulator_path = fs::canonical(config::get().emulator_path).string();
					std::string cmd = emulator_path + " " + project->cached_iso_path();
					int result = system(cmd.c_str());
					if(result != 0) {
						emu_error_box.open("Failed to execute shell command.");
					}
				} else {
					emu_error_box.open("Invalid emulator path.");
				}
			} else {
				emu_error_box.open("No project open.");
			}
		}
		ImGui::EndMenu();
	}
	
	static const int level_list_rows = 20;
	
	if(auto project = a.get_project()) {
		auto& levels = project->toc.levels;
		int columns = (int) std::ceil(levels.size() / (float) level_list_rows);
		ImGui::SetNextWindowContentSize(ImVec2(columns * 192, 0.f));
		if(ImGui::BeginMenu("Levels")) {
			const std::map<std::size_t, std::string>* level_names = nullptr;
			for(const gamedb_game& game : a.game_db) {
				if(game.name == project->game.game_db_entry) {
					level_names = &game.levels;
				}
			}
			
			ImGui::Columns(columns);
			for(std::size_t i = 0; i < levels.size(); i++) {
				std::stringstream label;
				label << std::setfill('0') << std::setw(2) << i;
				if(level_names != nullptr && level_names->find(i) != level_names->end()) {
					label << " " << level_names->at(i);
				}
				if(ImGui::MenuItem(label.str().c_str())) {
					project->open_level(i);
					a.renderer.reset_camera(&a);
				}
				if(i % level_list_rows == level_list_rows - 1) {
					ImGui::SetColumnWidth(i / level_list_rows, 192); // Make the columns non-resizable.
					ImGui::NextColumn();
				}
			}
			ImGui::Columns();
			ImGui::EndMenu();
		}
	} else {
		// If no project is open, draw a dummy menu.
		if(ImGui::BeginMenu("Levels")) {
			ImGui::EndMenu();
		}
	}

	ImGui::SetNextWindowContentSize(ImVec2(0.f, 0.f)); // Reset the menu width after the "Levels" menu made it larger.
	if(ImGui::BeginMenu("Windows")) {
		render_menu_bar_window_toggle<view_3d>(a);
		render_menu_bar_window_toggle<moby_list>(a);
		render_menu_bar_window_toggle<inspector>(a);
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<string_viewer>(a);
		render_menu_bar_window_toggle<texture_browser>(a);
		render_menu_bar_window_toggle<model_browser>(a);
		render_menu_bar_window_toggle<settings>(a);
		render_menu_bar_window_toggle<document_viewer>(a, "index.md");
		ImGui::Separator();
		if(ImGui::BeginMenu("Debug Tools")) {
			render_menu_bar_window_toggle<manual_patcher>(a);
			render_menu_bar_window_toggle<stream_viewer>(a);
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	
	auto open_document_viewer = [&](const char* path) {
		// Close any existing documentation windows.
		for(auto& window : a.windows) {
			if(dynamic_cast<document_viewer*>(window.get()) != nullptr) {
				window->close(a);
			}
		}
		a.emplace_window<document_viewer>(path);
	};
	
	static alert_box about_box("About Wrench Editor");
	about_box.render();
	
	if(ImGui::BeginMenu("Help")) {
		if(ImGui::MenuItem("About")) {
			about_box.open(
				"A set of modding tools for the\n"
				"Ratchet & Clank PS2 games.\n"
				"\n"
				"Application version: " WRENCH_VERSION_STR "\n"
				"License: GPLv3+ (see LICENSE file)\n"
				"\n"
				"Contributors:\n"
				" - chaoticgd (original author)\n"
				" - clip / stiantoften\n"
				" - tsparkles / detolly\n"
				"\n"
				"Libraries used:\n"
				" - cxxopts: https://github.com/jarro2783/cxxopts (MIT)\n"
				" - glad: https://github.com/Dav1dde/glad (MIT)\n"
				" - glfw: https://github.com/glfw/glfw (zlib)\n"
				" - glm: https://github.com/g-truc/glm (Happy Bunny/MIT)\n"
				" - imgui: https://github.com/ocornut/imgui (MIT)\n"
				" - imgui_markdown: https://github.com/juliettef/imgui_markdown (zlib)\n"
				" - nlohmann json: https://github.com/nlohmann/json (MIT)\n"
				" - toml11: https://github.com/ToruNiina/toml11 (MIT)\n"
				" - ZipLib: https://bitbucket.org/wbenny/ziplib/wiki/Home (zlib)\n"
				" - MD5 implementation by Colin Plumb\n"
			);
		}
		if(ImGui::MenuItem("User Guide")) {
			open_document_viewer("user_guide.md");
		}
		ImGui::Separator();
		if(ImGui::MenuItem("GitHub")) {
			open_in_browser("https://github.com/chaoticgd/wrench");
		}
		if(ImGui::MenuItem("Check for Updates")) {
			open_in_browser("https://github.com/chaoticgd/wrench/releases");
		}
		if(ImGui::MenuItem("Report Bug")) {
			open_in_browser("https://github.com/chaoticgd/wrench/issues");
		}
		ImGui::EndMenu();
	}
	
	float menu_bar_height = ImGui::GetWindowSize().y;
	ImGui::EndMainMenuBar();
	return menu_bar_height;
}

void gui::render_tools(app& a, float menu_bar_height) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGuiViewport* view = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(-1, menu_bar_height - 1));
	ImGui::SetNextWindowSize(ImVec2(56, view->Size.y));
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
			(void*) (intptr_t) a.tools[i]->icon(), ImVec2(32, 32), 
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
	inspector
*/

const char* gui::inspector::title_text() const {
	return "Inspector";
}

ImVec2 gui::inspector::initial_size() const {
	return ImVec2(250, 250);
}

template <typename T_field, typename T_entity>
void inspector_input_scalar(wrench_project& proj, const char* label, T_field T_entity::*field);
template <typename T_lane, typename T_field, typename T_entity>
void inspector_input(wrench_project& proj, const char* label, T_field T_entity::*field, std::size_t first_lane, int lane_count);
struct inspector_text_lane {
	std::string str;
	bool changed = false;
};
void inspector_input_text_n(const char* label, inspector_text_lane* lanes, int lane_count);

void gui::inspector::render(app& a) {
	if(!a.get_level()) {
		ImGui::Text("<no level>");
		return;
	}
	
	wrench_project& proj = *a.get_project();
	level& lvl = *proj.selected_level();
	
	bool selection_empty = true;
	lvl.for_each<entity>([&](entity& ent) {
		if(ent.selected) {
			selection_empty = false;
		}
	});
	
	if(selection_empty) {
		ImGui::Text("<no entity selected>");
		return;
	}
	
	inspector_input<float>(proj, "Mat I ", &matrix_entity::local_to_world, 0, 4);
	inspector_input<float>(proj, "Mat J ", &matrix_entity::local_to_world, 4, 4);
	inspector_input<float>(proj, "Mat K ", &matrix_entity::local_to_world, 8, 4);
	inspector_input<float>(proj, "Mat T ", &matrix_entity::local_to_world, 12, 4);
	inspector_input<float>(proj, "Pos   ", &euler_entity::position, 0, 3);
	inspector_input<float>(proj, "Rot   ", &euler_entity::rotation, 0, 3);
	inspector_input_scalar(proj, "Unk 0 ", &tie_entity::unknown_0);
	inspector_input_scalar(proj, "Unk 4 ", &tie_entity::unknown_4);
	inspector_input_scalar(proj, "Unk 8 ", &tie_entity::unknown_8);
	inspector_input_scalar(proj, "Unk c ", &tie_entity::unknown_c);
	inspector_input_scalar(proj, "Unk 50", &tie_entity::unknown_50);
	inspector_input_scalar(proj, "UID   ", &tie_entity::uid);
	inspector_input_scalar(proj, "Unk 58", &tie_entity::unknown_58);
	inspector_input_scalar(proj, "Unk 5c", &tie_entity::unknown_5c);
	inspector_input_scalar(proj, "Unk 0 ", &shrub_entity::unknown_0);
	inspector_input_scalar(proj, "Unk 4 ", &shrub_entity::unknown_4);
	inspector_input_scalar(proj, "Unk 8 ", &shrub_entity::unknown_8);
	inspector_input_scalar(proj, "Unk c ", &shrub_entity::unknown_c);
	inspector_input_scalar(proj, "Unk 50", &shrub_entity::unknown_50);
	inspector_input_scalar(proj, "Unk 54", &shrub_entity::unknown_54);
	inspector_input_scalar(proj, "Unk 58", &shrub_entity::unknown_58);
	inspector_input_scalar(proj, "Unk 5c", &shrub_entity::unknown_5c);
	inspector_input_scalar(proj, "Unk 60", &shrub_entity::unknown_60);
	inspector_input_scalar(proj, "Unk 64", &shrub_entity::unknown_64);
	inspector_input_scalar(proj, "Unk 68", &shrub_entity::unknown_68);
	inspector_input_scalar(proj, "Unk 6c", &shrub_entity::unknown_6c);
	inspector_input_scalar(proj, "Size  ", &moby_entity::size);
	inspector_input_scalar(proj, "Unk 4 ", &moby_entity::unknown_4);
	inspector_input_scalar(proj, "Unk 8 ", &moby_entity::unknown_8);
	inspector_input_scalar(proj, "Unk c ", &moby_entity::unknown_c);
	inspector_input_scalar(proj, "UID   ", &moby_entity::uid);
	inspector_input_scalar(proj, "Unk 14", &moby_entity::unknown_14);
	inspector_input_scalar(proj, "Unk 18", &moby_entity::unknown_18);
	inspector_input_scalar(proj, "Unk 1c", &moby_entity::unknown_1c);
	inspector_input_scalar(proj, "Unk 20", &moby_entity::unknown_20);
	inspector_input_scalar(proj, "Unk 24", &moby_entity::unknown_24);
	inspector_input_scalar(proj, "Class ", &moby_entity::class_num);
	inspector_input_scalar(proj, "Scale ", &moby_entity::scale);
	inspector_input_scalar(proj, "Unk 30", &moby_entity::unknown_30);
	inspector_input_scalar(proj, "Unk 34", &moby_entity::unknown_34);
	inspector_input_scalar(proj, "Unk 38", &moby_entity::unknown_38);
	inspector_input_scalar(proj, "Unk 3c", &moby_entity::unknown_3c);
	inspector_input_scalar(proj, "Unk 58", &moby_entity::unknown_58);
	inspector_input_scalar(proj, "Unk 5c", &moby_entity::unknown_5c);
	inspector_input_scalar(proj, "Unk 60", &moby_entity::unknown_60);
	inspector_input_scalar(proj, "Unk 64", &moby_entity::unknown_64);
	inspector_input_scalar(proj, "Pvar #", &moby_entity::pvar_index);
	inspector_input_scalar(proj, "Unk 6c", &moby_entity::unknown_6c);
	inspector_input_scalar(proj, "Unk 70", &moby_entity::unknown_70);
	inspector_input_scalar(proj, "Unk 74", &moby_entity::unknown_74);
	inspector_input_scalar(proj, "Unk 78", &moby_entity::unknown_78);
	inspector_input_scalar(proj, "Unk 7c", &moby_entity::unknown_7c);
	inspector_input_scalar(proj, "Unk 80", &moby_entity::unknown_80);
	inspector_input_scalar(proj, "Unk 84", &moby_entity::unknown_84);
	inspector_input<float>(proj, "Point ", &grindrail_spline_entity::special_point, 0, 4);
	
	// If mobies with different class numbers are selected, or entities other
	// than mobies are selected, we shouldn't draw the pvars.
	std::optional<uint32_t> last_class;
	std::optional<int32_t> last_pvar_index;
	bool should_draw_pvars = true;
	lvl.for_each<entity>([&](entity& base_ent) {
		if(base_ent.selected) {
			if(moby_entity* ent = dynamic_cast<moby_entity*>(&base_ent)) {
				if(last_class && *last_class != ent->class_num) {
					should_draw_pvars = false;
				} else {
					last_class = ent->class_num;
					if(ent->pvar_index > -1) {
						last_pvar_index = ent->pvar_index;
					}
				}
			} else {
				should_draw_pvars = false;
			}
		}
	});
	
	if(should_draw_pvars && last_pvar_index) {
		ImGui::Text("Pvar %d", *last_pvar_index);
		
		auto& first_pvar = lvl.world.pvars.at(*last_pvar_index);
		for(std::size_t i = 0; i < first_pvar.size(); i++) {
			bool should_be_blank = false;
			lvl.for_each<moby_entity>([&](moby_entity& ent) {
				if(ent.selected && ent.pvar_index > -1) {
					auto& pvar = lvl.world.pvars.at(ent.pvar_index);
					if(pvar.at(i) != first_pvar[i]) {
						should_be_blank = true;
					}
				}
			});
			if(should_be_blank) {
				ImGui::Text("  ");
			} else {
				uint8_t value = first_pvar[i];
				ImGui::Text("%02x", value);
			}
			if(i % 16 != 15) {
				ImGui::SameLine();
			}
		}
	}
}

template <typename T_field, typename T_entity>
void inspector_input_scalar(wrench_project& proj, const char* label, T_field T_entity::*field) {
	inspector_input<T_field>(proj, label, field, 0, 1);
}

template <typename T_lane, typename T_field, typename T_entity>
void inspector_input(wrench_project& proj, const char* label, T_field T_entity::*field, std::size_t first_lane, int lane_count) {
	static const int MAX_LANES = 4;
	assert(lane_count <= MAX_LANES);
	level* lvl = proj.selected_level();
	
	// Determine whether all the values from a given lane are the same for all
	// selected entities.
	std::optional<T_lane> last_value[MAX_LANES];
	bool values_equal[MAX_LANES] = { true, true, true, true };
	bool selection_contains_entity_without_field = false;
	lvl->for_each<entity>([&](entity& base_ent) {
		if(base_ent.selected) {
			if(T_entity* ent = dynamic_cast<T_entity*>(&base_ent)) {
				for(int i = 0; i < lane_count; i++) {
					T_lane* value = ((T_lane*) &(*ent.*field)) + first_lane + i;
					if(last_value[i] && *value != last_value[i]) {
						values_equal[i] = false;
					}
					last_value[i] = *value;
				}
			} else {
				selection_contains_entity_without_field = true;
			}
		}
	});
	
	if(!last_value[0]) {
		// None of the selected entities contain the given field, so we
		// shouldn't draw it.
		return;
	}
	
	if(selection_contains_entity_without_field) {
		// We only want to draw an input box if ALL the selected entities have
		// the corresponding field.
		return;
	}
	
	inspector_text_lane input_lanes[MAX_LANES];
	for(int i = 0; i < lane_count; i++) {
		if(values_equal[i]) {
			input_lanes[i].str = std::to_string(*last_value[i]);
		}
	}
	
	inspector_input_text_n(label, input_lanes, lane_count);
	
	bool any_lane_changed = false;
	for(int i = 0; i < lane_count; i++) {
		any_lane_changed |= input_lanes[i].changed;
	}
	
	if(any_lane_changed) {
		level_proxy lvlp(&proj);
		std::vector<entity_id> ids = lvl->selected_entity_ids();
		std::map<entity_id, T_field> old_values;
		lvl->for_each<T_entity>([&](T_entity& ent) {
			if(ent.selected) {
				old_values[ent.id] = ent.*field;
			}
		});
		T_lane new_values[MAX_LANES];
		for(int i = 0; i < MAX_LANES; i++) {
			if(input_lanes[i].changed) {
				try {
					if constexpr(std::is_floating_point_v<T_lane>) {
						new_values[i] = std::stof(input_lanes[i].str);
					} else {
						new_values[i] = std::stoi(input_lanes[i].str);
					}
				} catch(std::logic_error&) {
					// The user has entered an invalid string.
					return;
				}
			}
		}
		
		proj.push_command(
			[lvlp, ids, field, first_lane, input_lanes, new_values]() {
				lvlp.get().for_each<T_entity>([&](T_entity& ent) {
					if(contains(ids, ent.id)) {
						for(int i = 0; i < MAX_LANES; i++) {
							T_lane* value = ((T_lane*) &(ent.*field)) + first_lane + i;
							if(input_lanes[i].changed && input_lanes[i].str != std::to_string(*value)) {
								*value = new_values[i];
							}
						}
					}
				});
			},
			[lvlp, ids, field, old_values]() {
				lvlp.get().for_each<T_entity>([&](T_entity& ent) {
					if(contains(ids, ent.id)) {
						ent.*field = old_values.at(ent.id);
					}
				});
			});
	}
}

void inspector_input_text_n(const char* label, inspector_text_lane* lanes, int lane_count) {
	ImGui::PushID(label);
	
	ImGui::AlignTextToFramePadding();
	ImGui::Text("%s", label);
	ImGui::SameLine();
	
	ImGui::PushMultiItemsWidths(lane_count, ImGui::GetWindowWidth() - lane_count * 16.f);
	for(int i = 0; i < lane_count; i++) {
		ImGui::PushID(i);
		if(i > 0) {
			ImGui::SameLine();
		}
		lanes[i].changed = ImGui::InputText("", &lanes[i].str, ImGuiInputTextFlags_EnterReturnsTrue);
		ImGui::PopID(); // i
		ImGui::PopItemWidth();
	}
	
	ImGui::PopID(); // label
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
	
	level& lvl = *a.get_level();
	
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 16;
	size.y -= 64;
	ImGui::Text("     UID                Class");
	ImGui::PushItemWidth(-1);
	if(ImGui::ListBoxHeader("##mobylist", size)) {
		for(moby_entity& moby : lvl.world.mobies) {
			std::stringstream row;
			row << std::setfill(' ') << std::setw(8) << std::dec << moby.uid << " ";
			row << std::setfill(' ') << std::setw(20) << std::hex << moby.class_num << " ";
			
			if(ImGui::Selectable(row.str().c_str(), moby.selected)) {
				lvl.clear_selection();
				moby.selected = true;
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
	glm::vec3 cam_pos = a.renderer.camera_position;
	ImGui::Text("Camera Position:\n\t%.3f, %.3f, %.3f",
		cam_pos.x, cam_pos.y, cam_pos.z);
	glm::vec2 cam_rot = a.renderer.camera_rotation;
	ImGui::Text("Camera Rotation:\n\tPitch=%.3f, Yaw=%.3f",
		cam_rot.x, cam_rot.y);
	ImGui::Text("Camera Control (Z to toggle):\n\t%s",
		a.renderer.camera_control ? "On" : "Off");
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
	if(auto lvl = a.get_level()) {
		static std::size_t language = 0;
		
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 64);

		static prompt_box string_exporter("Export", "Enter Export Path");
		if(auto path = string_exporter.prompt()) {
			auto strings = lvl->world.game_strings[language];
			std::ofstream out_file(*path);
			for(game_string& string : strings) {
				out_file << std::hex << string.id << ": " << string.str << "\n";
			}
		}

		ImGui::NextColumn();
		
		static const char* language_names[] = {
			"American English", "British English", "French", "German", "Spanish", "Italian", "Japanese", "Korean"
		};
		
		for(std::size_t i = 0; i < 8; i++) {
			if(ImGui::Button(language_names[i])) {
				language = i;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::Columns(1);

		auto& strings = lvl->world.game_strings[language];

		ImGui::BeginChild(1);
		for(game_string& string : strings) {
			ImGui::Text("%x: %s", string.id, string.str.c_str());
		}
		ImGui::EndChild();
	}
}

/*
	texture_browser
*/

gui::texture_browser::texture_browser() {}

const char* gui::texture_browser::title_text() const {
	return "Texture Browser";
}

ImVec2 gui::texture_browser::initial_size() const {
	return ImVec2(800, 600);
}

void gui::texture_browser::render(app& a) {
	if(!a.get_project()) {
		ImGui::Text("<no project open>");
		return;
	}

	auto tex_lists = a.get_project()->texture_lists(&a);
	if(tex_lists.find(_list) == tex_lists.end()) {
		if(tex_lists.size() > 0) {
			_list = tex_lists.begin()->first;
		} else {
			ImGui::Text("<no texture lists>");
			return;
		}
	}

	std::vector<texture>& textures = *tex_lists.at(_list);
	if(_selection >= textures.size()) {
		_selection = 0;
	}

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 192);

	ImGui::BeginChild(1);
		if(ImGui::TreeNodeEx("Sources", ImGuiTreeNodeFlags_DefaultOpen)) {
			for(auto& tex_list : tex_lists) {
				auto str = tex_list.first.c_str();
				bool selected = _list == tex_list.first;
				if(ImGui::Selectable(str, selected)) {
					_list = tex_list.first;
				}
			}
			ImGui::TreePop();
		}
		ImGui::NewLine();

		if(ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Minimum Width:");
			ImGui::PushItemWidth(-1);
			ImGui::InputInt("##minwidth", &_filters.min_width);
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::NewLine();

		if(ImGui::TreeNodeEx("Details", ImGuiTreeNodeFlags_DefaultOpen)) {
			if(textures.size() > 0) {
				vec2i size = textures[_selection].size();
				ImGui::Text("Width:  %ld", size.x);
				ImGui::Text("Height: %ld", size.y);
			} else {
				ImGui::Text("<no texture selected>");
			}
			ImGui::TreePop();
		}
		ImGui::NewLine();
		
		static alert_box error_box("Error");
		error_box.render();
		
		if(ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
			if(textures.size() > 0) {
				try {
					texture* tex = &textures[_selection];
					
					static prompt_box importer("Replace Selected", "Enter Import Path");
					if(auto path = importer.prompt()) {
						file_stream bmp_file(*path);
						bmp_to_texture(tex, bmp_file);
						tex->upload_to_opengl();
					}
					
					static prompt_box exporter("Export Selected", "Enter Export Path");
					if(auto path = exporter.prompt()) {
						file_stream bmp_file(*path, std::ios::in | std::ios::out | std::ios::trunc);
						texture_to_bmp(bmp_file, tex);
					}
					
					static prompt_box mega_exporter("Export All", "Enter Export Path");
					if(auto path_str = mega_exporter.prompt()) {
						fs::path path(*path_str);
						if(!fs::exists(path)) {
							fs::create_directory(path);
						}
						for(texture& tex : textures) {
							fs::path bmp_file_path = path / (tex.pixel_data_path() + ".bmp");
							file_stream bmp_file(bmp_file_path.string(), std::ios::in | std::ios::out | std::ios::trunc);
							texture_to_bmp(bmp_file, &tex);
						}
					}
				} catch(stream_error& e) {
					error_box.open(e.what());
				}
			}
			ImGui::TreePop();
		}
	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild(2);
		ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / 128));
		render_grid(a, textures);
	ImGui::EndChild();
	ImGui::NextColumn();
}

void gui::texture_browser::render_grid(app& a, std::vector<texture>& tex_list) {
	int num_this_frame = 0;

	for(std::size_t i = 0; i < tex_list.size(); i++) {
		texture* tex = &tex_list[i];

		if(tex->size().x < _filters.min_width) {
			continue;
		}

		if(tex->opengl_id() == 0) {
			// Only load 10 textures per frame.
			if(num_this_frame >= 10) {
				ImGui::NextColumn();
				continue;
			}

			tex->upload_to_opengl();
			num_this_frame++;
		}

		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) tex->opengl_id(),
			ImVec2(128, 128),
			ImVec2(0, 0),
			ImVec2(1, 1),
			(_selection == i) ? 2 : 0,
			ImVec4(0, 0, 0, 1),
			ImVec4(1, 1, 1, 1)
		);
		if(clicked) {
			_selection = i;
		}

		std::string display_name =
			std::to_string(i) + " " + tex->name;
		ImGui::Text("%s", display_name.c_str());
		ImGui::NextColumn();
	}
}

/*
	model_browser
*/

gui::model_browser::model_browser() {}
	
const char* gui::model_browser::title_text() const {
	return "Model Browser";
}

ImVec2 gui::model_browser::initial_size() const {
	return ImVec2(400, 300);
}

void gui::model_browser::render(app& a) {
	if(!a.get_project()) {
		ImGui::Text("<no project open>");
		return;
	}
	
	ImGui::Columns(2);
	if(_fullscreen_preview) {
		ImGui::SetColumnWidth(0, 0);
	} else {
		ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 384);
	}
	
	_model_lists = a.get_project()->model_lists(&a);
	moby_model* model = render_selection_pane(a);
	if(model == nullptr) {
		return;
	}
	
	ImGui::NextColumn();
	
	if(ImGui::Button(_fullscreen_preview ? " > " : " < ")) {
		_fullscreen_preview = !_fullscreen_preview;
	}
	ImGui::SameLine();
	ImGui::SliderFloat("Zoom", &_view_params.zoom, 0.0, 1.0, "%.1f");
	
	ImVec2 preview_size;
	if(_fullscreen_preview) {
		auto win_size = ImGui::GetWindowSize();
		preview_size = { win_size.x, win_size.y - 300 };
	} else {
		preview_size = { 400, 300 };
	}
	
	// If the mouse is dragging, this will store the displacement from the
	// mouse position when the button was pressed down.
	glm::vec2 drag_delta(0, 0);
	
	ImGui::BeginChild("preview", preview_size);
	{
		static GLuint preview_texture = 0;
		ImGui::Image((void*) (intptr_t) preview_texture, preview_size);
		static bool is_dragging = false;
		bool image_hovered = ImGui::IsItemHovered();
		glm::vec2 pitch_yaw = _view_params.pitch_yaw;
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (image_hovered || is_dragging)) {
			drag_delta = get_drag_delta();
			is_dragging = true;
		}
		
		// Update zoom and rotation.
		if(image_hovered || is_dragging) {
			ImGuiIO& io = ImGui::GetIO();
			_view_params.zoom *= io.MouseWheel * a.delta_time * 0.0001 + 1;
			if(_view_params.zoom < 0.f) _view_params.zoom = 0.f;
			if(_view_params.zoom > 1.f) _view_params.zoom = 1.f;
			
			if(ImGui::IsMouseReleased(0)) {
				_view_params.pitch_yaw += get_drag_delta();
				is_dragging = false;
			}
		}
		
		view_params preview_params = _view_params;
		preview_params.pitch_yaw += drag_delta;
		render_preview(
			a,
			&preview_texture,
			*model,
			*_model_lists.at(_list).textures,
			a.renderer,
			preview_size,
			preview_params);
	}
	ImGui::EndChild();
	
	if(ImGui::BeginTabBar("tabs")) {
		if(ImGui::BeginTabItem("Details")) {
			std::string index = std::to_string(_model);
			ImGui::InputText("Index", &index, ImGuiInputTextFlags_ReadOnly);
			std::string res_path = model->resource_path();
			ImGui::InputText("Resource Path", &res_path, ImGuiInputTextFlags_ReadOnly);
			
			static const std::map<view_mode, const char*> modes = {
				{ view_mode::WIREFRAME, "Wireframe" },
				{ view_mode::TEXTURED_POLYGONS, "Textured Polygons" }
			};
			if(ImGui::BeginCombo("View Mode", modes.at(_view_params.mode))) {
				for(auto [mode, name] : modes) {
					if(ImGui::Selectable(name, _view_params.mode == mode)) {
						_view_params.mode = mode;
					}
				}
				ImGui::EndCombo();
			}
			
			ImGui::Checkbox("Show Vertex Indices", &_view_params.show_vertex_indices);
			
			static prompt_box importer("Import .ply");
			static alert_box import_error("Import Error");
			import_error.render();
			if(auto path = importer.prompt()) {
				try {
					model->import_ply(*path);
				} catch(stream_error& e) {
					import_error.open(e.what());
				}
			}
			
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Submodels")) {
			ImGui::BeginChild("submodels");
			render_submodel_list(*model);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("ST Coords")) {
			ImGui::BeginChild("stcoords");
			render_st_coords(*model, a.renderer.shaders);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("VIF Lists (Debug)")) {
			ImGui::BeginChild("vif_lists");
			try {
				render_dma_debug_info(*model);
			} catch(stream_error& e) {
				ImGui::Text("Error: Out of bounds read.");
			}
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

moby_model* gui::model_browser::render_selection_pane(app& a) {
	moby_model* result = nullptr;
	
	if(ImGui::BeginTabBar("lists")) {
		for(auto& list : _model_lists) {
			if(ImGui::BeginTabItem(list.first.c_str())) {
				result = render_selection_grid(a, list.first, list.second);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	return result;
}

moby_model* gui::model_browser::render_selection_grid(
		app& a,
		std::string list_name,
		model_list& list) {
	moby_model* result = nullptr;
	std::size_t num_this_frame = 0;
	
	ImGui::BeginChild(1);
	ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / 128));
	
	for(std::size_t i = 0; i < list.models->size(); i++) {
		moby_model& model = (*list.models)[i];

		if(model.thumbnail() == 0) {
			// Only load 10 textures per frame.
			if(num_this_frame >= 10) {
				ImGui::NextColumn();
				continue;
			}
			
			render_preview(
				a,
				&model.thumbnail(),
				model,
				*list.textures,
				a.renderer,
				ImVec2(128, 128),
				view_params {
					view_mode::TEXTURED_POLYGONS,           // view_mode
					list_name == "ARMOR.WAD" ? 0.8f : 0.5f, // zoom
					glm::vec2(0, glm::radians(90.f)),       // pitch_yaw
					false                                   // show_vertex_indices
				});
			num_this_frame++;
		}
		
		bool selected = _list == list_name && _model == i;
		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) model.thumbnail(),
			ImVec2(128, 128),
			ImVec2(0, 0),
			ImVec2(1, 1),
			selected,
			ImVec4(0, 0, 0, 1),
			ImVec4(1, 1, 1, 1)
		);
		ImGui::Text("%ld\n", i);
		
		if(clicked) {
			_list = list_name;
			_model = i;
			
			// Reset submodel visibility.
			for(moby_submodel& submodel : model.submodels) {
				submodel.visible_in_model_viewer = true;
			}
		}
		if(selected) {
			result = &model;
		}
		
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	
	return result;
}

void gui::model_browser::render_preview(
		app& a,
		GLuint* target,
		moby_model& model,
		std::vector<texture>& textures,
		const gl_renderer& renderer,
		ImVec2 preview_size,
		view_params params) {
	glm::vec3 eye = glm::vec3(2.f * (1.1f - params.zoom), 0, 0);
	
	glm::mat4 view_fixed = glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 view_pitched = glm::rotate(view_fixed, params.pitch_yaw.x, glm::vec3(0, 0, 1));
	glm::mat4 view = glm::rotate(view_pitched, params.pitch_yaw.y, glm::vec3(0, 1, 0));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), preview_size.x / preview_size.y, 0.01f, 100.0f);
	
	static const glm::mat4 yzx {
		0,  0, 1, 0,
		1,  0, 0, 0,
		0, -1, 0, 0,
		0,  0, 0, 1
	};
	glm::mat4 local_to_world = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -0.125f));
	glm::mat4 local_to_clip = projection * view * yzx * local_to_world;

	std::vector<GLuint> gl_textures;
	for(texture& tex : textures) {
		gl_textures.push_back(tex.opengl_id());
	}
	
	gl_buffer local_to_clip_buffer;
	glGenBuffers(1, &local_to_clip_buffer());
	glBindBuffer(GL_ARRAY_BUFFER, local_to_clip_buffer());
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::mat4),
		&local_to_clip, GL_STATIC_DRAW);

	render_to_texture(target, preview_size.x, preview_size.y, [&]() {
		renderer.draw_moby_models(model, textures, params.mode, false, local_to_clip_buffer(), 0, 1);
	});
	
	if(params.show_vertex_indices) {
		static const auto apply_local_to_screen = [&](glm::vec4 pos) {
			glm::vec4 homogeneous_pos = local_to_clip * pos;
			glm::vec3 gl_pos {
					homogeneous_pos.x / homogeneous_pos.w,
					homogeneous_pos.y / homogeneous_pos.w,
					homogeneous_pos.z / homogeneous_pos.w
			};
			ImVec2 window_pos = ImGui::GetWindowPos();
			glm::vec3 screen_pos(
					window_pos.x + (1 + gl_pos.x) * preview_size.x / 2.0,
					window_pos.y + (1 + gl_pos.y) * preview_size.y / 2.0,
					gl_pos.z
			);
			return screen_pos;
		};
		
		for(const moby_submodel& submodel : model.submodels) {
			if(!submodel.visible_in_model_viewer) {
				continue;
			}
			
			auto draw_list = ImGui::GetWindowDrawList();
			for(std::size_t j = 0; j < submodel.vertices.size(); j++) {
				const moby_model_vertex& vert = submodel.vertices[j];
				glm::vec3 proj_pos = apply_local_to_screen(glm::vec4(
					vert.x / (float) INT16_MAX,
					vert.y / (float) INT16_MAX,
					vert.z / (float) INT16_MAX, 1.f));
				if(proj_pos.z > 0.f) {
					draw_list->AddText(ImVec2(proj_pos.x, proj_pos.y), 0xffffffff, int_to_hex(j).c_str());
				}
			}
		}
	}
	
}

glm::vec2 gui::model_browser::get_drag_delta() const {
	auto delta = ImGui::GetMouseDragDelta();
	return glm::vec2(delta.y, delta.x) * 0.01f;
}

void gui::model_browser::render_submodel_list(moby_model& model) {
	// We're only reading in the main submodels for now, but there seem to
	// be more in some of the armour models.
	static const std::size_t SUBMODEL_GROUPS = 1;
	
	std::size_t low = 0;
	for(std::size_t i = 0; i < SUBMODEL_GROUPS; i++) {
		ImGui::PushID(i);
		
		const std::size_t high = model.submodels.size();
		
		// If every submodel in a given group is visible, we should draw the
		// box as being ticked.
		bool group_ticked = true;
		for(std::size_t j = low; j < high; j++) {
			group_ticked &= model.submodels[j].visible_in_model_viewer;
		}
		const bool group_ticked_before = group_ticked;
		
		std::string label = "Group " + std::to_string(i);
		
		bool group_expanded = ImGui::TreeNode("group", "%s", "");
		ImGui::SameLine();
		ImGui::Checkbox(label.c_str(), &group_ticked);
		if(group_expanded) {
			for(std::size_t j = low; j < high; j++) {
				ImGui::PushID(j);
				moby_submodel& submodel = model.submodels[j];
				
				std::string submodel_label = "Submodel " + std::to_string(j);
				bool submodel_expanded = ImGui::TreeNode("submodel", "%s", "");
				ImGui::SameLine();
				ImGui::Checkbox(submodel_label.c_str(), &submodel.visible_in_model_viewer);
				if(submodel_expanded) {
					for(const moby_model_vertex& vertex : submodel.vertices) {
						ImGui::Text("%x %x %x", vertex.x & 0xffff, vertex.y & 0xffff, vertex.z & 0xffff);
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		
		// If the user user ticked or unticked the box, apply said changes to
		// all submodels in the current group.
		if(group_ticked != group_ticked_before) {
			for(std::size_t j = low; j < high; j++) {
				model.submodels[j].visible_in_model_viewer = group_ticked;
			}
		}
		
		ImGui::PopID();
	}
}

void gui::model_browser::render_st_coords(moby_model& model, const shader_programs& shaders) {
	if(model.submodels.size() < 1) {
		return;
	}
	
	std::set<int32_t> texture_indices;
	//for(moby_submodel& submodel : model.submodels) {
	//	if(submodel.texture) {
	//		texture_indices.insert(submodel.texture->texture_index);
	//	}
	//}
	
	static int32_t texture_index = 0;
	if(ImGui::BeginTabBar("tabs")) {
		for(int32_t index : texture_indices) {
			std::string tab_name = std::to_string(index);
			if(ImGui::BeginTabItem(tab_name.c_str())) {
				texture_index = index;
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	static GLuint texture = 0;
	static const ImVec2 size(256, 256);
	
	int32_t current_texture_index = 0;
	render_to_texture(&texture, size.x, size.y, [&]() {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUseProgram(shaders.solid_colour.id());
		
		for(std::size_t i = 0; i < model.submodels.size(); i++) {
			//moby_submodel& submodel = model.submodels[i];
			//
			//if(submodel.texture) {
			//	current_texture_index = submodel.texture->texture_index;
			//}
			//
			//if(!submodel.visible_in_model_viewer || current_texture_index != texture_index) {
			//	continue;
			//}
			//
			//if(submodel.st_buffer == 0) {
			//	std::vector<float> st_data;
			//	for(const moby_model_st& st : submodel.st_data) {
			//		st_data.push_back((st.s / (float) INT16_MAX) * 8.f);
			//		st_data.push_back((st.t / (float) INT16_MAX) * 8.f);
			//		st_data.push_back(0.f);
			//	}
			//	
			//	glGenBuffers(1, &submodel.st_buffer());
			//	glBindBuffer(GL_ARRAY_BUFFER, submodel.st_buffer());
			//	glBufferData(GL_ARRAY_BUFFER,
			//		st_data.size() * sizeof(float),
			//		st_data.data(), GL_STATIC_DRAW);
			//}
			//
			//glm::vec4 colour = colour_coded_submodel_index(i, model.submodels.size());
			//static const glm::mat4 id(1.f);
			//glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &id[0][0]);
			//glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);
			//
			//glEnableVertexAttribArray(0);
			//glBindBuffer(GL_ARRAY_BUFFER, submodel.st_buffer());
			//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			//
			//glDrawArrays(GL_POINTS, 0, submodel.st_coords.size());
			//
			//glDisableVertexAttribArray(0);
		}
	});
	
	ImGui::Image((void*) (intptr_t) texture, size);
}

void gui::model_browser::render_dma_debug_info(moby_model& mdl) {
	for(std::size_t i = 0; i < mdl.submodels.size(); i++) {
		ImGui::PushID(i);
		moby_submodel& submodel = mdl.submodels[i];
		
		if(ImGui::TreeNode("submodel", "Submodel %ld", i)) {
			for(vif_packet& vpkt : submodel.vif_list) {
				ImGui::PushID(vpkt.address);
					
				if(vpkt.error != "") {
					ImGui::Text("   (error: %s)", vpkt.error.c_str());
					ImGui::PopID();
					continue;
				}
				
				std::string label = vpkt.code.to_string();
				if(ImGui::TreeNode("packet", "%lx %s", vpkt.address, label.c_str())) {
					auto lines = to_hex_dump((uint32_t*) vpkt.data.data(), vpkt.address, vpkt.data.size() / sizeof(uint32_t));
					for(std::string& line : lines) {
						ImGui::Text("    %s", line.c_str());
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

/*
	settings
*/

const char* gui::settings::title_text() const {
	return "Settings";
}

ImVec2 gui::settings::initial_size() const {
	return ImVec2(300, 200);
}

void gui::settings::render(app& a) {
	if(ImGui::BeginTabBar("tabs")) {
		if(ImGui::BeginTabItem("General")) {
			render_general_page(a);
			ImGui::EndTabItem();
		}
		
		if(ImGui::BeginTabItem("GUI")) {
			render_gui_page(a);
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Debug")) {
			render_debug_page(a);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	ImGui::NewLine();
	if(ImGui::Button("Close")) {
		close(a);
	}
}

void gui::settings::render_general_page(app& a) {
	ImGui::Text("Emulator Path");

	ImGui::PushItemWidth(-1);
	if(ImGui::InputText("##emulator_path", &config::get().emulator_path)) {
		config::get().write();
	}
	ImGui::PopItemWidth();
	ImGui::NewLine();

	ImGui::Text("Game Paths");

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 32);

	for(auto iter = config::get().game_isos.begin(); iter < config::get().game_isos.end(); iter++) {
		ImGui::PushID(std::distance(config::get().game_isos.begin(), iter));
		std::stringstream label;
		label << iter->game_db_entry << " " << iter->md5;
		ImGui::InputText(label.str().c_str(), &iter->path, ImGuiInputTextFlags_ReadOnly);
		ImGui::NextColumn();
		if(ImGui::Button("X")) {
			config::get().game_isos.erase(iter);
			config::get().write();
			ImGui::PopID();
			break;
		}
		ImGui::NextColumn();
		ImGui::PopID();
	}
	
	ImGui::Columns();
	ImGui::NewLine();
	
	if(a.game_db.size() < 1) {
		return;
	}
	
	ImGui::Text("Add Game");
	ImGui::InputText("ISO Path", &_new_game_path);
	if(ImGui::BeginCombo("##new_game_type", a.game_db[_new_game_type].name.c_str())) {
		for(std::size_t i = 0; i < a.game_db.size(); i++) {
			if(ImGui::Selectable(a.game_db[i].name.c_str())) {
				_new_game_type = i;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if(ImGui::Button("Add Game")) {
		game_iso game;
		game.path = _new_game_path;
		game.game_db_entry = a.game_db[_new_game_type].name;
		
		// Generate an MD5 hash on a different thread.
		a.emplace_window<worker_thread<game_iso, game_iso>>(
			"Adding Game", game,
			[](game_iso game, worker_logger& log) {
				try {
					file_stream iso(game.path);
					log << "Generating MD5 hash... ";
					game.md5 = md5_from_stream(iso);
					log << "done!";
					return std::optional<game_iso>(game);
				} catch(stream_error& e) {
					log << "Error: " << e.what();
				}
				return std::optional<game_iso>();
			},
			[&](game_iso game) {
				config::get().game_isos.push_back(game);
				config::get().write();
			}
		);
		
		_new_game_path = "";
	}
	ImGui::NewLine();
	
	ImGui::Text("Compression Threads");
	int compression_threads = config::get().compression_threads;
	if(ImGui::InputInt("##compression_threads", &compression_threads)) {
		if(compression_threads >= 1 && compression_threads <= 256) {
			config::get().compression_threads = compression_threads;
			config::get().write();
		}
	}
}

void gui::settings::render_gui_page(app& a) {
	if(ImGui::SliderFloat("GUI Scale", &config::get().gui_scale, 0.5, 2, "%.1f")) {
		a.update_gui_scale();
		config::get().write();
	}
	
	if(ImGui::Checkbox("Vsync", &config::get().vsync)) {
		glfwSwapInterval(config::get().vsync ? 1 : 0);
		config::get().write();
	}
}

void gui::settings::render_debug_page(app& a) {
	if(ImGui::Checkbox("Stream Tracing", &config::get().debug.stream_tracing)) {
		config::get().write();
	}
}

/*
	manual_patcher
*/

gui::manual_patcher::manual_patcher()
	: _scroll_offset(0) {}

const char* gui::manual_patcher::title_text() const {
	return "Manual Patcher";
}

ImVec2 gui::manual_patcher::initial_size() const {
	return ImVec2(800, 600);
}

void gui::manual_patcher::render(app& a) {
	auto* project = a.get_project();
	if(project == nullptr) {
		return;
	}
	
	ImGui::Text("Goto:");
	ImGui::SameLine();
	if(ImGui::InputText("##hex_goto", &_scroll_offset_str)) {
		_scroll_offset = parse_number(_scroll_offset_str);
	}
	
	if(_scroll_offset + 1 >= project->iso.size()) {
		ImGui::Text("<end of file>");
		return;
	}
	
	static const int row_size = 16;
	static const int num_rows = 16;
	
	std::vector<char> buffer(row_size * num_rows);

	std::size_t size_to_read = buffer.size();
	
	if(_scroll_offset >= project->iso.size() - row_size * num_rows) {
		size_to_read = project->iso.size() - _scroll_offset - 1;
	}
	
	if(_scroll_offset < project->iso.size()) {
		project->iso.seek(_scroll_offset);
	}
	project->iso.read_n(buffer.data(), size_to_read);
	
	// If we're viewing past the end of the file, display zeroes.
	for(std::size_t i = size_to_read; i < buffer.size(); i++) {
		buffer[i] = 0;
	}
	
	ImGui::BeginChild(1);
	for(std::size_t row = 0; row < num_rows; row++) {
		ImGui::Text("%010lx: ", _scroll_offset + row * row_size);
		ImGui::SameLine();
		for(std::size_t column = 0; column < row_size; column++) {
			if(column % 4 == 0) {
				ImGui::Text(" ");
				ImGui::SameLine();
			}
			std::size_t offset = row * row_size + column;
			char byte = buffer[offset];
			auto display_hex = int_to_hex(byte & 0xff);
			while(display_hex.size() < 2) display_hex = "0" + display_hex;
			std::string label = std::string("##") + std::to_string(offset);
			ImGui::SetNextItemWidth(20);
			if(ImGui::InputText(label.c_str(), &display_hex, ImGuiInputTextFlags_EnterReturnsTrue)) {
				char new_byte = hex_to_int(display_hex);
				if(byte != new_byte) {
					project->iso.write<char>(_scroll_offset + offset, new_byte);
				}
			}
			ImGui::SameLine();
		}
		for(std::size_t column = 0; column < row_size; column++) {
			std::size_t offset = row * row_size + column;
			char byte = buffer[offset];
			ImGui::Text("%c", byte);
			ImGui::SameLine();
		}
		ImGui::NewLine();
	}
	ImGui::EndChild();
	
}

/*
	document_viewer
*/

gui::document_viewer::document_viewer(const char* path) {
	_config.linkCallback = [](ImGui::MarkdownLinkCallbackData link) {
		document_viewer* window = static_cast<document_viewer*>(link.userData);
		std::string path(link.link, link.linkLength);
		window->load_page(path);
	};
	_config.userData = this;
	load_page(path);
}
	
const char* gui::document_viewer::title_text() const {
	return "Documentation";
}

ImVec2 gui::document_viewer::initial_size() const {
	return ImVec2(400, 300);
}

void gui::document_viewer::render(app& a) {
	ImGui::Markdown(_body.c_str(), _body.size(), _config);
}

void gui::document_viewer::load_page(std::string path) {
	try {
		file_stream file(std::string("docs/") + path);
		_body.resize(file.size());
		file.read_n(_body.data(), file.size());
	} catch(stream_error&) {
		_body = "Cannot open file.";
		return;
	}
}

/*
	stream_viewer
*/

gui::stream_viewer::stream_viewer() {}
		
const char* gui::stream_viewer::title_text() const {
	return "Stream Viewer";
}

ImVec2 gui::stream_viewer::initial_size() const {
	return ImVec2(800, 600);
}

void gui::stream_viewer::render(app& a) {
	wrench_project* project = a.get_project();
	if(project == nullptr) {
		ImGui::Text("<no project open>");
		return;
	}
	
	static alert_box error_box("Error");
	error_box.render();
	
	static prompt_box exporter("Export", "Enter Export Path");
	if(auto path = exporter.prompt()) {
		stream* selection = _selection;
		
		// The stream might not exist anymore, so we need to make sure that
		// it's still in the tree before dereferencing it.
		if(!project->iso.contains(selection)) {
			error_box.open("The selected stream no longer exists.");
			return;
		}
		
		if(selection->size() == 0) {
			error_box.open("The selected stream has an unknown size so cannot be exported.");
			return;
		}
			
		// Write out the stream to the specified file.
		try {
			file_stream out_file(*path, std::ios::out);
			selection->seek(0);
			stream::copy_n(out_file, *selection, selection->size());
		} catch(stream_error& err) {
			error_box.open(err.what());
		}
	}
	
	trace_stream* trace = dynamic_cast<trace_stream*>(_selection);
	if(trace != nullptr) {
		ImGui::SameLine();
		if(ImGui::Button("Export Trace")) {
			export_trace(trace);
		}
	}
		
	ImGui::BeginChild(1);
		ImGui::Columns(3);
		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Path");
		ImGui::NextColumn();
		ImGui::Text("Size");
		ImGui::NextColumn();
		for(int i = 0; i < 3; i++) {
			ImGui::NewLine();
			ImGui::NextColumn();
		}
		render_stream_tree_node(&project->iso, 0);
		ImGui::Columns();
	ImGui::EndChild();
}

void gui::stream_viewer::render_stream_tree_node(stream* node, std::size_t index) {
	bool is_selected = _selection == node;
	
	std::stringstream text;
	text << index;
	text << " " << node->name;
	text << " (" << node->children.size() <<")";
	
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if(is_selected) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if(node->children.size() == 0) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	
	ImGui::PushID(reinterpret_cast<std::size_t>(node));
	bool expanded = ImGui::TreeNodeEx(text.str().c_str(), flags);
	ImGui::NextColumn();
	bool make_selection = false;
	make_selection |= ImGui::Selectable(node->resource_path().c_str(), is_selected);
	ImGui::NextColumn();
	make_selection |= ImGui::Selectable(int_to_hex(node->size()).c_str(), is_selected);
	ImGui::NextColumn();
	if(expanded) {
		// Display streams with children before leaf streams.
		for(std::size_t i = 0; i < node->children.size(); i++) {
			if(node->children[i]->children.size() != 0) {
				render_stream_tree_node(node->children[i], i);
			}
		}
		for(std::size_t i = 0; i < node->children.size(); i++) {
			if(node->children[i]->children.size() == 0) {
				render_stream_tree_node(node->children[i], i);
			}
		}
		ImGui::TreePop();
	}
	if(make_selection) {
		_selection = node;
	}
	ImGui::PopID();
}

void gui::stream_viewer::export_trace(trace_stream* node) {
	std::vector<uint8_t> buffer(node->size());
	node->seek(0);
	node->parent->read_v(buffer); // Avoid tarnishing the read_mask buffer.
	
	static const std::size_t image_side_length = 1024;
	static const std::size_t image_pixel_count = image_side_length * image_side_length;
	
	struct bgr32 {
		uint8_t	b, g, r, pad;
	};
	std::vector<bgr32> bgr_pixel_data(image_pixel_count);
	
	// Convert stream to pixel data.
	float scale_factor = buffer.size() / (float) image_pixel_count;
	
	for(std::size_t i = 0; i < image_pixel_count; i++) {
		std::size_t in_index = (std::size_t) (i * scale_factor);
		std::size_t in_index_end = (std::size_t) ((i + 1) * scale_factor);
		if(in_index_end >= buffer.size()) {
			bgr_pixel_data[i] = { 0, 0, 0, 0 };
			continue;
		}
		
		uint8_t pixel = buffer[in_index];
		bool read = false;
		for(std::size_t j = in_index; j < in_index_end; j++) {
			read |= node->read_mask[j];
		}
		bgr_pixel_data[i] = bgr32 {
			(uint8_t) (read ? 0 : pixel),
			(uint8_t) (read ? 0 : pixel),
			pixel,
			0
		};
	}
	
	// Write out a BMP file.
	file_stream bmp_file(node->resource_path() + "_trace.bmp", std::ios::out);
	
	bmp_file_header header;
	std::memcpy(header.magic, "BM", 2);
	header.pixel_data =
		sizeof(bmp_file_header) + sizeof(bmp_info_header);
	header.file_size =
		header.pixel_data.value + image_pixel_count * sizeof(uint32_t);
	header.reserved = 0x3713;
	bmp_file.write<bmp_file_header>(0, header);

	bmp_info_header info;
	info.info_header_size      = 40;
	info.width                 = image_side_length;
	info.height                = image_side_length;
	info.num_colour_planes     = 1;
	info.bits_per_pixel        = 32;
	info.compression_method    = 0;
	info.pixel_data_size       = image_pixel_count * sizeof(uint32_t);
	info.horizontal_resolution = 0;
	info.vertical_resolution   = 0;
	info.num_colours           = 256;
	info.num_important_colours = 0;
	bmp_file.write<bmp_info_header>(info);
	
	bmp_file.write_v(bgr_pixel_data);
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

/*
	file_dialog
*/

gui::file_dialog::file_dialog(const char* title, mode m, std::vector<std::string> extensions)
	: _title(title), _mode(m), _extensions(extensions), _directory_input("."), _directory(".") {}

const char* gui::file_dialog::title_text() const {
	return _title;
}

ImVec2 gui::file_dialog::initial_size() const {
	return ImVec2(300, 200);
}

void gui::file_dialog::render(app& a) {

	// Draw file path input.
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 64);
	ImGui::Text("File: ");
	ImGui::NextColumn();
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if(ImGui::InputText("##file", &_file, ImGuiInputTextFlags_EnterReturnsTrue)) {
		_callback(_file);
		close(a);
		return;
	}
	ImGui::PopItemWidth();
	ImGui::NextColumn();
	if(ImGui::Button("Select")) {
		_callback(_file);
		close(a);
		return;
	}
	ImGui::NextColumn();
	
	// Draw current directory input.
	ImGui::Text("Dir: ");
	ImGui::NextColumn();
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);
	if(ImGui::InputText("##directory_input", &_directory_input, ImGuiInputTextFlags_EnterReturnsTrue)) {
		_directory = _directory_input;
		_directory_input = _directory.string();
	}
	ImGui::PopItemWidth();
	ImGui::NextColumn();
	if(ImGui::Button("Cancel")) {
		close(a);
		return;
	}
	ImGui::Columns(1);

	// Draw directory listing.
	if(fs::is_directory(_directory)) {
		std::vector<fs::path> items { _directory / ".." };
		for(auto item : fs::directory_iterator(_directory)) {
			items.push_back(item.path());
		}

		ImGui::PushItemWidth(-1);
		ImGui::BeginChild(1);
		for(auto item : items) {
			if(!fs::is_directory(item)) {
				continue;
			}
			
			std::string name = std::string("Dir ") + item.filename().string();
			if(ImGui::Selectable(name.c_str(), false)) {
				_directory = fs::canonical(item);
				_directory_input = _directory.string();
			}

			ImGui::NextColumn();
		}
		for(auto item : items) {
			if(fs::is_directory(item)) {
				continue;
			}
			if(std::find(_extensions.begin(), _extensions.end(), item.extension()) == _extensions.end()) {
				continue;
			}

			std::string name = std::string("	") + item.filename().string();
			if(ImGui::Selectable(name.c_str(), false)) {
				_file = item.string();
			}

			ImGui::NextColumn();
		}
		ImGui::EndChild();
		ImGui::PopItemWidth();
	} else {
		ImGui::PushItemWidth(-1);
		ImGui::Text("Not a directory.");
		ImGui::PopItemWidth();
	}
}

bool gui::file_dialog::is_unique() const {
	return false;
}

void gui::file_dialog::on_okay(std::function<void(std::string)> callback) {
	_callback = callback;
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

// Don't pass untrusted input to this!
void gui::open_in_browser(const char* url) {
	std::string cmd = "xdg-open " + std::string(url);
	int result = system(cmd.c_str());
	if(result != 0) {
		fprintf(stderr, "error: Failed to execute shell command.\n");
	}
}
