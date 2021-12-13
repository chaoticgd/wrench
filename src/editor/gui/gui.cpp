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

#include "../icons.h"
#include "../util.h"
#include "../config.h"
#include "../renderer.h"
#include "../unwindows.h"
#include "../worker_thread.h"
#include "window.h"

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

	ImGuiID left_centre, right;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 8.f / 10.f, &left_centre, &right);
	
	ImGuiID left, centre;
	ImGui::DockBuilderSplitNode(left_centre, ImGuiDir_Left, 2.f / 10.f, &left, &centre);
	
	ImGuiID inspector, middle_right;
	ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 1.f / 2.f, &inspector, &middle_right);
	
	ImGuiID mobies, viewport_info;
	ImGui::DockBuilderSplitNode(middle_right, ImGuiDir_Up, 1.f / 2.f, &mobies, &viewport_info);
	
	ImGui::DockBuilderDockWindow("Project Tree", left);
	ImGui::DockBuilderDockWindow("Start Screen", centre);
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
	pos.x += 55 * config::get().gui_scale;
	size.x -= 55 * config::get().gui_scale;
	
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
	
	enum class file_dialog_type {
		OPEN, SAVE, DIR
	};
	
	auto input_path = [](const char* label, std::string* dest, file_dialog_type type) {
		ImGui::PushID(label);
		if(strlen(label) > 0) {
			ImGui::Text("%s", label);
			ImGui::SameLine();
		}
		ImGui::InputText("##input", dest);
		ImGui::SameLine();
		if(ImGui::Button("Browse")) {
			nfdresult_t result;
			nfdchar_t* path;
			switch(type) {
				case file_dialog_type::OPEN: result = NFD_OpenDialog("iso", nullptr, &path); break;
				case file_dialog_type::SAVE: result = NFD_SaveDialog("iso", nullptr, &path); break;
				case file_dialog_type::DIR: result = NFD_PickFolder(nullptr, &path); break;
			}
			if(result == NFD_OKAY) {
				*dest = std::string(path);
				free(path);
			}
		}
		ImGui::PopID();
	};
	
	ImGui::BeginMainMenuBar();
	if(ImGui::BeginMenu("File")) {
		if(ImGui::BeginMenu("Extract ISO")) {
			static std::string input_iso;
			static std::string output_dir;
			input_path("Input ISO       ", &input_iso, file_dialog_type::OPEN);
			input_path("Output Directory", &output_dir, file_dialog_type::DIR);
			if(ImGui::Button("Extract")) {
				a.extract_iso(input_iso, output_dir);
				input_iso = "";
				output_dir = "";
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Open Directory")) {
			static std::string dir;
			input_path("", &dir, file_dialog_type::DIR);
			if(ImGui::Button("Open")) {
				a.open_directory(dir);
				dir = "";
			}
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("Build ISO")) {
			static bool build_from_custom_dir = false;
			static std::string custom_input_dir;
			ImGui::Checkbox("Custom Input Directory", &build_from_custom_dir);
			if(build_from_custom_dir) {
				input_path("Input Directory", &custom_input_dir, file_dialog_type::DIR);
			}
			
			static bool build_to_custom_path = false;
			static std::string custom_output_iso;
			ImGui::Checkbox("Custom Output Path", &build_to_custom_path);
			if(build_to_custom_path) {
				input_path("Output ISO     ", &custom_output_iso, file_dialog_type::SAVE);
			}
			
			static bool launch_emulator = false;
			ImGui::Checkbox("Launch emulator after building", &launch_emulator);
			
			static bool single_level = false;
			static int single_level_index = 0;
			ImGui::Checkbox("Only write out single level (much faster)", &single_level);
			if(single_level) {
				ImGui::InputInt("Single Level Index", &single_level_index);
			}
			
			static bool no_mpegs = false;
			ImGui::Checkbox("Skip writing out MPEG cutscenes (much faster)", &no_mpegs);
			
			static bool save_current_level = true;
			ImGui::Checkbox("Save and build currently open level", &save_current_level);
			
			if(((!build_from_custom_dir || !build_to_custom_path) && a.directory.empty())) {
				ImGui::TextWrapped("No directory open!\n");
			} else if(ImGui::Button("Build")) {
				build_settings settings;
				if(build_from_custom_dir) {
					settings.input_dir = custom_input_dir;
				} else {
					settings.input_dir = a.directory/"built";
				}
				
				std::string output_iso;
				if(build_to_custom_path) {
					settings.output_iso = custom_output_iso;
				} else {
					settings.output_iso = a.directory / "build.iso";
				}
				
				settings.launch_emulator = launch_emulator;
				settings.single_level = single_level;
				settings.single_level_index = single_level_index;
				settings.no_mpegs = no_mpegs;
				
				if(save_current_level && a.get_level()) {
					a.save_level();
				}
				a.build_iso(settings);
			}
			ImGui::EndMenu();
		}
		if(ImGui::MenuItem("Save and Build Level", nullptr, nullptr, a.get_level())) {
			a.save_level();
		}
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
					lvl->undo();
				} catch(command_error& error) {
					undo_error_box.open(error.what());
				}
			}
			if(ImGui::MenuItem("Redo")) {
				try {
					lvl->redo();
				} catch(command_error& error) {
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
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
	
	static alert_box emu_error_box("Error");
	emu_error_box.render();
	
	if(ImGui::BeginMenu("Tree")) {
		render_tree_menu(a);
		ImGui::EndMenu();
	}
	
	if(ImGui::BeginMenu("Windows")) {
		render_menu_bar_window_toggle<start_screen>(a);
		render_menu_bar_window_toggle<view_3d>(a);
		render_menu_bar_window_toggle<moby_list>(a);
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<Inspector>(a);
		render_menu_bar_window_toggle<settings>(a);
		ImGui::EndMenu();
	}
	
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
				" - CreepNT\n"
				" - Dnawrkshp\n"
				" - Pritchy96\n"
				" - tsparkles / detolly\n"
				"\n"
				"Libraries used:\n"
				" - cxxopts: https://github.com/jarro2783/cxxopts (MIT)\n"
				" - glad: https://github.com/Dav1dde/glad (MIT)\n"
				" - glfw: https://github.com/glfw/glfw (zlib)\n"
				" - glm: https://github.com/g-truc/glm (Happy Bunny/MIT)\n"
				" - imgui: https://github.com/ocornut/imgui (MIT)\n"
				" - nativefiledialog: https://github.com/mlabbe/nativefiledialog (zlib)\n"
				" - nlohmann json: https://github.com/nlohmann/json (MIT)\n"
				" - toml11: https://github.com/ToruNiina/toml11 (MIT)\n"
				" - MD5 implementation by Colin Plumb\n"
			);
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
	
	ImGui::SetNextWindowSize(ImVec2(56 * config::get().gui_scale, view->Size.y));
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
			(void*) (intptr_t) a.tools[i]->icon(), ImVec2(32*config::get().gui_scale, 32*config::get().gui_scale),
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

void gui::render_tree_menu(app& a) {
	struct project_tree_node {
		fs::path path;
		std::vector<project_tree_node> dirs;
		std::vector<fs::path> files;
	};
	
	std::function<void(project_tree_node&)> render_tree_node = [&](auto& node) {
		for(project_tree_node& subdir : node.dirs) {
			if(ImGui::BeginMenu(subdir.path.filename().string().c_str())) {
				render_tree_node(subdir);
				ImGui::EndMenu();
			}
		}
		if(node.dirs.size() > 0 && node.files.size() > 0) {
			ImGui::Separator();
		}
		for(fs::path& file : node.files) {
			if(ImGui::MenuItem(file.filename().string().c_str())) {
				a.open_file(file);
			}
		}
	};
	
	using reload_fun = std::function<void(int&, project_tree_node&, fs::path, int)>;
	reload_fun reload = [&](int& files, auto& dest, auto path, int depth) {
		dest.path = path;
		if(depth > 8) {
			fprintf(stderr, "warning: Directory depth exceeds 8!\n");
			return;
		}
		for(auto& file : fs::directory_iterator(path)) {
			if(file.is_directory()) {
				project_tree_node node;
				reload(files, node, file, depth + 1);
				dest.dirs.push_back(node);
			} else if(file.is_regular_file()) {
				if(files > 10000) {
					fprintf(stderr, "warning: More than 10000 files in directory!\n");
				}
				files++;
				dest.files.push_back(file);
			}
		}
		std::sort(dest.dirs.begin(), dest.dirs.end(),
			[](auto& l, auto& r) { return l.path < r.path; });
		std::sort(dest.files.begin(), dest.files.end());
	};
	
	static project_tree_node project_dir;
	if(!a.directory.empty()) {
		if((a.directory != project_dir.path) | ImGui::MenuItem("Reload")) {
			int files = 0;
			project_tree_node new_project_dir;
			reload(files, new_project_dir, a.directory, 0);
			project_dir = new_project_dir;
		}
		ImGui::Separator();
		render_tree_node(project_dir);
	} else {
		ImGui::Text("<no directory open>");
	}
}

/*
	start_screen
*/

gui::start_screen::start_screen()
	: dvd(create_dvd_icon()),
	  folder(create_folder_icon()),
	  floppy(create_floppy_icon()) {}

const char* gui::start_screen::title_text() const {
	return "Start Screen";
}

ImVec2 gui::start_screen::initial_size() const {
	return ImVec2(800, 600);
}

void gui::start_screen::render(app& a) {
	static ImVec2 content_size(0, 0);
	
	ImVec2 start_pos(ImGui::GetWindowSize() / 2 - content_size / 2);
	// Fix horrible artifacting with the icons.
	start_pos.x = ceilf(start_pos.x);
	start_pos.y = ceilf(start_pos.y);
	ImGui::SetCursorPos(start_pos);
	
	ImVec2 icon_size(START_SCREEN_ICON_SIDE, START_SCREEN_ICON_SIDE);
	if(button("Extract ISO", (void*) (intptr_t) dvd.id, icon_size)) {
		nfdchar_t* in_path;
		if(NFD_OpenDialog("iso", nullptr, &in_path) == NFD_OKAY) {
			nfdchar_t* out_path;
			if(NFD_PickFolder(nullptr, &out_path) == NFD_OKAY) {
				a.extract_iso(in_path, out_path);
				free(out_path);
			}
			free(in_path);
		}
	}
	ImGui::SameLine();
	if(button("Open Dir", (void*) (intptr_t) folder.id, icon_size)) {
		nfdchar_t* path;
		if(NFD_PickFolder(nullptr, &path) == NFD_OKAY) {
			a.open_directory(path);
			free(path);
		}
	}
	ImGui::SameLine();
	if(button("Build ISO", (void*) (intptr_t) floppy.id, icon_size)) {
		nfdchar_t* in_path;
		if(NFD_PickFolder(nullptr, &in_path) == NFD_OKAY) {
			nfdchar_t* out_path;
			if(NFD_SaveDialog("iso", nullptr, &out_path) == NFD_OKAY) {
				a.build_iso({in_path, out_path});
				free(out_path);
			}
			free(in_path);
		}
	}
	ImGui::SameLine();
	
	if(content_size.y == 0) {
		ImVec2 end_pos = ImGui::GetCursorPos();
		content_size = end_pos - start_pos;
		content_size.y += 110; // Hack the get it vertically centred.
	}
}

// Adapted from ImGui::ImageButton.
bool gui::start_screen::button(const char* str, ImTextureID user_texture_id, const ImVec2& icon_size) const {
	ImVec4 bg_col(0, 0, 0, 0);
	
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	if(window->SkipItems) {
		return false;
	}
	
	// Default to using texture ID as ID. User can still push string/integer prefixes.
	ImGui::PushID((void*)(intptr_t)user_texture_id);
	const ImGuiID id = window->GetID("#image");
	ImGui::PopID();
	
	const ImVec2 size(128, 128);
	const ImVec2 padding(8, 6);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
	ImGui::ItemSize(bb);
	if(!ImGui::ItemAdd(bb, id)) {
		return false;
	}
	const ImVec2 icon_mid((bb.Min.x + bb.Max.x) / 2, bb.Min.y + padding.y + icon_size.y / 2);
	
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
	
	// Render
	const ImU32 col = (held && hovered) ? ImGui::GetColorU32(ImGuiCol_ButtonActive) : hovered ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : 0;
	ImGui::RenderNavHighlight(bb, id);
	ImGui::RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, g.Style.FrameRounding));
	window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(bg_col));
	window->DrawList->AddImage(user_texture_id, icon_mid - icon_size / 2, icon_mid + icon_size / 2);
	
	float text_size = ImGui::GetFontSize() * (strlen(str) + 1) / 2;
	ImVec2 text_mid(icon_mid.x - text_size / 2, bb.Max.y - padding.y - ImGui::GetFontSize());
	window->DrawList->AddText(text_mid, 0xffffffff, str);
	
	return pressed;
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

// Don't pass untrusted input to this!
void gui::open_in_browser(const char* url) {
	std::string cmd = "xdg-open " + std::string(url);
	int result = system(cmd.c_str());
	if(result != 0) {
		fprintf(stderr, "error: Failed to execute shell command.\n");
	}
}
