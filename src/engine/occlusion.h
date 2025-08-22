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

#ifndef ENGINE_OCCLUSION_H
#define ENGINE_OCCLUSION_H

#include <core/buffer.h>

// A 4x4x4 cube with a bit mask that determines what is visible when the camera
// is inside the cube. Similar to how the collision works.
struct OcclusionOctant
{
	s32 x = -1;
	s32 y = -1;
	s32 z = -1;
	u8 visibility[128] = {};
	union {
		struct {
			s32 mask_index;
		} read_scratch;
		struct {
			s32 sort_index;
			s32 canonical;
			s32 new_index;
		} write_scratch;
	};
	
	bool operator==(const OcclusionOctant& rhs) const;
};

struct OcclusionVector
{
	s32 x = -1;
	s32 y = -1;
	s32 z = -1;
	s32 chunk = -1;
	
	friend auto operator<=>(const OcclusionVector& lhs, const OcclusionVector& rhs) = default;
};

std::vector<OcclusionOctant> read_occlusion_grid(Buffer src);
void write_occlusion_grid(OutBuffer dest, std::vector<OcclusionOctant>& octants);
s32 compute_occlusion_tree_size(std::vector<OcclusionVector> octants);

std::vector<OcclusionVector> read_occlusion_octants(const char* ptr);
void write_occlusion_octants(OutBuffer dest, const std::vector<OcclusionVector>& octants);

void swap_occlusion(std::vector<OcclusionOctant>& grid, std::vector<OcclusionVector>& vectors);

#endif
