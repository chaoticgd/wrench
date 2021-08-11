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

#ifndef COLLADA_H
#define COLLADA_H

#include "util.h"
#include "buffer.h"

struct TriFace {
	s32 v0;
	s32 v1;
	s32 v2;
	s32 collision_type;
};

struct QuadFace {
	s32 v0;
	s32 v1;
	s32 v2;
	s32 v3;
	s32 collision_type;
};

struct Mesh {
	std::vector<glm::vec3> positions;
	Opt<std::vector<glm::vec2>> texture_coords;
	std::vector<TriFace> tris;
	std::vector<QuadFace> quads;
	bool is_collision_mesh;
};

struct DaeNode {
	std::string name;
	s32 mesh;
	Opt<glm::vec3> translate;
	std::vector<DaeNode> children;
};

struct DaeScene {
	std::vector<DaeNode> nodes;
	std::vector<Mesh> meshes;
};

DaeScene mesh_to_dae(Mesh mesh);
DaeScene import_dae(std::vector<u8> src);
std::vector<u8> write_dae(const DaeScene& scene);

#endif
