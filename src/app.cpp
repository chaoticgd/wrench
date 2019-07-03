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

#include "gui.h"
#include "stream.h"
#include "renderer.h"
#include "worker_thread.h"
#include "formats/level_data.h"
#include "formats/texture.h"

bool app::has_level() const {
	return _level.get() != nullptr;
}

void app::if_level(std::function<void(level&)> callback) {
	if(has_level()) {
		callback(*_level.get());
	}
}

void app::if_level(std::function<void(const level_impl&)> callback) const {
	if(has_level()) {
		callback(*_level.get());
	}
}

void app::if_level(std::function<void(level&, const level_impl&)> callback) {
	if(has_level()) {
		callback(*_level.get(), *_level.get());
	}
}

void app::import_level(std::string path) {
	// Decompression takes a long time so we spin off another thread.
	using worker_type = worker_thread<std::unique_ptr<level_impl>, std::string>;
	windows.emplace_back(std::make_unique<worker_type>(
		"Level Importer", path,
		[](std::string path, worker_logger& log) {
			try {
				file_stream stream(path);
				texture_provider tex(&stream);
				tex.textures();
				return std::optional(level_data::import_level(stream, log));
			} catch(stream_error& e) {
				log << "stream_error: " << e.what() << "\n";
				log << e.stack_trace;
			}
			return std::optional<std::unique_ptr<level_impl>>();
		},
		[=](std::unique_ptr<level_impl> lvl) {
			_level.swap(lvl);
			if(auto view = get_3d_view()) {
				static_cast<three_d_view*>(*view)->reset_camera(*this);
			}
		}
	));
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
