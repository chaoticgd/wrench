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

#include "occlusion_grid.h"

std::vector<OcclusionOctant> read_occlusion_grid(Buffer src) {
	ERROR_CONTEXT("reading occlusion grid");
	
	std::vector<OcclusionOctant> octants;
	
	s32 masks_offset = src.read<s32>(0);
	u16 z_coord = src.read<u16>(4);
	u16 z_count = src.read<u16>(6);
	auto z_offsets = src.read_multiple<u16>(8, z_count, "z offsets");
	for(s32 i = 0; i < z_count; i++) {
		if(z_offsets[i] == 0) {
			continue;
		}
		s32 z_offset = z_offsets[i] * 4;
		u16 y_coord = src.read<u16>(z_offset + 0);
		u16 y_count = src.read<u16>(z_offset + 2);
		auto y_offsets = src.read_multiple<u16>(z_offset + 4, y_count, "y offsets");
		for(s32 j = 0; j < y_count; j++) {
			if(y_offsets[j] == 0) {
				continue;
			}
			s32 x_offset = y_offsets[j] * 4;
			u16 x_coord = src.read<u16>(x_offset + 0);
			u16 x_count = src.read<u16>(x_offset + 2);
			auto x_offsets = src.read_multiple<u16>(x_offset + 4, x_count, "x offsets");
			for(s32 k = 0; k < x_count; k++) {
				if(x_offsets[k] == 0xffff) {
					continue;
				}
				
				OcclusionOctant& octant = octants.emplace_back();
				octant.x = x_coord + k;
				octant.y = y_coord + j;
				octant.z = z_coord + i;
				
				auto mask = src.read_multiple<u8>(masks_offset + x_offsets[k] * 128, 128, "octant mask");
				verify_fatal(mask.size() == sizeof(octant.mask));
				memcpy(octant.mask, mask.lo, sizeof(octant.mask));
			}
		}
	}
	
	return octants;
}

void write_occlusion_grid(OutBuffer dest, const std::vector<OcclusionOctant>& octants) {
	ERROR_CONTEXT("writing occlusion grid");
}

std::vector<OcclusionVector> read_occlusion_octants(std::string& str) {
	std::vector<OcclusionVector> octants;
	
	char line[128];
	
	const char* ptr = str.c_str();
	while(*ptr != '\0') {
		OcclusionVector& octant = octants.emplace_back();
		int total_read = sscanf(ptr, "%d %d %d\n", &octant.x, &octant.y, &octant.z);
		verify(octant.x != -1 && octant.y != -1 && octant.z != -1, "Failed to parse octants list.");
		ptr += total_read;
	}
	
	return octants;
}

void write_occlusion_octants(OutBuffer dest, const std::vector<OcclusionVector>& octants) {
	for(const OcclusionVector& octant : octants) {
		dest.writelf("%d %d %d", octant.x, octant.y, octant.z);
	}
}

void swap_occlusion(std::vector<OcclusionOctant>& grid, std::vector<OcclusionVector>& vectors) {
	verify_fatal(grid.size() == vectors.size());
	for(size_t i = 0; i < grid.size(); i++) {
		OcclusionOctant& octant = grid[i];
		OcclusionVector& vector = vectors[i];
		std::swap(octant.x, vector.x);
		std::swap(octant.y, vector.y);
		std::swap(octant.z, vector.z);
	}
}
