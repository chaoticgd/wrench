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

void after_directory_loaded(app& a) {
	for(auto& window : a.windows) {
		if(dynamic_cast<gui::start_screen*>(window.get()) != nullptr) {
			window->close(a);
			break;
		}
	}
}

#ifdef _WIN32
	const char* ISO_UTILITY_PATH = ".\\bin\\iso";
#else
	const char* ISO_UTILITY_PATH = "./bin/iso";
#endif

void app::extract_iso(fs::path iso_path, fs::path dir) {
	if(_lock_project) {
		return;
	}

	_lock_project = true;
	directory = "";
	
	std::pair<std::string, std::string> in(iso_path.string(), dir.string());
	
	emplace_window<worker_thread<int, decltype(in)>>(
		"Extract ISO", in,
		[](std::pair<std::string, std::string> in, worker_logger& log) {
			std::vector<std::string> args = {"extract", in.first, in.second};
			int exit_code = execute_command(ISO_UTILITY_PATH, args);
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
			std::vector<std::string> args = {
				"build",
				settings.input_dir.string(),
				settings.output_iso.string()
			};
			if(settings.single_level) {
				args.push_back("--single-level");
				args.push_back(std::to_string(settings.single_level_index));
			}
			if(settings.no_mpegs) {
				args.push_back("--no-mpegs");
			}
			int exit_code = execute_command(ISO_UTILITY_PATH, args);
			if(exit_code != 0) {
				log << "\nFailed to build ISO file!\n";
			}
			return exit_code;
		},
		[settings](int exit_code) {
			if(exit_code == 0 && settings.launch_emulator) {
				fs::path emu_path = config::get().emulator_path;
				execute_command(emu_path.string(), {settings.output_iso.string()});
			}
		}
	);
}

void app::open_file(fs::path path) {
	file_stream file(path.string());
	
	uint32_t magic = file.read<uint32_t>(0x0);
	auto info = LEVEL_FILE_TYPES.find(magic);
	if(info != LEVEL_FILE_TYPES.end()) {
		switch(info->second.type) {
			case level_file_type::LEVEL: {
				level new_lvl;
				try {
					new_lvl.read(file, path);
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
		return;
	}
	
	armor_archive armor;
	if(armor.read(file)) {
		_armor.emplace(std::move(armor));
		return;
	}
}

void app::save_level() {
	level* lvl = get_level();
	
	array_stream dest;
	lvl->write(dest);
	
	file_stream file(lvl->path.string(), std::ios::out);
	file.write_n(dest.buffer.data(), dest.buffer.size());
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

std::map<std::string, std::vector<texture>*> app::texture_lists() {
	std::map<std::string, std::vector<texture>*> result;
	if(auto* lvl = get_level()) {
		auto name = lvl->path.filename().string();
		result[name + "/Mipmaps"] = &lvl->mipmap_textures;
		result[name + "/Tfrags"] = &lvl->tfrag_textures;
		result[name + (renderer.flag ? "/Mobys" : "/Mobies")] = &lvl->moby_textures;
		result[name + "/Ties"] = &lvl->tie_textures;
		result[name + "/Shrubs"] = &lvl->shrub_textures;
		result[name + "/Sprites"] = &lvl->sprite_textures;
		result[name + "/Loading Screen"] = &lvl->loading_screen_textures;
	}
	if(_armor) {
		result["Armour"] = &_armor->textures;
	}
	return result;
}

std::map<std::string, model_list> app::model_lists() {
	std::map<std::string, model_list> result;
	if(auto* lvl = get_level()) {
		auto name = lvl->path.filename().string();
		result[name + (renderer.flag ? "/Mobys" : "/Mobies")] = { &lvl->moby_models, &lvl->moby_textures };
	}
	if(_armor) {
		result["Armour"] = { &_armor->models, &_armor->textures };
	}
	return result;
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
		}}
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
