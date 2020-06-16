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

#include "app.h"

#include <stdlib.h>
#include "unwindows.h"
#include <toml11/toml.hpp>

#include "gui.h"
#include "stream.h"
#include "renderer.h"
#include "fs_includes.h"
#include "worker_thread.h"

app::app()
	: mouse_last(0, 0),
	  delta_time(0),
	  current_tool(tool::picker),
	  translate_tool_displacement(0, 0, 0),
	  game_db(gamedb_read()),
	  _lock_project(false) {
	
	read_settings();
}

using project_ptr = std::unique_ptr<wrench_project>;

void app::new_project(game_iso game) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	_project.reset(nullptr);

	// TODO: Make less ugly.
	using worker_type = worker_thread<project_ptr, game_iso>;
	windows.emplace_back(std::make_unique<worker_type>(
		"New Project", game,
		[](game_iso game, worker_logger& log) {
			try {
				auto result = std::make_unique<wrench_project>(game, log);
				log << "\nProject created successfully.";
				return std::make_optional(std::move(result));
			} catch(stream_error& err) {
				log << err.what() << "\n";
				log << err.stack_trace;
			}
			return std::optional<project_ptr>();
		},
		[=](project_ptr project) {
			_project.swap(project);
			_lock_project = false;

			renderer.reset_camera(this);
			
			glfwSetWindowTitle(glfw_window, "Wrench Editor - [Unsaved Project]");
		}
	));
}

void app::open_project(std::string path) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	_project.reset(nullptr);

	// TODO: Make less ugly.
	using worker_type = worker_thread<project_ptr, std::pair<std::vector<game_iso>, std::string>>;
	auto in = std::make_pair(settings.game_isos, path);
	windows.emplace_back(std::make_unique<worker_type>(
		"Open Project", in,
		[](auto paths, worker_logger& log) {
			try {
				auto result = std::make_unique<wrench_project>(paths.first, paths.second, log);
				log << "\nProject opened successfully.";
				return std::make_optional(std::move(result));
			} catch(stream_error& err) {
				log << err.what() << "\n";
				log << err.stack_trace;
			}
			return std::optional<project_ptr>();
		},
		[=](project_ptr project) {
			_project.swap(project);
			_lock_project = false;

			renderer.reset_camera(this);
			
			auto title = std::string("Wrench Editor - [") + path + "]";
			glfwSetWindowTitle(glfw_window, title.c_str());
		}
	));
}

void app::save_project(bool save_as) {
	if(_project.get() == nullptr) {
		return;
	}

	auto on_done = [=]() {
		auto title = std::string("Wrench Editor - [") + _project->project_path() + "]";
		glfwSetWindowTitle(glfw_window, title.c_str());
	};

	try {
		if(save_as) {
			_project->save_as(this, on_done);
		} else {
			_project->save(this, on_done);
		}
	} catch(stream_error& err) {
		std::stringstream error_message;
		error_message << err.what() << "\n";
		error_message << err.stack_trace;
		emplace_window<gui::message_box>("Error Saving Project", error_message.str());
	}
}

wrench_project* app::get_project() {
	return _project.get();
}

const wrench_project* app::get_project() const {
	return const_cast<app*>(this)->get_project();
}

level* app::get_level() {
	if(auto project = get_project()) {
		return project->selected_level();
	}
	return nullptr;
}

const level* app::get_level() const {
	return const_cast<app*>(this)->get_level();
}

bool app::has_camera_control() {
	return renderer.camera_control;
}

const char* settings_file_path = "wrench_settings.ini";

void app::read_settings() {
	settings.gui_scale = 1.f;

	if(fs::exists(settings_file_path)) {
		try {
			auto settings_file = toml::parse(settings_file_path);
			
			auto general_table = toml::find(settings_file, "general");
			settings.emulator_path =
				toml::find_or(general_table, "emulator_path", settings.emulator_path);

			auto gui_table = toml::find(settings_file, "gui");
			settings.gui_scale = toml::find_or(gui_table, "scale", 1.f);
			
			auto game_paths = toml::find<std::vector<toml::table>>(settings_file, "game_paths");
			for(auto& game_path : game_paths) {
				auto game_path_value = toml::value(game_path);
				game_iso game;
				game.path = toml::find<std::string>(game_path_value, "path");
				game.game_db_entry = toml::find<std::string>(game_path_value, "game");
				game.md5 = toml::find<std::string>(game_path_value, "md5");
				settings.game_isos.push_back(game);
			}
		} catch(toml::syntax_error& err) {
			emplace_window<gui::message_box>("Failed to parse settings", err.what());
		} catch(std::out_of_range& err) {
			emplace_window<gui::message_box>("Failed to load settings", err.what());
		}
	} else {
		emplace_window<gui::settings>();
	}
}

void app::save_settings() {
	std::vector<toml::value> game_paths_table;
	for(std::size_t i = 0; i < settings.game_isos.size(); i++) {
		auto game = settings.game_isos[i];
		game_paths_table.emplace_back(toml::value {
			{"path", game.path},
			{"game", game.game_db_entry},
			{"md5", game.md5}
		});
	}
	
	toml::value file {
		{"general", {
			{"emulator_path", settings.emulator_path}
		}},
		{"gui", {
			{"scale", settings.gui_scale}
		}},
		{"game_paths", toml::value(game_paths_table)}
	};
	
	std::ofstream settings(settings_file_path);
	settings << toml::format(toml::value(file));
}

void app::run_emulator() {
	if(_project.get() == nullptr) {
		emplace_window<gui::message_box>("Error", "No project open.");
		return;
	}
	
	_project->iso.commit(); // Recompress WAD segments.
	
	if(fs::is_regular_file(settings.emulator_path)) {
		std::string emulator_path = fs::canonical(settings.emulator_path).string();
		std::string cmd = emulator_path + " " + _project->cached_iso_path();
		system(cmd.c_str());
	} else {
		emplace_window<gui::message_box>("Error", "Invalid emulator path.");
	}
}

std::vector<float*> get_imgui_scale_parameters() {
	ImGuiStyle& s = ImGui::GetStyle();
	ImGuiIO& i = ImGui::GetIO();
	return {
		&s.WindowPadding.x,          &s.WindowPadding.y,
		&s.WindowRounding,           &s.WindowBorderSize,
		&s.WindowMinSize.x,          &s.WindowMinSize.y,
		&s.ChildRounding,            &s.ChildBorderSize,
		&s.PopupRounding,            &s.PopupBorderSize,
		&s.FramePadding.x,           &s.FramePadding.y,
		&s.FrameRounding,            &s.FrameBorderSize,
		&s.ItemSpacing.x,            &s.ItemSpacing.y,
		&s.ItemInnerSpacing.x,       &s.ItemInnerSpacing.y,
		&s.TouchExtraPadding.x,      &s.TouchExtraPadding.y,
		&s.IndentSpacing,            &s.ColumnsMinSpacing,
		&s.ScrollbarSize,            &s.ScrollbarRounding,
		&s.GrabMinSize,              &s.GrabRounding,
		&s.TabRounding,              &s.TabBorderSize,
		&s.DisplayWindowPadding.x,   &s.DisplayWindowPadding.y,
		&s.DisplaySafeAreaPadding.x, &s.DisplaySafeAreaPadding.y,
		&s.MouseCursorScale,         &i.FontGlobalScale
	};
}

void app::init_gui_scale() {
	auto parameters = get_imgui_scale_parameters();
	_gui_scale_parameters.resize(parameters.size());
	std::transform(parameters.begin(), parameters.end(), _gui_scale_parameters.begin(),
		[](float* param) { return *param; });
}

void app::update_gui_scale() {
	auto parameters = get_imgui_scale_parameters();
	for(std::size_t i = 0; i < parameters.size(); i++) {
		*parameters[i] = _gui_scale_parameters[i] * settings.gui_scale;
	}
}
