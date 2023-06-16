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

#include <stdlib.h>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#define popen _popen
#define pclose _pclose
#define setenv(name, value, overwrite) (_putenv_s(name, value) == 0 ? 0 : -1)
#else
#include <unistd.h>
#endif

#include <platform/pipeio.h>

static std::string prepare_arguments(s32 argc, const char** argv);

CommandThread::~CommandThread() {
	stop();
}

void CommandThread::start(const std::vector<std::string>& args) {
	clear();
	CommandThread* command = this;
	thread = std::thread([args, command]() {
		{
			std::lock_guard<std::mutex> lock(command->mutex);
			command->shared.state = RUNNING;
		}
		std::vector<const char*> pointers(args.size());
		for(size_t i = 0; i < args.size(); i++) {
			pointers[i] = args[i].c_str();
		}
		worker_thread(pointers.size(), pointers.data(), *command);
	});
}

void CommandThread::stop() {
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(shared.state == NOT_RUNNING) {
			return;
		} else if(shared.state != STOPPED) {
			shared.state = STOPPING;
		}
	}

	// Wait for the thread to stop processing data.
	for(;;) {
		{
			std::lock_guard<std::mutex> lock(mutex);
			if(shared.state == STOPPED) {
				shared.state = NOT_RUNNING;
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	// Wait for the thread to terminate.
	thread.join();
}

void CommandThread::clear() {
	stop();
	{
		std::lock_guard<std::mutex> lock(mutex);
		shared = {NOT_RUNNING};
	}
}

std::string& CommandThread::get_last_output_lines() {
	update_last_output_lines();
	return buffer;
}

std::string CommandThread::copy_entire_output() {
	std::lock_guard<std::mutex> lock(mutex);
	return shared.output;
}

bool CommandThread::is_running() {
	std::lock_guard<std::mutex> lock(mutex);
	return shared.state == RUNNING;
}

bool CommandThread::succeeded() {
	std::lock_guard<std::mutex> lock(mutex);
	return shared.success;
}

void CommandThread::worker_thread(s32 argc, const char** argv, CommandThread& command) {
	verify_fatal(argc >= 1);

	// Pass arguments to the shell as enviroment variables.
	std::string command_string = prepare_arguments(argc, argv);

	if(command_string.empty()) {
		std::lock_guard<std::mutex> lock(command.mutex);
		command.shared.output = "Failed to pass arguments to shell.\n";
		command.shared.state = STOPPED;
		command.shared.success = false;
		return;
	}

#ifdef __linux__
	// Redirect stderr to stdout so we can capture it.
	command_string += "2>&1";
#endif

	WrenchPipeHandle* pipe = pipe_open(command_string.c_str(), WRENCH_PIPE_MODE_READ);
	if (!pipe) {
		std::lock_guard<std::mutex> lock(command.mutex);
		command.shared.output += std::string("pipe_open failed: ") + PIPEIO_ERROR_CONTEXT_STRING + "\n";
		command.shared.state = STOPPED;
		command.shared.success = false;
		return;
	}


	// Read data from the pipe until the process has finished or the main thread
	// has requested that we stop.
	char buffer[1024];
	while(pipe_gets(buffer, sizeof(buffer), pipe) != NULL) {
		std::lock_guard<std::mutex> lock(command.mutex);
		command.shared.output += buffer;
		if(command.shared.state == STOPPING) {
			break;
		}
	}

	// Notify the main thread that we're done before we call pclose.
	ThreadState state;
	{
		std::lock_guard<std::mutex> lock(command.mutex);
		state = command.shared.state;
		if(state == RUNNING) {
			command.shared.state = STOPPING;
		}
	}

	int exit_code = pipe_close(pipe);
	{
		std::lock_guard<std::mutex> lock(command.mutex);
		command.shared.state = STOPPED;
		if(exit_code == 0) {
			command.shared.output += "\nProcess exited normally.\n";
			command.shared.success = true;
		} else {
			command.shared.output += stringf("\nProcess exited with error code %d.\nError message upon pipe closing: %s\n", exit_code, PIPEIO_ERROR_CONTEXT_STRING);
			command.shared.success = false;
		}
	}
}

void CommandThread::update_last_output_lines() {
	std::lock_guard<std::mutex> lock(mutex);

	buffer.resize(0);

	// Find the start of the last 15 lines.
	s64 i = 0;
	s32 new_line_count = 0;
	if(shared.output.size() > 0) {
		for(i = (s64)shared.output.size() - 1; i > 0; i--) {
			if(shared.output[i] == '\n') {
				new_line_count++;
				if(new_line_count >= 15) {
					i++;
					break;
				}
			}
		}
	}

	// Go through the last 15 lines of the output and strip out colour
	// codes, taking care to handle incomplete buffers.
	for(; i < (s64)shared.output.size(); i++) {
		// \033[%sm%02x\033[0m
		if(i + 1 < shared.output.size() && memcmp(shared.output.data() + i, "\033[", 2) == 0) {
			if(i + 3 < shared.output.size() && shared.output[i + 3] == 'm') {
				i += 3;
			} else {
				i += 4;
			}
		} else {
			buffer += shared.output[i];
		}
	}
}

s32 execute_command(s32 argc, const char** argv, bool blocking) {
	verify_fatal(argc >= 1);

	std::string command_string = prepare_arguments(argc, argv);

	if(!blocking) {
		popen(command_string.c_str(), "r");
		return 0;
	}

	return system(command_string.c_str());
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

// https://web.archive.org/web/20190109172835/https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
static std::string argv_quote(const std::string& argument) {
	std::string command;
	if(!argument.empty() && argument.find_first_of(" \t\n\v\"") == std::string::npos) {
		command.append(argument);
	} else {
		command.push_back('"');
		for(auto iter = argument.begin();; iter++) {
			s32 backslash_count = 0;
		
			while (iter != argument.end() && *iter == '\\') {
				iter++;
				backslash_count++;
			}
		
			if(iter == argument.end()) {
				command.append(backslash_count * 2, '\\');
				break;
			} else if (*iter == '"') {
				command.append(backslash_count * 2 + 1, '\\');
				command.push_back(*iter);
			} else {
				command.append(backslash_count, '\\');
				command.push_back(*iter);
			}
		}
		command.push_back('"');
	}
	return command;
}

static std::string prepare_arguments(s32 argc, const char** argv) {
	std::string command;
	
#ifdef _WIN32
	for(s32 i = 0; i < argc; i++) {
		command += argv_quote(argv[i]) + " ";
	}
	
	printf("command: %s\n", command.c_str());
#else
	// Pass arguments as enviroment variables.
	for(s32 i = 0; i < argc; i++) {
		std::string env_var = stringf("WRENCH_ARG_%d", i);
		if(setenv(env_var.c_str(), argv[i], 1) == -1) {
			return "";
		}
		command += "\"$" + env_var + "\" ";
		printf("arg: %s\n", argv[i]);
	}
#endif
	
	return command;
}
