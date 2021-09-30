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
	TriFace() { memset(this, 0, sizeof(TriFace)); }
	bool operator==(const TriFace& rhs) { return memcmp(this, &rhs, sizeof(TriFace)) == 0; }
	bool operator<(const TriFace& rhs) { return memcmp(this, &rhs, sizeof(TriFace)) < 0; }
	s32 v0;
	s32 v1;
	s32 v2;
	s32 collision_type;
};

struct QuadFace {
	QuadFace() { memset(this, 0, sizeof(QuadFace)); }
	bool operator==(const QuadFace& rhs) { return memcmp(this, &rhs, sizeof(QuadFace)) == 0; }
	bool operator<(const QuadFace& rhs) { return memcmp(this, &rhs, sizeof(QuadFace)) < 0; }
	s32 v0;
	s32 v1;
	s32 v2;
	s32 v3;
	s32 collision_type;
};

struct Vertex {
	Vertex() { memset(this, 0, sizeof(Vertex)); }
	Vertex(const glm::vec3& p) : Vertex() { pos = p; }
	bool operator==(const Vertex& rhs) { return memcmp(this, &rhs, sizeof(Vertex)) == 0; }
	bool operator<(const Vertex& rhs) { return memcmp(this, &rhs, sizeof(Vertex)) < 0; }
	glm::vec3 pos;
	glm::vec2 uv;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<TriFace> tris;
	std::vector<QuadFace> quads;
	bool has_uvs = false;
	bool is_collision_mesh = false;
};

Mesh sort_vertices(Mesh mesh);

Mesh deduplicate_vertices(Mesh old_mesh);
Mesh deduplicate_faces(Mesh old_mesh);

#endif
