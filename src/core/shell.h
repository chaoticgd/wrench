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

class CommandThread
{
	friend s32 execute_command(s32 argc, const char** argv, bool blocking);
public:
	~CommandThread();
	
	void start(const std::vector<std::string>& args); // Run a command.
	void stop(); // Interrupt the thread.
	void clear(); // Free memory.
	
	std::string& get_last_output_lines();
	std::string copy_entire_output();
	
	bool is_running();
	bool succeeded();
	
private:
	static void worker_thread(s32 argc, const char** argv, CommandThread& command);
	void update_last_output_lines();

	enum ThreadState { // condition                                  description
		NOT_RUNNING,   // initial state, main thread sees STOPPED    the worker is not running (hasn't yet been spawned, or has been joined)
		RUNNING,       // start() called on main thread              the worker is running the command
		STOPPING,      // stop() called on main thread               the main thread has requested the worker stop
		STOPPED,       // worker sees STOPPING or is finished        the worker has stopped, main thread needs to acknowledge
	};
	
	struct SharedData
	{
		ThreadState state = NOT_RUNNING;
		std::string output;
		bool success = false;
	};
	
	std::string m_buffer;
	std::thread m_thread;
	std::mutex m_mutex;
	SharedData m_shared;
};

s32 execute_command(s32 argc, const char** argv, bool blocking = true);
void open_in_file_manager(const char* path);

#endif
