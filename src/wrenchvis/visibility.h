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

#ifndef WRENCHVIS_VISIBILITY_H
#define WRENCHVIS_VISIBILITY_H

// Implements the Potentially Visible Set occlusion culling algorithm used for
// reducing the number of objects the game draws per frame based on the position
// of the camera.
//
// The playable space is divided up into cube-shaped "octants" and then it is
// determined which objects are visible from each octant. This data is then
// crunched down into a 1024 bit mask per octant for use by the game.

#include <core/mesh.h>
#include <engine/occlusion.h>

#define VIS_OBJECT_TYPE_COUNT 3
#define VIS_TFRAG 0
#define VIS_TIE 1
#define VIS_MOBY 2
#define VIS_MAX_CHUNKS 3
#define VIS_MAX_SAMPLES_PER_OCTANT 8

struct VisInstance
{
	s32 mesh;
	s32 chunk;
	glm::mat4 matrix;
};

struct VisSamplePoint
{
	s32 x = -1;
	s32 y = -1;
	s32 z = -1;
	
	friend auto operator<=>(const VisSamplePoint& lhs, const VisSamplePoint& rhs) = default;
};

struct VisInput
{
	// The size of a single octant. Normally 4x4x4.
	s32 octant_size_x;
	s32 octant_size_y;
	s32 octant_size_z;
	// List of samples to be taken for each octant.
	VisSamplePoint sample_points[VIS_MAX_SAMPLES_PER_OCTANT];
	// The octants for which visibility should be precomputed.
	std::vector<OcclusionVector> octants;
	// Lists of objects in the level that matter for occlusion.
	std::vector<VisInstance> instances[VIS_OBJECT_TYPE_COUNT];
	// List of meshes referenced by the instances.
	std::vector<const Mesh*> meshes;
};

struct VisOutput
{
	// For each object type, a mapping from the index of said object to the
	// index of the bit in the visibility mask which should be checked to see
	// if the object needs to be drawn.
	std::vector<s32> mappings[VIS_OBJECT_TYPE_COUNT];
	// A list of octants (4x4x4 cubes) for which visibility has been precomputed
	// including finished visibility masks.
	std::vector<OcclusionOctant> octants;
};

VisOutput compute_level_visibility(const VisInput& input, s32 memory_budget_for_masks);

#endif
