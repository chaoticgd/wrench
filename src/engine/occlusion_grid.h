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

#ifndef ENGINE_OCCLUSION_GRID_H
#define ENGINE_OCCLUSION_GRID_H

#include <core/buffer.h>

// A 4x4x4 cube with a bit mask that determines what is visible when the camera
// is inside the cube. Similar to how the collision works.
struct OcclusionOctant {
	s32 x = -1;
	s32 y = -1;
	s32 z = -1;
	s32 index = -1;
	s32 index2 = -1;
	u8 mask[128] = {};
};

std::vector<OcclusionOctant> read_occlusion_grid(Buffer src);
void write_occlusion_grid(OutBuffer dest, std::vector<OcclusionOctant>& octants);

struct OcclusionVector {
	s32 x = -1;
	s32 y = -1;
	s32 z = -1;
};

std::vector<OcclusionVector> read_occlusion_octants(std::string& str);
void write_occlusion_octants(OutBuffer dest, const std::vector<OcclusionVector>& octants);

void swap_occlusion(std::vector<OcclusionOctant>& grid, std::vector<OcclusionVector>& vectors);

#endif
