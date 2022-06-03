/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "config.h"

#include <core/filesystem.h>

void gui::Config::read() {
	
}

void gui::Config::write() {
	
}

std::string gui::get_config_file_path() {
	// TODO: Windows support
	const char* home = getenv("HOME");
	verify(home, "HOME enviroment variable not set!");
	return std::string(home) + "/.config/wrench.cfg";
}

bool gui::config_file_exists() {
	return fs::exists(get_config_file_path());
}

gui::Config g_config = {};
