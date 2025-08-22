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
#include <core/texture.h>
#include <toolwads/wads.h>
#include <gui/gui.h>
#include <gui/config.h>
#include <gui/commands.h>

enum class LauncherMode
{
	DRAWING_GUI,
	RUNNING_EMULATOR,
	EXIT
};

struct GLFWwindow;

struct LauncherState
{
	LauncherMode mode;
	FileInputStream wad;
	FileInputStream buildwad;
	GLFWwindow* window;
	ImFont* font_regular;
	ImFont* font_italic;
	Texture logo;
	gui::EmulatorParams emulator_params;
};

extern LauncherState g_launcher;

#endif
