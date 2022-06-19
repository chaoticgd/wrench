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
#include <thread>
#include <core/util.h>

struct CommandStatus {
	std::string buffer;
	std::thread thread;
	std::mutex mutex;
	// Shared data.
	std::string output;
	s32 exit_code;
	bool finished = false;
};

void spawn_command_thread(const std::vector<std::string>& args, CommandStatus* output);
s32 execute_command(s32 argc, const char** argv, CommandStatus* output = nullptr);
void execute_command_non_blocking(const char** args); // Last element of args should be nullptr.
void open_in_file_manager(const char* path);

#endif
