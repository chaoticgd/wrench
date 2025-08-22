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

CommandThread::~CommandThread()
{
	stop();
}

void CommandThread::start(const std::vector<std::string>& args)
{
	clear();
	CommandThread* command = this;
	m_thread = std::thread([args, command]() {
		{
			std::lock_guard<std::mutex> lock(command->m_mutex);
			command->m_shared.state = RUNNING;
		}
		std::vector<const char*> pointers(args.size());
		for (size_t i = 0; i < args.size(); i++) {
			pointers[i] = args[i].c_str();
		}
		worker_thread(pointers.size(), pointers.data(), *command);
	});
}

void CommandThread::stop()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_shared.state == NOT_RUNNING) {
			return;
		} else if (m_shared.state != STOPPED) {
			m_shared.state = STOPPING;
		}
	}

	// Wait for the thread to stop processing data.
	for (;;) {
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_shared.state == STOPPED) {
				m_shared.state = NOT_RUNNING;
				break;
			}
		}
		std::this_thread::sleep_for (std::chrono::milliseconds(5));
	}

	// Wait for the thread to terminate.
	m_thread.join();
}

void CommandThread::clear()
{
	stop();
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_shared = {NOT_RUNNING};
	}
}

std::string& CommandThread::get_last_output_lines()
{
	update_last_output_lines();
	return m_buffer;
}

std::string CommandThread::copy_entire_output()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_shared.output;
}

bool CommandThread::is_running()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_shared.state == RUNNING;
}

bool CommandThread::succeeded()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_shared.success;
}

void CommandThread::worker_thread(s32 argc, const char** argv, CommandThread& command)
{
	verify_fatal(argc >= 1);

	// Pass arguments to the shell as enviroment variables.
	std::string command_string = prepare_arguments(argc, argv);

	if (command_string.empty()) {
		std::lock_guard<std::mutex> lock(command.m_mutex);
		command.m_shared.output = "Failed to pass arguments to shell.\n";
		command.m_shared.state = STOPPED;
		command.m_shared.success = false;
		return;
	}

#ifdef __linux__
	// Redirect stderr to stdout so we can capture it.
	command_string += "2>&1";
#endif

	WrenchPipeHandle* pipe = pipe_open(command_string.c_str(), WRENCH_PIPE_MODE_READ);
	if (!pipe) {
		std::lock_guard<std::mutex> lock(command.m_mutex);
		command.m_shared.output += PIPEIO_ERROR_CONTEXT_STRING;
		command.m_shared.output += "\n";
		command.m_shared.state = STOPPED;
		command.m_shared.success = false;
		return;
	}


	// Read data from the pipe until the process has finished or the main thread
	// has requested that we stop.
	char buffer[1024];
	while (pipe_gets(buffer, sizeof(buffer), pipe) != NULL) {
		std::lock_guard<std::mutex> lock(command.m_mutex);
		command.m_shared.output += buffer;
		if (command.m_shared.state == STOPPING) {
			break;
		}
	}

	// Notify the main thread that we're done before we call pclose.
	ThreadState state;
	{
		std::lock_guard<std::mutex> lock(command.m_mutex);
		state = command.m_shared.state;
		if (state == RUNNING) {
			command.m_shared.state = STOPPING;
		}
	}

	int exit_code = pipe_close(pipe);
	{
		std::lock_guard<std::mutex> lock(command.m_mutex);
		command.m_shared.state = STOPPED;
		if (strlen(PIPEIO_ERROR_CONTEXT_STRING) == 0) {
			if (exit_code == 0) {
				command.m_shared.output += "\nProcess exited normally.\n";
				command.m_shared.success = true;
			} else {
				command.m_shared.output += stringf("\nProcess exited with error code %d.\n", exit_code);
				command.m_shared.success = false;
			}
		} else {
			command.m_shared.output += stringf("\nFailed to close pipe (%s).\n", PIPEIO_ERROR_CONTEXT_STRING);
			command.m_shared.success = false;
		}
	}
}

void CommandThread::update_last_output_lines() {
	std::lock_guard<std::mutex> lock(m_mutex);

	m_buffer.resize(0);

	// Find the start of the last 15 lines.
	s64 i = 0;
	s32 new_line_count = 0;
	if (m_shared.output.size() > 0) {
		for (i = (s64)m_shared.output.size() - 1; i > 0; i--) {
			if (m_shared.output[i] == '\n') {
				new_line_count++;
				if (new_line_count >= 15) {
					i++;
					break;
				}
			}
		}
	}

	// Go through the last 15 lines of the output and strip out colour
	// codes, taking care to handle incomplete buffers.
	for (; i < (s64)m_shared.output.size(); i++) {
		// \033[%sm%02x\033[0m
		if (i + 1 < m_shared.output.size() && memcmp(m_shared.output.data() + i, "\033[", 2) == 0) {
			if (i + 3 < m_shared.output.size() && m_shared.output[i + 3] == 'm') {
				i += 3;
			} else {
				i += 4;
			}
		} else {
			m_buffer += m_shared.output[i];
		}
	}
}

s32 execute_command(s32 argc, const char** argv, bool blocking)
{
	verify_fatal(argc >= 1);
	
	std::string command_string = prepare_arguments(argc, argv);
	
	if (!blocking) {
#ifdef _WIN32
		STARTUPINFO startup_info;
		memset(&startup_info, 0, sizeof(STARTUPINFOW));
		
		startup_info.cb = sizeof(STARTUPINFOW);
		startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		startup_info.dwFlags |= STARTF_USESTDHANDLES;
		
		PROCESS_INFORMATION process_info;
		memset(&process_info, 0, sizeof(PROCESS_INFORMATION));
		
		if (CreateProcessA(NULL, (LPSTR) command_string.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info)) {
			CloseHandle(process_info.hProcess);
			CloseHandle(process_info.hThread);
		}
#else
		popen(command_string.c_str(), "r");
#endif
		return 0;
	}

	return system(command_string.c_str());
}

void open_in_file_manager(const char* path)
{
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
	if (!argument.empty() && argument.find_first_of(" \t\n\v\"") == std::string::npos) {
		command.append(argument);
	} else {
		command.push_back('"');
		for (auto iter = argument.begin();; iter++) {
			s32 backslash_count = 0;
		
			while (iter != argument.end() && *iter == '\\') {
				iter++;
				backslash_count++;
			}
		
			if (iter == argument.end()) {
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

static std::string prepare_arguments(s32 argc, const char** argv)
{
	std::string command;
	
#ifdef _WIN32
	for (s32 i = 0; i < argc; i++) {
		command += argv_quote(argv[i]) + " ";
	}
	
	printf("command: %s\n", command.c_str());
#else
	// Pass arguments as enviroment variables.
	for (s32 i = 0; i < argc; i++) {
		std::string env_var = stringf("WRENCH_ARG_%d", i);
		if (setenv(env_var.c_str(), argv[i], 1) == -1) {
			return "";
		}
		command += "\"$" + env_var + "\" ";
		printf("arg: %s\n", argv[i]);
	}
#endif
	
	return command;
}
