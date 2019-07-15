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

#include "window.h"
#include "renderer.h"
#include "inspector.h"
#include "formats/bmp.h"

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
		render_menu_bar_window_toggle<three_d_view>(a, &a);
		render_menu_bar_window_toggle<moby_list>(a);
		render_menu_bar_window_toggle<inspector>(a, &a.selection);
		render_menu_bar_window_toggle<viewport_information>(a);
		render_menu_bar_window_toggle<string_viewer>(a);
		render_menu_bar_window_toggle<texture_browser>(a);
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
	moby_list
*/

const char* gui::moby_list::title_text() const {
	return "Moby List";
}

ImVec2 gui::moby_list::initial_size() const {
	return ImVec2(250, 500);
}

void gui::moby_list::render(app& a) {
	if(auto lvl = a.get_level()) {
		ImVec2 size = ImGui::GetWindowSize();
		size.x -= 16;
		size.y -= 64;

		ImGui::Text("UID  Class");

		ImGui::PushItemWidth(-1);
		ImGui::ListBoxHeader("##nolabel", size);
		for(const auto& [uid, moby] : lvl->mobies()) {
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
	}
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

gui::texture_browser::texture_browser()
	: _selection(nullptr),
	  _provider(nullptr) {}

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
	if(!a.get_level()) {
		ImGui::Text("<no level open>");
		return;
	}

	std::vector<texture_provider*> sources = a.texture_providers();
	if(_provider == nullptr) {
		_provider = sources[0];
	}

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 192);

	ImGui::BeginChild(1);
		if(ImGui::TreeNodeEx("Sources", ImGuiTreeNodeFlags_DefaultOpen)) {
			for(texture_provider* provider : sources) {
				if(ImGui::Button(provider->display_name().c_str())) {
					_provider = provider;
				}
			}
			ImGui::TreePop();
		}
		ImGui::NewLine();

		if(ImGui::TreeNodeEx("Filters", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Text("Minimum Width:");
			ImGui::PushItemWidth(-1);
			ImGui::InputInt("##nolabel", &_filters.min_width);
			ImGui::PopItemWidth();
			ImGui::TreePop();
		}
		ImGui::NewLine();

		if(ImGui::TreeNodeEx("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
			if(a.selection.type() == typeid(texture*)) {
				texture* tex = std::any_cast<texture*>(a.selection);
				if(ImGui::Button("Import Selected")) {
					import_bmp(a, tex);
				}
				if(ImGui::Button("Export Selected")) {
					export_bmp(a, tex);
				}
			}
			ImGui::TreePop();
		}
		ImGui::NewLine();
	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild(2);
		ImGui::Columns(std::max(1.f, ImGui::GetWindowSize().x / 128));
		render_grid(a, _provider);
	ImGui::EndChild();
	ImGui::NextColumn();
}

void gui::texture_browser::render_grid(app& a, texture_provider* provider) {
	int num_this_frame = 0;

	auto textures = provider->textures();
	for(std::size_t i = 0; i < textures.size(); i++) {
		texture* tex = textures[i];

		if(tex->size().x < _filters.min_width) {
			continue;
		}

		if(_gl_textures.find(tex) == _gl_textures.end()) {

			// Only load 10 textures per frame.
			if(num_this_frame > 10) {
				ImGui::NextColumn();
				continue;
			}

			cache_texture(tex);
			num_this_frame++;
		}

		bool selected =
			a.selection.type() == typeid(texture*) &&
			std::any_cast<texture*>(a.selection) == tex;

		bool clicked = ImGui::ImageButton(
			(void*) (intptr_t) _gl_textures.at(tex),
			ImVec2(128, 128),
			ImVec2(0, 0),
			ImVec2(1, 1),
			selected ? 2 : 0,
			ImVec4(0, 0, 0, 1),
			ImVec4(1, 1, 1, 1)
		);
		if(clicked) {
			a.selection = tex;
		}

		std::string num = std::to_string(i);
		ImGui::Text("%s", num.c_str());
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

void gui::string_input::on_okay(std::function<void(app&, std::string)> callback) {
	_callback = callback;
}
