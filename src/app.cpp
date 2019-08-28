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

#include <toml11/toml.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>

#include "gui.h"
#include "inspector.h"
#include "stream.h"
#include "renderer.h"
#include "worker_thread.h"

namespace bp = boost::process;

app::app()
	: mouse_last(0, 0),
	  mouse_diff(0, 0),
	  this_any(this),
	  _lock_project(false) {
	
	read_settings();
}

level* app::get_level() {
	if(_project) {
		auto& levels = _project->views.levels;
		if(levels.find("LEVEL4.WAD") != levels.end()) {
			return levels.at("LEVEL4.WAD").get();
		}
	}
	return nullptr;
}

using project_ptr = std::unique_ptr<wrench_project>;

void app::new_project() {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	_project.reset(nullptr);

	using worker_type = worker_thread<project_ptr, std::string>;
	windows.emplace_back(std::make_unique<worker_type>(
		"New Project", settings.game_paths["rc2pal"],
		[](std::string path, worker_logger& log) {
			try {
				auto result = std::make_unique<wrench_project>(path, log, "rc2pal");
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

			if(auto view = get_3d_view()) {
				(*view)->reset_camera(*this);
			}
		}
	));
}

void app::open_project(std::string path) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	_project.reset(nullptr);

	using worker_type = worker_thread<project_ptr, std::pair<std::string, std::string>>;
	std::pair<std::string, std::string> in(settings.game_paths["rc2pal"], path);
	windows.emplace_back(std::make_unique<worker_type>(
		"Open Project", in,
		[](std::pair<std::string, std::string> paths, worker_logger& log) {
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

			if(auto view = get_3d_view()) {
				(*view)->reset_camera(*this);
			}
		}
	));
}

void app::save_project(bool save_as) {
	if(_project.get() == nullptr) {
		return;
	}

	try {
		if(save_as) {
			_project->save_as(this);
		} else {
			_project->save(this);
		}
	} catch(stream_error& err) {
		std::stringstream error_message;
		error_message << err.what() << "\n";
		error_message << err.stack_trace;
		emplace_window<gui::message_box>("Error Saving Project", error_message.str());
	}
}

const level* app::get_level() const {
	return const_cast<app*>(this)->get_level();
}

bool app::has_camera_control() {
	auto view = get_3d_view();
	if(!view) {
		return false;
	}
	return static_cast<three_d_view*>(*view)->camera_control;
}

std::optional<three_d_view*> app::get_3d_view() {
	for(auto& window : windows) {
		if(dynamic_cast<three_d_view*>(window.get()) != nullptr) {
			return dynamic_cast<three_d_view*>(window.get());
		}
	}
	return {};
}

std::vector<texture_provider*> app::texture_providers() {
	std::vector<texture_provider*> result;
	if(_project) {
		for(auto& level : _project->views.levels) {
			result.push_back(level.second->get_texture_provider());
		}
		for(auto& scanner : _project->views.texture_wads) {
			result.push_back(scanner.get());
		}
	}
	return result;
}

const char* settings_file_path = "wrench_settings.ini";

void app::read_settings() {
	// Default settings.
	settings.game_paths["rc2pal"] = "";
	settings.gui_scale = 1.f;

	if(boost::filesystem::exists(settings_file_path)) {
		try {
			const auto settings_file = toml::parse(settings_file_path);

			const auto general_tbl = toml::find(settings_file, "general");
			settings.emulator_path =
				toml::find_or(general_tbl, "emulator_path", settings.emulator_path);

			const auto game_paths_tbl = toml::find(settings_file, "game_paths");
			for(auto& [game, path] : settings.game_paths) {
				path = toml::find_or(game_paths_tbl, game.c_str(), "");
			}

			const auto gui_tbl = toml::find(settings_file, "gui");
			settings.gui_scale = toml::find_or(gui_tbl, "scale", 1.f);
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
	toml::table genera_tbl;
	genera_tbl["emulator_path"] = settings.emulator_path;

	toml::table game_paths_tbl;
	for(auto& [game, path] : settings.game_paths) {
		game_paths_tbl[game] = path;
	}

	toml::table gui_tbl;
	gui_tbl["scale"] = settings.gui_scale;

	std::ofstream settings(settings_file_path);
	settings << "[general]\n" << toml::format(toml::value(genera_tbl)) << "\n";
	settings << "[game_paths]\n" << toml::format(toml::value(game_paths_tbl)) << "\n";
	settings << "[gui]\n" << toml::format(toml::value(gui_tbl));
}

void app::run_emulator() {
	if(boost::filesystem::is_regular_file(settings.emulator_path) && _project.get() != nullptr) {
		std::string emulator_path = boost::filesystem::canonical(settings.emulator_path).string();
		bp::spawn(emulator_path, _project->cached_iso_path());
	} else {
		emplace_window<gui::message_box>("Error", "Invalid emulator path.");
	}
}

std::vector<float*> get_imgui_scale_parameters() {
	ImGuiStyle& s = ImGui::GetStyle();
	ImGuiIO& i = ImGui::GetIO();
	return {
		&s.WindowPadding.x,
		&s.WindowPadding.y,
		&s.WindowRounding,
		&s.WindowBorderSize,
		&s.WindowMinSize.x,
		&s.WindowMinSize.y,
		&s.ChildRounding,
		&s.ChildBorderSize,
		&s.PopupRounding,
		&s.PopupBorderSize,
		&s.FramePadding.x,
		&s.FramePadding.y,
		&s.FrameRounding,
		&s.FrameBorderSize,
		&s.ItemSpacing.x,
		&s.ItemSpacing.y,
		&s.ItemInnerSpacing.x,
		&s.ItemInnerSpacing.y,
		&s.TouchExtraPadding.x,
		&s.TouchExtraPadding.y,
		&s.IndentSpacing,
		&s.ColumnsMinSpacing,
		&s.ScrollbarSize,
		&s.ScrollbarRounding,
		&s.GrabMinSize,
		&s.GrabRounding,
		&s.TabRounding,
		&s.TabBorderSize,
		&s.DisplayWindowPadding.x,
		&s.DisplayWindowPadding.y,
		&s.DisplaySafeAreaPadding.x,
		&s.DisplaySafeAreaPadding.y,
		&s.MouseCursorScale,
		&i.FontGlobalScale
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
