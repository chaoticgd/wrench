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
#include "worker_thread.h"
#include "formats/level_data.h"

bool app::has_level() const {
	return _level.get() != nullptr;
}

bool app::has_camera_control() const {
	bool result = false;
	if_level([&result](const level_impl& lvl) {
		result = lvl.camera_control;
	});
	return result;
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
				return std::optional(level_data::import_level(stream, log));
			} catch(stream_error& e) {
				log << "stream_error: " << e.what() << "\n";
				log << e.stack_trace;
			}
			return std::optional<std::unique_ptr<level_impl>>();
		},
		[=](std::unique_ptr<level_impl> lvl) {
			_level.swap(lvl);
			_level->reset_camera();
		}
	));
}
