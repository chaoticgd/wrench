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

#ifndef CORE_SHELL_H
#define CORE_SHELL_H

#include <mutex>
#include <core/util.h>

struct CommandOutput {
	std::mutex mutex;
	std::string string;
	s32 exit_code;
	bool finished = false;
};

s32 execute_command(s32 argc, const char** argv, CommandOutput* output = nullptr);
void open_in_file_manager(const char* path);

#endif
