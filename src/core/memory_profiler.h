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

#ifndef CORE_MEMORY_PROFILER
#define CORE_MEMORY_PROFILER

#include <core/util.h>

struct MemoryUsageStatistics
{
	s64 bytes_used = 0;
	s64 max_bytes_used = 0;
	s64 total_allocations = 0;
	s64 total_frees = 0;
};

enum MemoryZoneType
{
	MEMORY_ZONE_ASSET_SYSTEM = 0,
	MAX_MEMORY_ZONE = 1
};

struct MemoryZone
{
	const char* name;
	MemoryUsageStatistics stats;
};

extern MemoryZone g_memory_zones[MAX_MEMORY_ZONE];

void* zone_new(std::size_t size, s32 zone);
void zone_delete(void* pointer, std::size_t size, s32 zone);
void report_memory_statistics();

#define SETUP_MEMORY_ZONE(zone) \
	void* operator new(std::size_t size) { return zone_new(size, zone); } \
	void operator delete(void* pointer, std::size_t size) { zone_delete(pointer, size, zone); }

#endif
