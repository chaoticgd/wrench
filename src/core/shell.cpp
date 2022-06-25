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

#include "shell.h"

#ifdef _WIN32
	#include <shellapi.h>
#else
	#include <unistd.h>
#endif

void spawn_command_thread(const std::vector<std::string>& args, CommandStatus* output) {
	output->running = true;
	{
		std::lock_guard<std::mutex>(output->mutex);
		output->finished = false;
	}
	if(output->thread.joinable()) {
		output->thread.join();
	}
	output->thread = std::thread([args, output]() {
		std::vector<const char*> pointers(args.size());
		for(size_t i = 0; i < args.size(); i++) {
			pointers[i] = args[i].c_str();
		}
		execute_command(pointers.size(), pointers.data(), output);
	});
}

s32 execute_command(s32 argc, const char** argv, CommandStatus* output, bool blocking) {
	assert(argc >= 1);
	
	std::string command;
	
	// Pass arguments as enviroment variables.
	for(s32 i = 0; i < argc; i++) {
		printf("arg: %s\n", argv[i]);
		std::string env_var = stringf("WRENCH_ARG_%d", i);
		if(setenv(env_var.c_str(), argv[i], 1) == -1) {
			return 1;
		}
		command += "\"$" + env_var + "\" ";
	}
	
	if(!blocking) {
		popen(command.c_str(), "r");
		return 0;
	} else if(output) {
		// Redirect stderr to stdout so we can capture it.
		command += "2>&1";
		
		FILE* pipe = popen(command.c_str(), "r");
		if(!pipe) {
			perror("popen() failed");
			std::lock_guard<std::mutex> guard(output->mutex);
			output->output += std::string("popen() failed: ") + strerror(errno) + "\n";
		}
		
		char buffer[16];
		while(fgets(buffer, sizeof(buffer), pipe) != NULL) {
			if(output) {
				std::lock_guard<std::mutex> guard(output->mutex);
				output->output += buffer;
			}
		}
		
		s32 exit_code = pclose(pipe);
		{
			std::lock_guard<std::mutex> guard(output->mutex);
			if(exit_code == 0) {
				output->output += "\nProcess exited normally.\n";
			} else {
				output->output += stringf("\nProcess exited with error code %d.\n", exit_code);
			}
			output->exit_code = exit_code;
			output->finished = true;
		}
		return exit_code;
	} else {
		return system(command.c_str());
	}
}

void open_in_file_manager(const char* path) {
#ifdef _WIN32
	ShellExecuteA(nullptr, "open", path, nullptr, nullptr, SW_SHOWDEFAULT);
#else
	setenv("WRENCH_ARG_0", "xdg-open", 1);
	setenv("WRENCH_ARG_1", path, 1);
	system("\"$WRENCH_ARG_0\" \"$WRENCH_ARG_1\"");
#endif
}
