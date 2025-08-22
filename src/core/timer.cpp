/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "timer.h"

#include <chrono>
#include <stdio.h>

#include "util.h"

struct Timer
{
	const char* task;
	std::chrono::high_resolution_clock::time_point start_time;
};
static std::vector<Timer> timers;

void start_timer(const char* task)
{
	for (size_t i = 0; i < timers.size(); i++) {
		printf("  ");
	}
	printf("%s: started\n", task);
	timers.push_back(Timer {task});
	timers.back().start_time = std::chrono::high_resolution_clock::now();
}

void stop_timer()
{
	auto end_time = std::chrono::high_resolution_clock::now();
	Timer timer = timers.back();
	timers.pop_back();
	for (size_t i = 0; i < timers.size(); i++) {
		printf("  ");
	}
	f64 time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - timer.start_time).count() * 1e-6;
	printf("%s: stopped %fs\n", timer.task, time);
}
