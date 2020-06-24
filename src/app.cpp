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
	config::get().read(*this);
}

using project_ptr = std::unique_ptr<wrench_project>;

void app::new_project(game_iso game) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	_project.reset(nullptr);

	emplace_window<worker_thread<project_ptr, game_iso>>(
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
		[&](project_ptr project) {
			project->post_load();
			_project.swap(project);
			_lock_project = false;

			renderer.reset_camera(this);
			
			glfwSetWindowTitle(glfw_window, "Wrench Editor - [Unsaved Project]");
		}
	);
}

void app::open_project(std::string path) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	_project.reset(nullptr);

	struct open_project_input {
		std::vector<game_iso> game_isos;
		std::string path;
	};

	auto in = open_project_input { config::get().game_isos, path };
	emplace_window<worker_thread<project_ptr, open_project_input>>(
		"Open Project", in,
		[](auto in, worker_logger& log) {
			try {
				auto result = std::make_unique<wrench_project>(in.game_isos, in.path, log);
				log << "\nProject opened successfully.";
				return std::make_optional(std::move(result));
			} catch(stream_error& err) {
				log << err.what() << "\n";
				log << err.stack_trace;
			}
			return std::optional<project_ptr>();
		},
		[&](project_ptr project) {
			project->post_load();
			_project.swap(project);
			_lock_project = false;

			renderer.reset_camera(this);
			
			auto title = std::string("Wrench Editor - [") + path + "]";
			glfwSetWindowTitle(glfw_window, title.c_str());
		}
	);
}

void app::save_project(bool save_as) {
	if(_project.get() == nullptr) {
		return;
	}

	auto on_done = [&]() {
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

void app::run_emulator() {
	if(_project.get() == nullptr) {
		emplace_window<gui::message_box>("Error", "No project open.");
		return;
	}
	
	_project->iso.commit(); // Recompress WAD segments.
	
	if(fs::is_regular_file(config::get().emulator_path)) {
		std::string emulator_path = fs::canonical(config::get().emulator_path).string();
		std::string cmd = emulator_path + " " + _project->cached_iso_path();
		int result = system(cmd.c_str());
		if(result != 0) {
			emplace_window<gui::message_box>("Error", "Failed to execute shell command.");
		}
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
		*parameters[i] = _gui_scale_parameters[i] * config::get().gui_scale;
	}
}

config& config::get() {
	static config instance;
	return instance;
}

const char* settings_file_path = "wrench_settings.ini";

void config::read(app& a) {
	// Default settings
	gui_scale = 1.f;
	vsync = true;
	debug.stream_tracing = false;

	if(fs::exists(settings_file_path)) {
		try {
			auto settings_file = toml::parse(settings_file_path);
			
			auto general_table = toml::find_or(settings_file, "general", toml::value());
			emulator_path =
				toml::find_or(general_table, "emulator_path", emulator_path);

			auto gui_table = toml::find_or(settings_file, "gui", toml::value());
			gui_scale = toml::find_or(gui_table, "scale", 1.f);
			vsync = toml::find_or(gui_table, "vsync", true);
			
			auto debug_table = toml::find_or(settings_file, "debug", toml::value());
			debug.stream_tracing = toml::find_or(debug_table, "stream_tracing", false);
			
			auto game_paths = toml::find_or<std::vector<toml::table>>(settings_file, "game_paths", {});
			for(auto& game_path : game_paths) {
				auto game_path_value = toml::value(game_path);
				game_iso game;
				game.path = toml::find<std::string>(game_path_value, "path");
				game.game_db_entry = toml::find<std::string>(game_path_value, "game");
				game.md5 = toml::find<std::string>(game_path_value, "md5");
				game_isos.push_back(game);
			}
		} catch(toml::syntax_error& err) {
			a.emplace_window<gui::message_box>("Failed to parse settings", err.what());
		} catch(std::out_of_range& err) {
			a.emplace_window<gui::message_box>("Failed to load settings", err.what());
		}
	} else {
		a.emplace_window<gui::settings>();
	}
}

void config::write() {
	std::vector<toml::value> game_paths_table;
	for(std::size_t i = 0; i < game_isos.size(); i++) {
		auto game = game_isos[i];
		game_paths_table.emplace_back(toml::value {
			{"path", game.path},
			{"game", game.game_db_entry},
			{"md5", game.md5}
		});
	}
	
	toml::value file {
		{"general", {
			{"emulator_path", emulator_path}
		}},
		{"gui", {
			{"scale", gui_scale},
			{"vsync", vsync}
		}},
		{"debug", {
			{"stream_tracing", debug.stream_tracing}
		}},
		{"game_paths", toml::value(game_paths_table)}
	};
	
	std::ofstream settings(settings_file_path);
	settings << toml::format(toml::value(file));
}
