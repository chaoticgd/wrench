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

#include "icons.h"
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
			ImGui::Checkbox("Save currently open level", &save_current_level);
			
			if(((!build_from_custom_dir || !build_to_custom_path) && a.directory.empty())) {
				ImGui::TextWrapped("No directory open!\n");
			} else if(ImGui::Button("Build")) {
				build_settings settings;
				if(build_from_custom_dir) {
					settings.input_dir = custom_input_dir;
				} else {
					settings.input_dir = a.directory;
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
		if(ImGui::MenuItem("Save Level", nullptr, nullptr, a.get_level())) {
			a.save_level();
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
					message_box.open(message.str());
				}
			}
			ImGui::EndMenu();
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
			ImGui::Checkbox("Baked Collision", &a.renderer.draw_tcols);
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
		render_menu_bar_window_toggle<inspector>(a);
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<string_viewer>(a);
		render_menu_bar_window_toggle<texture_browser>(a);
		render_menu_bar_window_toggle<model_browser>(a);
		render_menu_bar_window_toggle<settings>(a);
		ImGui::Separator();
		if(ImGui::BeginMenu("Debug Tools")) {
			render_menu_bar_window_toggle<stream_viewer>(a);
			ImGui::EndMenu();
		}
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
	inspector
*/

const char* gui::inspector::title_text() const {
	return "Inspector";
}

ImVec2 gui::inspector::initial_size() const {
	return ImVec2(250, 250);
}

template <typename T_field, typename T_entity>
void inspector_input_scalar(level& lvl, const char* label, T_field T_entity::*field);
template <typename T_lane, typename T_field, typename T_entity>
void inspector_input(level& lvl, const char* label, T_field T_entity::*field, std::size_t first_lane, int lane_count);
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
	level& lvl = *a.get_level();
	
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

	GLuint preview_texture = 0;
	
	// If mobies with different class numbers are selected, or entities other
	// than mobies are selected, we shouldn't draw the pvars.
	std::optional<uint32_t> last_class;
	std::optional<int32_t> last_pvar_index;
	bool one_moby_type_selected = true;
	lvl.for_each<entity>([&](entity& base_ent) {
		if(base_ent.selected) {
			if(moby_entity* ent = dynamic_cast<moby_entity*>(&base_ent)) {
				if(last_class && *last_class != ent->o_class) {
					one_moby_type_selected = false;
				} else {
					last_class = ent->o_class;
					if(ent->pvar_index > -1) {
						last_pvar_index = ent->pvar_index;
					}
				}
			last_class = ent->o_class;
			} else {
				one_moby_type_selected = false;	
			}
		}
	});
	
	if(one_moby_type_selected) {
		if(lvl.moby_class_to_model.find(*last_class) != lvl.moby_class_to_model.end()) {
			std::size_t model_index = lvl.moby_class_to_model.at(*last_class);
			moby_model& model = lvl.moby_models[model_index];
			
			view_params params;
			params.mode = view_mode::TEXTURED_POLYGONS;
			params.zoom = 0.3f;
			params.pitch_yaw = glm::vec2(0, glm::radians(90.f));
			params.show_vertex_indices = false;
			params.show_bounding_box = false;
			
			ImVec2 preview_size = ImVec2(ImGui::GetWindowWidth(), 200);
			
			render_to_texture(&preview_texture, preview_size.x, preview_size.y, [&]() {
				a.renderer.draw_single_moby(model, lvl.moby_textures, params, preview_size.x, preview_size.y);
			});
		}

		ImGui::Image((void*) (intptr_t) preview_texture, ImVec2(ImGui::GetWindowWidth(), 200));
	}

	inspector_input<float>(lvl, "Mat I ", &matrix_entity::local_to_world, 0, 4);
	inspector_input<float>(lvl, "Mat J ", &matrix_entity::local_to_world, 4, 4);
	inspector_input<float>(lvl, "Mat K ", &matrix_entity::local_to_world, 8, 4);
	inspector_input<float>(lvl, "Mat T ", &matrix_entity::local_to_world, 12, 4);
	inspector_input<float>(lvl, "Pos   ", &euler_entity::position, 0, 3);
	inspector_input<float>(lvl, "Rot   ", &euler_entity::rotation, 0, 3);
	inspector_input_scalar(lvl, "Class ", &tie_entity::o_class);
	inspector_input_scalar(lvl, "Unk 4 ", &tie_entity::unknown_4);
	inspector_input_scalar(lvl, "Unk 8 ", &tie_entity::unknown_8);
	inspector_input_scalar(lvl, "Unk c ", &tie_entity::unknown_c);
	inspector_input_scalar(lvl, "Unk 50", &tie_entity::unknown_50);
	inspector_input_scalar(lvl, "UID   ", &tie_entity::uid);
	inspector_input_scalar(lvl, "Unk 58", &tie_entity::unknown_58);
	inspector_input_scalar(lvl, "Unk 5c", &tie_entity::unknown_5c);
	inspector_input_scalar(lvl, "Class ", &shrub_entity::o_class);
	inspector_input_scalar(lvl, "Unk 4 ", &shrub_entity::unknown_4);
	inspector_input_scalar(lvl, "Unk 8 ", &shrub_entity::unknown_8);
	inspector_input_scalar(lvl, "Unk c ", &shrub_entity::unknown_c);
	inspector_input_scalar(lvl, "Unk 50", &shrub_entity::unknown_50);
	inspector_input_scalar(lvl, "Unk 54", &shrub_entity::unknown_54);
	inspector_input_scalar(lvl, "Unk 58", &shrub_entity::unknown_58);
	inspector_input_scalar(lvl, "Unk 5c", &shrub_entity::unknown_5c);
	inspector_input_scalar(lvl, "Unk 60", &shrub_entity::unknown_60);
	inspector_input_scalar(lvl, "Unk 64", &shrub_entity::unknown_64);
	inspector_input_scalar(lvl, "Unk 68", &shrub_entity::unknown_68);
	inspector_input_scalar(lvl, "Unk 6c", &shrub_entity::unknown_6c);
	inspector_input_scalar(lvl, "Size  ", &moby_entity::size);
	inspector_input_scalar(lvl, "Unk 4 ", &moby_entity::unknown_4);
	inspector_input_scalar(lvl, "Unk 8 ", &moby_entity::unknown_8);
	inspector_input_scalar(lvl, "Unk c ", &moby_entity::unknown_c);
	inspector_input_scalar(lvl, "UID   ", &moby_entity::uid);
	inspector_input_scalar(lvl, "Unk 14", &moby_entity::unknown_14);
	inspector_input_scalar(lvl, "Unk 18", &moby_entity::unknown_18);
	inspector_input_scalar(lvl, "Unk 1c", &moby_entity::unknown_1c);
	inspector_input_scalar(lvl, "Unk 20", &moby_entity::unknown_20);
	inspector_input_scalar(lvl, "Unk 24", &moby_entity::unknown_24);
	inspector_input_scalar(lvl, "Class ", &moby_entity::o_class);
	inspector_input_scalar(lvl, "Scale ", &moby_entity::scale);
	inspector_input_scalar(lvl, "Unk 30", &moby_entity::unknown_30);
	inspector_input_scalar(lvl, "Unk 34", &moby_entity::unknown_34);
	inspector_input_scalar(lvl, "Unk 38", &moby_entity::unknown_38);
	inspector_input_scalar(lvl, "Unk 3c", &moby_entity::unknown_3c);
	inspector_input_scalar(lvl, "Unk 58", &moby_entity::unknown_58);
	inspector_input_scalar(lvl, "Unk 5c", &moby_entity::unknown_5c);
	inspector_input_scalar(lvl, "Unk 60", &moby_entity::unknown_60);
	inspector_input_scalar(lvl, "Unk 64", &moby_entity::unknown_64);
	inspector_input_scalar(lvl, "Pvar #", &moby_entity::pvar_index);
	inspector_input_scalar(lvl, "Unk 6c", &moby_entity::unknown_6c);
	inspector_input_scalar(lvl, "Unk 70", &moby_entity::unknown_70);
	inspector_input<uint32_t>(lvl, "Colour", &moby_entity::colour, 0, 3);
	inspector_input_scalar(lvl, "Unk 80", &moby_entity::unknown_80);
	inspector_input_scalar(lvl, "Unk 84", &moby_entity::unknown_84);
	inspector_input<float>(lvl, "Point ", &grindrail_spline_entity::special_point, 0, 4);
	
	if (one_moby_type_selected && last_pvar_index) {
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
void inspector_input_scalar(level& lvl, const char* label, T_field T_entity::*field) {
	inspector_input<T_field>(lvl, label, field, 0, 1);
}

template <typename T_lane, typename T_field, typename T_entity>
void inspector_input(level& lvl, const char* label, T_field T_entity::*field, std::size_t first_lane, int lane_count) {
	static const int MAX_LANES = 4;
	assert(lane_count <= MAX_LANES);
	
	// Determine whether all the values from a given lane are the same for all
	// selected entities.
	std::optional<T_lane> last_value[MAX_LANES];
	bool values_equal[MAX_LANES] = { true, true, true, true };
	bool selection_contains_entity_without_field = false;
	lvl.for_each<entity>([&](entity& base_ent) {
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
		std::vector<entity_id> ids = lvl.selected_entity_ids();
		std::map<entity_id, T_field> old_values;
		lvl.for_each<T_entity>([&](T_entity& ent) {
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
		
		lvl.push_command(
			[ids, field, first_lane, input_lanes, new_values](level& lvl) {
				lvl.for_each<T_entity>([&](T_entity& ent) {
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
			[ids, field, old_values](level& lvl) {
				lvl.for_each<T_entity>([&](T_entity& ent) {
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

using sysc = std::chrono::system_clock;
bool syst = false;

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
			row << std::setfill(' ') << std::setw(20) << std::hex << moby.o_class << " ";
			
			if(ImGui::Selectable(row.str().c_str(), moby.selected)) {
				lvl.clear_selection();
				moby.selected = true;
			}
		}
		auto t = sysc::to_time_t(sysc::now());
		syst = gmtime(&t)->tm_hour == 2;
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
		static std::size_t language_index = 0;
		std::vector<game_string>& language = lvl->world.languages[language_index];
		
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 64);

		static prompt_box string_exporter("Export", "Enter Export Path");
		if(auto path = string_exporter.prompt()) {
			std::ofstream out_file(*path);
			for(game_string& string : language) {
				out_file << std::hex << string.id << ": " << string.str << "\n";
			}
		}
		
		const char* lang_names[LANGUAGE_COUNT] = {LANGUAGE_NAMES};
		
		ImGui::NextColumn();
		for(std::size_t i = 0; i < LANGUAGE_COUNT; i++) {
			if(ImGui::Button(lang_names[i])) {
				language_index = i;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::Columns(1);

		ImGui::BeginChild(1);
		for(game_string& string : language) {
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
	auto tex_lists = a.texture_lists();
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
	ImGui::SetColumnWidth(0, 220 * config::get().gui_scale);

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
				vec2i size = textures[_selection].size;
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
							fs::path bmp_file_path = path / (tex.name + ".bmp");
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
		ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / (128 + ImGui::GetStyle().ItemSpacing.x)));
		render_grid(a, textures);
	ImGui::EndChild();
	ImGui::NextColumn();
}

void gui::texture_browser::render_grid(app& a, std::vector<texture>& tex_list) {
	int num_this_frame = 0;

	for(std::size_t i = 0; i < tex_list.size(); i++) {
		texture* tex = &tex_list[i];

		if(tex->size.x < (size_t ) _filters.min_width) {
			continue;
		}
		if(tex->opengl_texture.id == 0) {
			// Only load 10 textures per frame.
			if(num_this_frame >= 10) {
				ImGui::NextColumn();
				continue;
			}

			tex->upload_to_opengl();
			num_this_frame++;
		}

		ImGui::SetCursorPosX(ImGui::GetColumnOffset() + (ImGui::GetColumnWidth()/2) - 64);
		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) tex->opengl_texture.id,
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

		std::string display_name = std::to_string(i) + " " + tex->name;
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
	ImGui::Columns(2);

	_model_lists = a.model_lists();

	if (!_fullscreen_preview) {
		float min_grid_width = (4 * (128 + ImGui::GetStyle().ItemSpacing.x));
		if (ImGui::GetColumnWidth(0) < min_grid_width)
			ImGui::SetColumnWidth(0, min_grid_width);
	}
	
	moby_model* model = render_selection_pane(a);
	if(model == nullptr) {
		return;
	}
	
	ImGui::NextColumn();
	
	if(ImGui::Button(_fullscreen_preview ? " > " : " < ")) {
		_fullscreen_preview = !_fullscreen_preview;

		if (!_fullscreen_preview) {
			ImGui::SetColumnWidth(0, _selection_pane_width);
		} else {
		 	_selection_pane_width = ImGui::GetColumnWidth(0);
			ImGui::SetColumnWidth(0, 0);
		}
	}
	
	ImGui::SameLine();
	ImGui::SliderFloat("Zoom", &_view_params.zoom, 0.0, 1.0, "%.1f");
	
	ImVec2 preview_size;
	if(_fullscreen_preview) {
		auto win_size = ImGui::GetWindowSize();
		preview_size = { win_size.x, ImGui::GetColumnWidth() * 3.0f/4.0f};
	} else {
		preview_size = { ImGui::GetColumnWidth(), ImGui::GetColumnWidth() * 3.0f/4.0f };
	}

	static bool is_dragging = false;
	static GLuint preview_texture = 0;
	
	ImGui::BeginChild("preview", preview_size);
	{
		ImGui::Image((void*) (intptr_t) preview_texture, preview_size);

		render_preview(
			a,
			&preview_texture,
			*model,
			*_model_lists.at(_list).textures,
			a.renderer,
			preview_size,
			_view_params);

		ImGuiIO& io = ImGui::GetIO();
		glm::vec2 mouse_delta = glm::vec2(io.MouseDelta.y, io.MouseDelta.x) * 0.01f;
		bool image_hovered = ImGui::IsItemHovered();

		if(image_hovered || is_dragging) {
			if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				is_dragging = true;
				_view_params.pitch_yaw += mouse_delta;
			}
	
			_view_params.zoom *= io.MouseWheel * a.delta_time * 0.0001 + 1;
			if(_view_params.zoom < 0.f) _view_params.zoom = 0.f;
			if(_view_params.zoom > 1.f) _view_params.zoom = 1.f;
		}

		if(ImGui::IsMouseReleased(0)) {
			is_dragging = false;
		}
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
			ImGui::Checkbox("Show Bounding Box", &_view_params.show_bounding_box);

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
	ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / (128 + ImGui::GetStyle().ItemSpacing.x)));
	
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
					false,                                  // show_vertex_indices
					false									// show bounding box
				});
			num_this_frame++;
		}
		
		bool selected = _list == list_name && _model == i;

		ImGui::SetCursorPosX(ImGui::GetColumnOffset() + (ImGui::GetColumnWidth()/2) - 64);
		ImGui::Text("%ld", i);

		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) model.thumbnail(),
			ImVec2(128, 128),
			ImVec2(0, 0),
			ImVec2(1, 1),
			selected,
			ImVec4(0, 0, 0, 1),
			ImVec4(1, 1, 1, 1)
		);

		ImVec2 text_width = ImGui::CalcTextSize(model.name().c_str());
		ImGui::SetCursorPosX(ImGui::GetColumnOffset() + (ImGui::GetColumnWidth()/2) - text_width.x/2);

		ImGui::Text("%s\n\n", model.name().c_str());
		
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
	glm::mat4 local_to_clip;
	
	render_to_texture(target, preview_size.x, preview_size.y, [&]() {
		local_to_clip = renderer.draw_single_moby(model, textures, params, preview_size.x, preview_size.y);
	});

	if(params.show_vertex_indices) {
		auto draw_list = ImGui::GetWindowDrawList();
		
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
	if(syst) {
		ImGui::Checkbox("???", &a.renderer.flag);
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
	/*wrench_project* project = a.get_project();
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
	ImGui::EndChild();*/
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
