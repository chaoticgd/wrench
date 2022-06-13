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

#ifndef WRENCHGUI_COMMANDS_H
#define WRENCHGUI_COMMANDS_H

#include <core/shell.h>

namespace gui {

struct UnpackerParams {
	std::string iso_path;
};

struct PackerParams {
	std::string game_path;
	std::vector<std::string> mod_paths;
	std::string build;
	std::string output_path = "build.iso";
	bool launch_emulator = true;
	bool keep_window_open = false;
	struct {
		bool single_level_enabled = false;
		std::string single_level_tag;
		bool nompegs = false;
	} debug;
};

struct EmulatorParams {
	std::string iso_path;
};

void setup_bin_paths(const char* bin_path);

void run_unpacker(const UnpackerParams& params, CommandStatus* status);
std::string run_packer(const PackerParams& params, CommandStatus* status);
void run_emulator(const EmulatorParams& params, CommandStatus* status);

}

#endif
