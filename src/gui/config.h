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

#ifndef WRENCHGUI_CONFIG_H
#define WRENCHGUI_CONFIG_H

#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>
#include <core/util.h>

namespace gui {

struct FolderConfig {
	std::string base_folder;
	std::vector<std::string> mods_folders;
	std::string cache_folder;
};

struct UiConfig {
	bool custom_scale = false;
	f32 scale = 1.f;
	bool developer = true;
};

struct Config {
	FolderConfig folders;
	UiConfig ui;
	
	void read();
	void write();
};

std::string get_config_file_path();
bool config_file_exists();

}

extern gui::Config g_config;

#endif
