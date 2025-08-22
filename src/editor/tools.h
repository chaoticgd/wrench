/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef EDITOR_TOOLS_H
#define EDITOR_TOOLS_H

#include <core/util.h>

typedef void ToolActivateFunc();
typedef void ToolUpdateFunc();
typedef void ToolDrawFunc();
typedef void ToolDeactivateFunc();

struct ToolFuncs
{
	ToolActivateFunc* activate;
	ToolDeactivateFunc* deactivate;
	ToolUpdateFunc* update;
	ToolDrawFunc* draw;
};

struct ToolInfo
{
	const char* name;
	ToolFuncs funcs;
};

extern ToolInfo* g_tools[];
extern s32 g_tool_count;
extern s32 g_active_tool;

#endif
