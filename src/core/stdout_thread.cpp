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

#include "stdout_thread.h"

#include <mutex>
#include <thread>
#include <stdio.h>

static std::mutex stdout_flush_mutex;
static bool stdout_flush_started = false;
static bool stdout_flush_finished = false;
static std::thread stdout_flush_thread;

void start_stdout_flusher_thread()
{
	if (!stdout_flush_started) {
		stdout_flush_started = true;
		stdout_flush_thread = std::thread([]() {
			for (;;) {
				{
					std::lock_guard<std::mutex> g(stdout_flush_mutex);
					if (stdout_flush_finished) {
						break;
					}
				}
				fflush(stdout);
				fflush(stderr);
				std::this_thread::sleep_for (std::chrono::milliseconds(100));
			}
		});
	}
}

void stop_stdout_flusher_thread()
{
	if (stdout_flush_started) {
		{
			std::lock_guard<std::mutex> g(stdout_flush_mutex);
			stdout_flush_finished = true;
		}
		stdout_flush_thread.join();
	}
}
