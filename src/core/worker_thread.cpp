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

#include "worker_thread.h"

WorkerThread::~WorkerThread() {
	interrupt();
}


void WorkerThread::start() {
	clear();
	state = STARTING;
	WorkerThread* command = this;
	thread = std::thread([command]() {
		{
			std::lock_guard<std::mutex> lock(command->mutex);
			command->state = RUNNING;
		}
		command->run();
	});
}

void WorkerThread::interrupt() {
	{
		std::lock_guard<std::mutex> lock(mutex);
		if(state == NOT_RUNNING) {
			return;
		} else if(state != STOPPED) {
			state = STOPPING;
		}
	}

	// Wait for the thread to stop processing data.
	for(;;) {
		{
			std::lock_guard<std::mutex> lock(mutex);
			if(state == STOPPED) {
				state = NOT_RUNNING;
				break;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	// Wait for the thread to terminate.
	thread.join();
}

bool WorkerThread::is_running() {
	std::lock_guard<std::mutex> lock(mutex);
	return state == STARTING || state == RUNNING;
}

const char* WorkerThread::state_string() {
	std::lock_guard<std::mutex> lock(mutex);
	switch(state) {
		case NOT_RUNNING: return "Not Running";
		case STARTING: return "Starting";
		case RUNNING: return "Running";
		case STOPPING: return "Stopping";
		case STOPPED: return "Stopped";
		default: {}
	}
	return "Error";
}

bool WorkerThread::check_if_interrupted(std::function<void()>&& callback) {
	std::lock_guard<std::mutex> lock(mutex);
	callback();
	return state == STOPPING;
}

void WorkerThread::transition_to_stopping() {
	std::lock_guard<std::mutex> lock(mutex);
	state = STOPPED;
}

void WorkerThread::transition_to_stopped(std::function<void()>&& callback) {
	std::lock_guard<std::mutex> lock(mutex);
	state = STOPPED;
	callback();
}
