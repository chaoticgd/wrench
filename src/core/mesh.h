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

#ifndef MESH_H
#define MESH_H

#include "util.h"

struct TriFace {
	s32 v0;
	s32 v1;
	s32 v2;
	s32 collision_type;
	
	bool operator==(const TriFace& rhs) const {
		return v0 == rhs.v0 && v1 == rhs.v1 && v2 == rhs.v2 && collision_type == rhs.collision_type;
	}
	
	bool operator<(const TriFace& rhs) {
		if(v0 != rhs.v0) {
			return v0 < rhs.v0;
		} else if(v1 != rhs.v1) {
			return v2 < rhs.v2;
		} else if(v2 != rhs.v2) {
			return v2 < rhs.v2;
		} else {
			return collision_type < rhs.collision_type;
		}
	}
};

struct QuadFace {
	s32 v0;
	s32 v1;
	s32 v2;
	s32 v3;
	s32 collision_type;
	
	bool operator==(const QuadFace& rhs) const {
		return v0 == rhs.v0 && v1 == rhs.v1 && v2 == rhs.v2 && v3 == rhs.v3 && collision_type == rhs.collision_type;
	}
	
	bool operator<(const QuadFace& rhs) {
		if(v0 != rhs.v0) {
			return v0 < rhs.v0;
		} else if(v1 != rhs.v1) {
			return v2 < rhs.v2;
		} else if(v2 != rhs.v2) {
			return v2 < rhs.v2;
		} else if(v3 != rhs.v3) {
			return v3 < rhs.v3;
		} else {
			return collision_type < rhs.collision_type;
		}
	}
};

struct Mesh {
	std::vector<glm::vec3> positions;
	Opt<std::vector<glm::vec2>> texture_coords;
	std::vector<TriFace> tris;
	std::vector<QuadFace> quads;
	bool is_collision_mesh;
};

Mesh sort_vertices(Mesh mesh);

Mesh deduplicate_vertices(Mesh old_mesh);
Mesh deduplicate_faces(Mesh old_mesh);

#endif
