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
		if(levels.find(4) != levels.end()) {
			return levels.at(4).get();
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

void app::open_project(std::string wratch_path) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;

	using worker_type = worker_thread<project_ptr, std::pair<std::string, std::string>>;
	std::pair<std::string, std::string> in(settings.game_paths["rc2pal"], wratch_path);
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
		result.push_back(&_project->views.space_wad);
		result.push_back(&_project->views.armor_wad);
	}
	return result;
}

const char* settings_file_path = "wrench_settings.ini";

void app::read_settings() {
	// Default settings.
	settings.game_paths["rc2pal"] = "";

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
	toml::table general;
	general["emulator_path"] = settings.emulator_path;

	toml::table game_paths;
	for(auto& [game, path] : settings.game_paths) {
		game_paths[game] = path;
	}

	std::ofstream settings(settings_file_path);
	settings << "[general]\n" << toml::format(toml::value(general)) << "\n";
	settings << "[game_paths]\n" << toml::format(toml::value(game_paths)) << "\n";
}

void app::run_emulator() {
	if(boost::filesystem::is_regular_file(settings.emulator_path) && _project.get() != nullptr) {
		std::string emulator_path = boost::filesystem::canonical(settings.emulator_path).string();
		bp::spawn(emulator_path, _project->cached_iso_path());
	} else {
		emplace_window<gui::message_box>("Error", "Invalid emulator path.");
	}
}
