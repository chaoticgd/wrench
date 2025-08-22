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

#include "memory_profiler.h"

MemoryZone g_memory_zones[MAX_MEMORY_ZONE] = {
	{"asset system"}
};

void* zone_new(std::size_t size, s32 zone)
{
	MemoryUsageStatistics& stats = g_memory_zones[zone].stats;
	stats.bytes_used += size;
	stats.max_bytes_used = std::max(stats.bytes_used, stats.max_bytes_used);
	stats.total_allocations++;
	return ::operator new(size);
}

void zone_delete(void* pointer, std::size_t size, s32 zone)
{
	MemoryUsageStatistics& stats = g_memory_zones[zone].stats;
	stats.bytes_used -= size;
	stats.total_frees++;
	return ::operator delete(pointer);
}

void report_memory_statistics()
{
	printf("\033[34m");
	for (const MemoryZone& zone : g_memory_zones) {
		printf("%s: %ldk used, %ld allocations, %ld frees, %ld leaked\n",
			zone.name, zone.stats.max_bytes_used / 1024,
			zone.stats.total_allocations, zone.stats.total_frees,
			zone.stats.total_allocations - zone.stats.total_frees);
	}
	printf("\033[0m");
}
