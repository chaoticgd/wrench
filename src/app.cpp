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
#include "level_file_types.h"

using project_ptr = std::unique_ptr<wrench_project>;

void after_directory_loaded(app& a) {
	for(auto& window : a.windows) {
		if(dynamic_cast<gui::start_screen*>(window.get()) != nullptr) {
			window->close(a);
			break;
		}
	}
}

void app::extract_iso(fs::path iso_path, fs::path dir) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	directory = "";
	
	std::pair<std::string, std::string> in(iso_path, dir);
	
	emplace_window<worker_thread<int, decltype(in)>>(
		"Extract ISO", in,
		[](std::pair<std::string, std::string> in, worker_logger& log) {
			std::vector<std::string> args = {"extract", in.first, in.second};
			int exit_code = execute_command("bin/iso", args);
			if(exit_code != 0) {
				log << "\nFailed to extract files from ISO file!\n";
			}
			return exit_code;
		},
		[dir, this](int exit_code) {
			if(exit_code != 0) {
				_lock_project = false;
				return;
			}
			directory = dir;
			_lock_project = false;
			renderer.reset_camera(this);
			auto title = std::string("Wrench Editor - [") + dir.string() + "]";
			glfwSetWindowTitle(glfw_window, title.c_str());
			after_directory_loaded(*this);
		}
	);
}

void app::open_directory(fs::path dir) {
	if(fs::is_directory(dir)) {
		directory = dir;
		after_directory_loaded(*this);
	}
}

void app::build_iso(build_settings settings) {
	emplace_window<worker_thread<int, build_settings>>(
		"Build ISO", settings,
		[](build_settings settings, worker_logger& log) {
			std::vector<std::string> args = {"build", settings.input_dir, settings.output_iso};
			if(settings.single_level) {
				args.push_back("--single-level");
				args.push_back(std::to_string(settings.single_level_index));
			}
			int exit_code = execute_command("bin/iso", args);
			if(exit_code != 0) {
				log << "\nFailed to build ISO file!\n";
			}
			return exit_code;
		},
		[settings](int exit_code) {
			if(exit_code == 0 && settings.launch_emulator) {
				fs::path emu_path = config::get().emulator_path;
				execute_command(emu_path, {settings.output_iso});
			}
		}
	);
}

void app::open_file(fs::path path) {
	file_stream file(path);
	uint32_t magic = file.read<uint32_t>(0x0);
	auto info = LEVEL_FILE_TYPES.find(magic);
	if(info == LEVEL_FILE_TYPES.end()) {
		return;
	}
	
	switch(info->second.type) {
		case level_file_type::LEVEL: {
			level new_lvl;
			try {
				new_lvl.read(file);
			} catch(stream_error& e) {
				printf("error: Failed to load level! %s\n", e.what());
				return;
			}
			_lvl.emplace(std::move(new_lvl));
			renderer.reset_camera(this);
			break;
		}
		case level_file_type::AUDIO:
			break;
		case level_file_type::SCENE:
			break;
	}
}

level* app::get_level() {
	return _lvl ? &(*_lvl) : nullptr;
}

const level* app::get_level() const {
	return _lvl ? &(*_lvl) : nullptr;
}

bool app::has_camera_control() {
	return renderer.camera_control;
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

void config::read() {
	// Default settings
	compression_threads = 8;
	gui_scale = 1.f;
	vsync = true;
	debug.stream_tracing = false;

	if(fs::exists(settings_file_path)) {
		try {
			auto settings_file = toml::parse(settings_file_path);
			
			auto general_table = toml::find_or(settings_file, "general", toml::value());
			emulator_path =
				toml::find_or(general_table, "emulator_path", emulator_path);
			compression_threads = toml::find_or(general_table, "compression_threads", 8);

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
				// Earlier versions of wrench would generate corrupted MD5
				// hashes that were too short.
				if(game.md5.size() == 32) {
					game_isos.push_back(game);
				}
			}
		} catch(toml::syntax_error& err) {
			fprintf(stderr, "Failed to parse settings: %s", err.what());
		} catch(std::out_of_range& err) {
			fprintf(stderr, "Failed to load settings: %s", err.what());
		}
	} else {
		request_open_settings_dialog = true;
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
			{"emulator_path", emulator_path},
			{"compression_threads", compression_threads}
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

gl_texture load_icon(std::string path) {
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
	
	gl_texture texture;
	glGenTextures(1, &texture());
	glBindTexture(GL_TEXTURE_2D, texture());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	return texture;
}
