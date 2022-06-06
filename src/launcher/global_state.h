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

#ifndef LAUNCHER_GLOBAL_STATE_H
#define LAUNCHER_GLOBAL_STATE_H

#include <string>

#include <core/shell.h>
#include <core/stream.h>
#include <toolwads/wads.h>
#include <gui/gui.h>
#include <gui/config.h>

enum class LauncherMode {
	DRAWING_GUI,
	RUNNING_EMULATOR,
	EXIT
};

struct GLFWwindow;

struct BinPaths {
	const char* wrenchbuild;
};

struct LaunchParams {
	std::string emulator_executable;
	std::string iso_path;
};

struct LauncherState {
	LauncherMode mode;
	FileInputStream wad;
	FileInputStream buildwad;
	LauncherWadHeader* header;
	BinPaths bin_paths;
	GLFWwindow* window;
	std::vector<u8> font;
	GlTexture placeholder_image;
	LaunchParams launch_params;
	CommandStatus import_iso_command;
};

extern LauncherState g_launcher;

#endif
