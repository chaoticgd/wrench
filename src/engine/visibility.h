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

#ifndef ENGINE_VISIBILITY_H
#define ENGINE_VISIBILITY_H

#include <core/mesh.h>
#include <engine/occlusion.h>

#define VIS_OBJECT_TYPE_COUNT 3
#define VIS_TFRAG 0
#define VIS_TIE 1
#define VIS_MOBY 2

struct VisInstance {
	s32 mesh;
	glm::mat4 matrix;
};

struct VisInput {
	s32 octant_size_x;
	s32 octant_size_y;
	s32 octant_size_z;
	std::vector<OcclusionVector> octants;
	std::vector<VisInstance> instances[VIS_OBJECT_TYPE_COUNT];
	std::vector<const Mesh*> meshes;
};

struct VisOutput {
	std::vector<s32> mappings[VIS_OBJECT_TYPE_COUNT];
	std::vector<OcclusionOctant> octants;
};

VisOutput compute_level_visibility(const VisInput& input);

#endif
