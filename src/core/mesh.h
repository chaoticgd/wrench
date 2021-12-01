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

#include <glm/glm.hpp>

struct Face {
	s32 v0;
	s32 v1;
	s32 v2;
	s32 v3; // -1 for tris.
	Face(s32 a0, s32 a1, s32 a2, s32 a3 = -1)
		: v0(a0), v1(a1), v2(a2), v3(a3) {}
	bool operator==(const Face& rhs) const {
		return v0 == rhs.v0 && v1 == rhs.v1 && v2 == rhs.v2 && v3 == rhs.v3;
	}
	bool operator<(const Face& rhs) const {
		// Test v3 first to seperate out tris and quads.
		if(v3 != rhs.v3) return v3 < rhs.v3;
		if(v2 != rhs.v2) return v2 < rhs.v2;
		if(v1 != rhs.v1) return v1 < rhs.v1;
		return v0 < rhs.v0;
	}
	bool is_quad() const {
		return v3 > -1;
	}
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 tex_coord;
	Vertex(const glm::vec3& p) : pos(p), normal(0, 0, 0), tex_coord(0, 0) {}
	Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& t) : pos(p), normal(n), tex_coord(t) {}
	bool operator<(const Vertex& rhs) const {
		if(pos.z != rhs.pos.z) return pos.z < rhs.pos.z;
		if(pos.y != rhs.pos.y) return pos.y < rhs.pos.y;
		if(pos.x != rhs.pos.x) return pos.x < rhs.pos.x;
		if(normal.z != rhs.normal.z) return normal.z < rhs.normal.z;
		if(normal.y != rhs.normal.y) return normal.y < rhs.normal.y;
		if(normal.x != rhs.normal.x) return normal.x < rhs.normal.x;
		if(tex_coord.y != rhs.tex_coord.y) return tex_coord.y < rhs.tex_coord.y;
		return tex_coord.x < rhs.tex_coord.x;
	}
};

enum MeshFlags {
	MESH_HAS_QUADS = 1 << 0,
	MESH_HAS_NORMALS = 1 << 1,
	MESH_HAS_TEX_COORDS = 1 << 2
};

struct SubMesh {
	s32 material = -1;
	std::vector<Face> faces;
};

struct Mesh {
	std::string name;
	u32 flags = 0;
	std::vector<Vertex> vertices;
	std::vector<SubMesh> submeshes;
};

Mesh sort_vertices(Mesh mesh);

Mesh deduplicate_vertices(Mesh old_mesh);
Mesh deduplicate_faces(Mesh old_mesh); // Removes identical faces and tris that shadow quads.
Mesh reverse_winding_order(Mesh mesh);

#endif
