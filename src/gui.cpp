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

#include "md5.h"
#include "util.h"
#include "config.h"
#include "window.h"
#include "renderer.h"
#include "worker_thread.h"
#include "formats/bmp.h"
#include "commands/translate_command.h"

#include "unwindows.h"

void gui::render(app& a) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	begin_docking();
	render_menu_bar(a);

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
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(a.window_width, a.window_height));

	ImGuiID main_left, far_right;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 8.f / 10.f, &main_left, &far_right);
	
	ImGuiID far_left, centre;
	ImGui::DockBuilderSplitNode(main_left, ImGuiDir_Left, 2.f / 8.f, &far_left, &centre);
	
	ImGuiID top_left, mobies;
	ImGui::DockBuilderSplitNode(far_left, ImGuiDir_Up, 0.75f, &top_left, &mobies);
	
	ImGuiID inspector, viewport_info;
	ImGui::DockBuilderSplitNode(far_right, ImGuiDir_Up, 0.75f, &inspector, &viewport_info);
	
	ImGuiID project, tools;
	ImGui::DockBuilderSplitNode(top_left, ImGuiDir_Up, 0.8f, &project, &tools);
	
	ImGui::DockBuilderDockWindow("3D View", centre);
	ImGui::DockBuilderDockWindow("Texture Browser", centre);
	ImGui::DockBuilderDockWindow("Model Browser", centre);
	ImGui::DockBuilderDockWindow("Documentation", centre);
	ImGui::DockBuilderDockWindow("Project", project);
	ImGui::DockBuilderDockWindow("Tools", tools);
	ImGui::DockBuilderDockWindow("Mobies", mobies);
	ImGui::DockBuilderDockWindow("Inspector", inspector);
	ImGui::DockBuilderDockWindow("Viewport Information", viewport_info);

	ImGui::DockBuilderFinish(dockspace_id);
}

void gui::begin_docking() {
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
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

void gui::render_menu_bar(app& a) {
	ImGui::BeginMainMenuBar();
	
	if(ImGui::BeginMenu("File")) {
		if(ImGui::BeginMenu("New")) {
			for(const game_iso& game : a.settings.game_isos) {
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
			a.save_project(false);
		}
		if(ImGui::MenuItem("Save As")) {
			a.save_project(true);
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
					level_code_segment segment = lvl->read_code_segment();
					
					std::stringstream name;
					name << "codeseg";
					name << "_" << std::hex << segment.header.base_address;
					name << "_" << std::hex << segment.header.unknown_4;
					name << "_" << std::hex << segment.header.unknown_8;
					name << "_" << std::hex << segment.header.entry_offset;
					name << ".bin";
					
					file_stream dump_file(name.str(), std::ios::out | std::ios::trunc);
					dump_file.write_v(segment.bytes);
					
					std::stringstream message;
					message << "The code segment for the current level has been written to\n\t\"";
					message << name.str() << "\"\n";
					message << "relative to the main Wrench directory.\n";
					message << "\n";
					message << "Base address: " << std::hex << segment.header.base_address << "\n";
					message << "Unknown (0x4): " << std::hex << segment.header.unknown_4 << "\n";
					message << "Unknown (0x8): " << std::hex << segment.header.unknown_8 << "\n";
					message << "Entry point: " << std::hex << segment.header.entry_offset << "\n";
					a.emplace_window<message_box>("Export Complete", message.str());
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}

	if(ImGui::BeginMenu("Edit")) {
		if(auto project = a.get_project()) {
			if(ImGui::MenuItem("Undo")) {
				try {
					project->undo();
				} catch(command_error& error) {
					a.emplace_window<message_box>("Undo Error", error.what());
				}
			}
			if(ImGui::MenuItem("Redo")) {
				try {
					project->redo();
				} catch(command_error& error) {
					a.emplace_window<message_box>("Redo Error", error.what());
				}
			}
		}
		ImGui::EndMenu();
	}
	
	if(ImGui::BeginMenu("Emulator")) {
		if(ImGui::MenuItem("Run")) {
			a.run_emulator();
		}
		ImGui::EndMenu();
	}
	
	static const int level_list_rows = 20;
	
	if(auto project = a.get_project()) {
		auto& levels = project->toc.levels;
		int columns = (int) std::ceil(levels.size() / (float) level_list_rows);
		
		ImGui::SetNextWindowContentWidth(columns * 192);
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
				label << std::setfill('0') << std::setw(2) << levels[i].main_part.level_number;
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

	ImGui::SetNextWindowContentWidth(0.f); // Reset the menu width after the "Levels" menu made it larger.
	if(ImGui::BeginMenu("Windows")) {
		render_menu_bar_window_toggle<view_3d>(a, &a);
		render_menu_bar_window_toggle<moby_list>(a);
		render_menu_bar_window_toggle<inspector>(a);
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<tools>(a);
		render_menu_bar_window_toggle<string_viewer>(a);
		render_menu_bar_window_toggle<texture_browser>(a);
		render_menu_bar_window_toggle<model_browser>(a);
		render_menu_bar_window_toggle<manual_patcher>(a);
		render_menu_bar_window_toggle<settings>(a);
		render_menu_bar_window_toggle<document_viewer>(a, "index.md");
		ImGui::EndMenu();
	}
	
	auto open_document_viewer = [=, &a](const char* path) {
		// Close any existing documentation windows.
		for(auto& window : a.windows) {
			if(dynamic_cast<document_viewer*>(window.get()) != nullptr) {
				window->close(a);
			}
		}
		a.emplace_window<document_viewer>(path);
	};
	
	if(ImGui::BeginMenu("Help")) {
		if(ImGui::MenuItem("About")) {
			a.emplace_window<message_box>(
				"About Wrench Editor",
				"A set of modding tools for the\n"
				"Ratchet & Clank PS2 games.\n"
				"\n"
				"Application version: " WRENCH_VERSION_STR "\n"
				"License: GPLv3+ (see LICENSE file)\n"
				"\n"
				"Libraries used:\n"
				" - better-enums: https://github.com/aantron/better-enums\n"
				" - Dear ImGui: https://github.com/ocornut/imgui\n"
				" - imgui_markdown: https://github.com/juliettef/imgui_markdown\n"
				" - nlohmann json: https://github.com/nlohmann/json\n"
				" - toml11: https://github.com/ToruNiina/toml11\n"
				" - ZipLib: https://bitbucket.org/wbenny/ziplib/wiki/Home\n"
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
	
	ImGui::EndMainMenuBar();
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

void gui::inspector::render(app& a) {
	if(!a.get_level()) {
		ImGui::Text("<no level>");
		return;
	}
	
	_project = a.get_project();
	_lvl = a.get_level();
	_num_properties = 0;
	
	if(_lvl->world.selection.size() < 1) {
		ImGui::Text("<no object selected>");
		return;
	}
	
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 80);

	object_list& sel = _lvl->world.selection;

	if(sel.ties.size() > 0) {
		category("Tie");
	}
	
	if(sel.shrubs.size() > 0) {
		category("Shrub");
	}
	
	if(sel.mobies.size() > 0) {
		category("Moby");
		
		input_vec3<moby>(sel.mobies, "Position", &moby::position);
		input_vec3<moby>(sel.mobies, "Rotation", &moby::rotation);
		input_i32 <moby>(sel.mobies, "UID",      &moby::uid);
		input_u32 <moby>(sel.mobies, "Class",    &moby::class_num);
	}
	
	if(sel.splines.size() > 0) {
		category("Spline");
	}
	
	ImGui::PopStyleVar();
	
	ImGui::Columns(1);
	
	if(sel.size() == 1 && sel.splines.size() == 0) {
		_lvl->world.find_object_by_id(sel.first(), [&a](object_id id, auto& object) {
			if(ImGui::Button("Hex Dump (Debug)")) {
				a.emplace_window<hex_dump>((uint8_t*) &object, sizeof(object));
			}
		});
	}
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
	ImGui::ListBoxHeader("##mobylist", size);
	lvl.world.for_each_object_of_type<moby>([=, &lvl](object_id id, moby& object) {
		std::stringstream row;
		row << std::setfill(' ') << std::setw(8) << std::dec << object.uid << " ";
		row << std::setfill(' ') << std::setw(20) << std::hex << object.class_num << " ";
		
		bool is_selected = lvl.world.is_selected(id);
		if(ImGui::Selectable(row.str().c_str(), is_selected)) {
			lvl.world.selection = {};
			lvl.world.selection.add<moby>(id);
		}
	});
	ImGui::ListBoxFooter();
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
	int fps = a.delta_time == 0 ? 0 : 1000000.0 / a.delta_time;
	ImGui::Text("FPS:\n\t%d\n", fps);
	glm::vec3 cam_pos = a.renderer.camera_position;
	ImGui::Text("Camera Position:\n\t%.3f, %.3f, %.3f",
		cam_pos.x, cam_pos.y, cam_pos.z);
	glm::vec2 cam_rot = a.renderer.camera_rotation;
	ImGui::Text("Camera Rotation:\n\tPitch=%.3f, Yaw=%.3f",
		cam_rot.x, cam_rot.y);
	ImGui::Text("Camera Control (Z to toggle):\n\t%s",
		a.renderer.camera_control ? "On" : "Off");
		
	if(ImGui::Button("Reset Camera")) {
		a.renderer.reset_camera(&a);
	}
}

/*
	tools
*/

gui::tools::tools()
	: _picker_icon(load_icon("data/icons/picker_tool.txt")),
	  _selection_icon(load_icon("data/icons/selection_tool.txt")),
	  _translate_icon(load_icon("data/icons/translate_tool.txt")) {}

gui::tools::~tools() {
	glDeleteTextures(1, &_picker_icon);
	glDeleteTextures(1, &_selection_icon);
	glDeleteTextures(1, &_translate_icon);
}

const char* gui::tools::title_text() const {
	return "Tools";
}

ImVec2 gui::tools::initial_size() const {
	return ImVec2(200, 50);
}

void gui::tools::render(app& a) {
	const char* tool_name = "\n";
	switch(a.current_tool) {
		case tool::picker:    tool_name = "Picker";    break;
		case tool::selection: tool_name = "Selection"; break;
		case tool::translate: tool_name = "Translate"; break;
	}
	ImGui::Text("Current Tool: %s", tool_name);
	
	if(ImGui::ImageButton((void*) (intptr_t) _picker_icon, ImVec2(32, 32))) {
		a.current_tool = tool::picker;
	}
	ImGui::SameLine();
	if(ImGui::ImageButton((void*) (intptr_t) _selection_icon, ImVec2(32, 32))) {
		a.current_tool = tool::selection;
	}
	ImGui::SameLine();
	if(ImGui::ImageButton((void*) (intptr_t) _translate_icon, ImVec2(32, 32))) {
		a.current_tool = tool::translate;
	}
	
	if(a.current_tool == tool::translate) {
		ImGui::Text("Displacement:");
		ImGui::InputFloat3("##displacement_input", &a.translate_tool_displacement.x);
		if(ImGui::Button("Apply")) {
			if(auto lvl = a.get_level()) {
				a.get_project()->emplace_command<translate_command>
					(lvl, lvl->world.selection, a.translate_tool_displacement);
			}
			a.translate_tool_displacement = glm::vec3(0, 0, 0);
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
	if(auto lvl = a.get_level()) {		
		auto strings = lvl->game_strings();

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 64);

		if(ImGui::Button("Export")) {
			auto string_exporter = std::make_unique<string_input>("Enter Export Path");
			string_exporter->on_okay([=](app& a, std::string path) {
				auto lang = std::find_if(strings.begin(), strings.end(),
					[=](auto& ptr) { return ptr.first == _selected_language; });
				if(lang == strings.end()) {
					return;
				}
				std::ofstream out_file(path);
				for(auto& [id, string] : lang->second) {
					out_file << std::hex << id << ": " << string << "\n";
				}
			});
			a.windows.emplace_back(std::move(string_exporter));
		}

		ImGui::NextColumn();

		for(auto& language : strings) {
			if(ImGui::Button(language.first.c_str())) {
				_selected_language = language.first;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

		ImGui::Columns(1);

		auto lang = std::find_if(strings.begin(), strings.end(),
			[=](auto& ptr) { return ptr.first == _selected_language; });
		if(lang == strings.end()) {
			return;
		}

		ImGui::BeginChild(1);
		for(auto& string : lang->second) {
			ImGui::Text("%x: %s", string.first, string.second.c_str());
		}
		ImGui::EndChild();
	}
}

/*
	texture_browser
*/

gui::texture_browser::texture_browser() {}

gui::texture_browser::~texture_browser() {
	for(auto& tex : _gl_textures) {
		glDeleteTextures(1, &tex.second);
	}
}

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
	
	// Clear the texture cache when a new project is opened.
	if(a.get_project()->id() != _project_id) {
		for(auto& tex : _gl_textures) {
			glDeleteTextures(1, &tex.second);
		}
		_gl_textures.clear();
		_project_id = a.get_project()->id();
	}

	auto tex_lists = a.get_project()->texture_lists();
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
				ImGui::Text("Width:  %d", size.x);
				ImGui::Text("Height: %d", size.y);
			} else {
				ImGui::Text("<no texture selected>");
			}
			ImGui::TreePop();
		}
		ImGui::NewLine();

		if(ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
			if(textures.size() > 0) {
				if(ImGui::Button("Replace Selected")) {
					import_bmp(a, &textures[_selection]);
				}
				if(ImGui::Button("Export Selected")) {
					export_bmp(a, &textures[_selection]);
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

		if(_gl_textures.find(tex) == _gl_textures.end()) {

			// Only load 10 textures per frame.
			if(num_this_frame >= 10) {
				ImGui::NextColumn();
				continue;
			}

			cache_texture(tex);
			num_this_frame++;
		}

		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) _gl_textures.at(tex),
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

void gui::texture_browser::cache_texture(texture* tex) {
	auto size = tex->size();

	// Prepare pixel data.
	std::vector<uint8_t> indexed_pixel_data = tex->pixel_data();
	std::vector<uint8_t> colour_data(indexed_pixel_data.size() * 4);
	auto palette = tex->palette();
	for(std::size_t i = 0; i < indexed_pixel_data.size(); i++) {
		colour c = palette[indexed_pixel_data[i]];
		colour_data[i * 4] = c.r;
		colour_data[i * 4 + 1] = c.g;
		colour_data[i * 4 + 2] = c.b;
		colour_data[i * 4 + 3] = static_cast<int>(c.a) * 2 - 1;
	}

	// Send image to OpenGL.
	GLuint texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, colour_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	_gl_textures[tex] = texture_id;
}

void gui::texture_browser::import_bmp(app& a, texture* tex) {
	auto importer = std::make_unique<string_input>("Enter Import Path");
	importer->on_okay([=](app& a, std::string path) {
		try {
			file_stream bmp_file(path);
			bmp_to_texture(tex, bmp_file);
			cache_texture(tex);
		} catch(stream_error& e) {
			a.emplace_window<message_box>("Error", e.what());
		}
	});
	a.windows.emplace_back(std::move(importer));
}

void gui::texture_browser::export_bmp(app& a, texture* tex) {
	// Filter out characters not allowed in file paths (on certain platforms).
	std::string default_file_path = tex->pixel_data_path() + ".bmp";
	const static std::string foridden = "<>:\"/\\|?*";
	for(char& c : default_file_path) {
		if(std::find(foridden.begin(), foridden.end(), c) != foridden.end()) {
			c = '_';
		}
	}

	auto exporter = std::make_unique<string_input>
		("Enter Export Path", default_file_path);
	exporter->on_okay([=](app& a, std::string path) {
		try {
			file_stream bmp_file(path, std::ios::in | std::ios::out | std::ios::trunc);
			texture_to_bmp(bmp_file, tex);
		} catch(stream_error& e) {
			a.emplace_window<message_box>("Error", e.what());
		}
	});
	a.windows.emplace_back(std::move(exporter));
}

/*
	model_browser
*/

gui::model_browser::model_browser() {}

gui::model_browser::~model_browser() {
	for(auto& tex : _model_thumbnails) {
		glDeleteTextures(1, &tex.second);
	}
}
	
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
	ImGui::SetColumnWidth(1, 384);
	
	game_model* model = render_selection_pane(a);
	if(model == nullptr) {
		return;
	}
	
	ImGui::NextColumn();
	
	// Update zoom and rotation.
	if(ImGui::IsWindowHovered()) {
		ImGuiIO& io = ImGui::GetIO();
		_zoom *= -io.MouseWheel * a.delta_time * 0.0001 + 1;
		if(_zoom < 0.2) _zoom = 0.2;
		if(_zoom > 4) _zoom = 4;	
		
		if(ImGui::IsMouseReleased(0)) {
			_pitch_yaw += get_drag_delta();
		}
	}
	
	// Clear the texture cache when a new project is opened.
	if(a.get_project()->id() != _project_id) {
		for(auto& tex : _model_thumbnails) {
			glDeleteTextures(1, &tex.second);
		}
		_model_thumbnails.clear();
		_project_id = a.get_project()->id();
	}
	
	ImVec2 preview_size { 400, 300 };
	static GLuint preview_texture = 0;
	render_preview(&preview_texture, *model, a.renderer, preview_size, _zoom, _pitch_yaw);
	ImGui::Image((void*) (intptr_t) preview_texture, preview_size);
	
	if(ImGui::BeginTabBar("tabs")) {
		if(ImGui::BeginTabItem("Details")) {
			std::string index = std::to_string(_model);
			ImGui::InputText("Index", &index, ImGuiInputTextFlags_ReadOnly);
			std::string res_path = model->resource_path();
			ImGui::InputText("Resource Path", &res_path, ImGuiInputTextFlags_ReadOnly);
			ImGui::EndTabItem();
		}
		
		if(ImGui::BeginTabItem("VIF Chains (Debug)")) {
			ImGui::BeginChild(2);
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

game_model* gui::model_browser::render_selection_pane(app& a) {
	game_model* result = nullptr;
	
	auto lists = a.get_project()->model_lists();
	if(ImGui::BeginTabBar("lists")) {
		for(auto& list : lists) {
			if(ImGui::BeginTabItem(list.first.c_str())) {
				result = render_selection_grid(a, list.first, *list.second);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	return result;
}

game_model* gui::model_browser::render_selection_grid(
		app& a,
		std::string list,
		std::vector<game_model>& models) {
	game_model* result = nullptr;
	std::size_t num_this_frame = 0;
	
	ImGui::BeginChild(1);
	ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / 128));
	
	for(std::size_t i = 0; i < models.size(); i++) {
		game_model* model = &models[i];
		
		if(_model_thumbnails.find(model) == _model_thumbnails.end()) {
			// Only load 10 textures per frame.
			if(num_this_frame >= 10) {
				ImGui::NextColumn();
				continue;
			}
			
			_model_thumbnails[model] = 0;
			render_preview(
				&_model_thumbnails.at(model),
				*model, a.renderer,
				ImVec2(128, 128), 1, glm::vec2(0, 0));
			num_this_frame++;
		}
		
		bool selected = _list == list && _model == i;
		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) _model_thumbnails.at(model),
			ImVec2(128, 128),
			ImVec2(0, 0),
			ImVec2(1, 1),
			selected,
			ImVec4(0, 0, 0, 1),
			ImVec4(1, 1, 1, 1)
		);
		ImGui::Text("%d\n", i);
		
		if(clicked) {
			_list = list;
			_model = i;
		}
		if(selected) {
			result = model;
		}
		
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	
	return result;
}

void gui::model_browser::render_preview(
		GLuint* target,
		const game_model& model,
		const gl_renderer& renderer,
		ImVec2 preview_size,
		float zoom,
		glm::vec2 pitch_yaw) {
	if(ImGui::IsMouseDragging()) {
		pitch_yaw += get_drag_delta();
	}
	
	glm::vec3 eye = glm::vec3(_zoom, 0, 0);
	
	glm::mat4 view_fixed = glm::lookAt(eye, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 view_pitched = glm::rotate(view_fixed, pitch_yaw.x, glm::vec3(0, 0, 1));
	glm::mat4 view = glm::rotate(view_pitched, pitch_yaw.y, glm::vec3(0, 1, 0));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), preview_size.x / preview_size.y, 0.1f, 100.0f);
	
	static const glm::mat4 yzx {
		0,  0, 1, 0,
		1,  0, 0, 0,
		0, -1, 0, 0,
		0,  0, 0, 1
	};
	glm::mat4 vp = projection * view * yzx;
	
	glDeleteTextures(1, target);
	
	glGenTextures(1, target);
	glBindTexture(GL_TEXTURE_2D, *target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, preview_size.x, preview_size.y, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint fb_id;
	glGenFramebuffers(1, &fb_id);
	glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *target, 0);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, preview_size.x, preview_size.y);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(renderer.shaders.solid_colour.id());
	
	try {
		renderer.draw_model(model, vp, glm::vec4(0, 1, 0, 1));
	} catch(stream_error& e) {
		glClearColor(1, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		ImGui::Text("Error: Out of bounds read.");
	}

	glDeleteFramebuffers(1, &fb_id);
}

glm::vec2 gui::model_browser::get_drag_delta() const {
	auto delta = ImGui::GetMouseDragDelta();
	return glm::vec2(delta.y, delta.x) * 0.01f;
}

void gui::model_browser::render_dma_debug_info(game_model& mdl) {
	for(std::size_t submodel = 0; submodel < mdl.num_submodels; submodel++) {
		ImGui::PushID(submodel);
		
		if(ImGui::TreeNode("submodel", "Submodel %ld", submodel)) {
			std::vector<std::vector<vif_packet>> chains = {
				mdl.get_vif_chain(submodel)
			};
			for(int i = 0; i < 1; i++) {
				auto& chain = chains[i];
				ImGui::Text("  Chain %d:", i);
				for(vif_packet& vpkt : chain) {
					ImGui::PushID(vpkt.address);
						
					if(vpkt.error != "") {
						ImGui::Text("   (error: %s)", vpkt.error.c_str());
						ImGui::PopID();
						continue;
					}
					
					std::string label = vpkt.code.to_string();
					if(ImGui::TreeNode("packet", "%lx %s", vpkt.address, label.c_str())) {
						auto lines = to_hex_dump(vpkt.data.data(), vpkt.address, vpkt.data.size());
						for(std::string& line : lines) {
							ImGui::Text("    %s", line.c_str());
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
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
		if(ImGui::BeginTabItem("Paths")) {
			render_paths_page(a);
			ImGui::EndTabItem();
		}
		
		if(ImGui::BeginTabItem("GUI")) {
			render_gui_page(a);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	ImGui::NewLine();
	if(ImGui::Button("Close")) {
		close(a);
	}
}

void gui::settings::render_paths_page(app& a) {
	ImGui::Text("Emulator Path");

	ImGui::PushItemWidth(-1);
	if(ImGui::InputText("##emulator_path", &a.settings.emulator_path)) {
		a.save_settings();
	}
	ImGui::PopItemWidth();
	ImGui::NewLine();

	ImGui::Text("Game Paths");

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, ImGui::GetWindowSize().x - 32);

	for(auto iter = a.settings.game_isos.begin(); iter < a.settings.game_isos.end(); iter++) {
		ImGui::PushID(0);
		std::stringstream label;
		label << iter->game_db_entry << " " << iter->md5;
		ImGui::InputText(label.str().c_str(), &iter->path, ImGuiInputTextFlags_ReadOnly);
		ImGui::NextColumn();
		if(ImGui::Button("X")) {
			a.settings.game_isos.erase(iter);
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
					iso.seek(0);
					
					log << "Generating MD5 hash... ";
					
					MD5_CTX ctx;
					MD5Init(&ctx);
					
					static const std::size_t BLOCK_SIZE = 1024 * 4;
					
					std::vector<uint8_t> block(BLOCK_SIZE);
					for(std::size_t i = 0; i < iso.size() / BLOCK_SIZE; i++) {
						iso.read_n((char*) block.data(), BLOCK_SIZE);
						MD5Update(&ctx, block.data(), BLOCK_SIZE);
					}
					iso.read_n((char*) block.data(), iso.size() % BLOCK_SIZE);
					MD5Update(&ctx, block.data(), iso.size() % BLOCK_SIZE);

					uint8_t digest[MD5_DIGEST_LENGTH];
					MD5Final(digest, &ctx);
					
					log << "done!";
					
					game.md5 = md5_to_printable_string(digest);
					return std::optional<game_iso>(game);
				} catch(stream_error& e) {
					log << "Error: " << e.what();
				}
				return std::optional<game_iso>();
			},
			[&](game_iso game) {
				a.settings.game_isos.push_back(game);
			}
		);
		
		_new_game_path = "";
	}
}

void gui::settings::render_gui_page(app& a) {
	ImGui::Text("GUI Scale");

	ImGui::PushItemWidth(-1);
	if(ImGui::SliderFloat("##gui_scale", &a.settings.gui_scale, 0.5, 2, "%.1f")) {
		a.update_gui_scale();
		a.save_settings();
	}
	ImGui::PopItemWidth();
}

/*
	manual_patcher
*/

gui::manual_patcher::manual_patcher()
	: _scroll_offset(0) {}

const char* gui::manual_patcher::title_text() const {
	return "Manual Patcher (debug)";
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
	ImGui::InputTextMultiline("##message", &_message, size, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();
	if(ImGui::Button("Close")) {
		close(a);
	}
}

bool gui::message_box::is_unique() const {
	return false;
}

/*
	string_input
*/

gui::string_input::string_input(const char* title, std::string default_text)
	: _title_text(title),
	  _input(default_text) {}

const char* gui::string_input::title_text() const {
	return _title_text;
}

ImVec2 gui::string_input::initial_size() const {
	return ImVec2(400, 100);
}

void gui::string_input::render(app& a) {
	ImGui::InputText("", &_input);
	bool pressed = ImGui::Button("Okay");
	if(pressed) {
		_callback(a, _input);
	}
	pressed |= ImGui::Button("Cancel");
	if(pressed) {
		close(a);
	}
}

bool gui::string_input::is_unique() const {
	return false;
}

void gui::string_input::on_okay(std::function<void(app&, std::string)> callback) {
	_callback = callback;
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

GLuint gui::load_icon(std::string path) {
	std::ifstream image_file(path);
	
	uint32_t image_buffer[32][32];
	for(std::size_t y = 0; y < 32; y++) {
		std::string line;
		std::getline(image_file, line);
		if(line.size() > 32) {
			line = line.substr(0, 32);
		}
		for(std::size_t x = 0; x < line.size(); x++) {
			image_buffer[y][x] = line[x] == '#' ? 0xffffffff : 0x00000000;
		}
		for(std::size_t x = line.size(); x < 32; x++) {
			image_buffer[y][x] = 0;
		}
	}
	
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	return texture;
}

// Don't pass untrusted input to this!
void gui::open_in_browser(const char* url) {
	std::string cmd = "xdg-open " + std::string(url);
	system(cmd.c_str());
}
