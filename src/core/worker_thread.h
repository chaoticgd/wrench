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

#ifndef CORE_WORKER_THREAD_H
#define CORE_WORKER_THREAD_H

#include <mutex>
#include <thread>
#include <functional>
#include <core/util.h>

class WorkerThread {
public:
	virtual ~WorkerThread();
	
	void start();
	void interrupt();
	
	bool is_running();
	const char* state_string();
	
	std::mutex mutex;
	
protected:
	virtual void run() = 0;
	virtual void clear() = 0;
	
	bool check_if_interrupted(std::function<void()>&& callback);
	void transition_to_stopping();
	void transition_to_stopped(std::function<void()>&& callback);
	
	enum ThreadState { // condition                                  description
		NOT_RUNNING,   // initial state, main thread sees STOPPED    the worker is not running (hasn't yet been spawned, or has been joined)
		STARTING,      // start() called on main thread              the worker thread hasn't started yet
		RUNNING,       // worker has started                         the worker is running the command
		STOPPING,      // stop() called on main thread               the main thread has requested the worker stop
		STOPPED,       // worker sees STOPPING or is finished        the worker has stopped, main thread needs to acknowledge
	};
	
	std::string buffer;
	std::thread thread;
	ThreadState state = NOT_RUNNING;
};

#endif
