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

#include <boost/stacktrace.hpp>

#include "gui.h"
#include "stream.h"
#include "formats/level_data.h"

bool app::has_level() const {
	return _level.get() != nullptr;
}

level& app::get_level() {
	return *_level.get();
}

const level_impl& app::read_level() const {
	return *_level.get();
}

void app::import_level(std::string path) {
	try {
		file_stream stream(path);
		auto lvl = ::import_level(stream);
		_level.swap(lvl);
		_level->reset_camera();
	} catch(stream_error& e) {
		std::stringstream message;
		message << "stream_error: " << e.what() << "\n";
		message << boost::stacktrace::stacktrace();
		auto error_box = std::make_unique<gui::message_box>
			("Level Import Failed", message.str());
		tools.emplace_back(std::move(error_box));
	}
}
