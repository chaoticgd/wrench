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

#include "mesh.h"
#include "util.h"
#include "buffer.h"

struct ColladaNode {
	std::string name;
	s32 mesh;
	Opt<glm::vec3> translate;
	std::vector<ColladaNode> children;
};

struct ColladaScene {
	std::vector<ColladaNode> nodes;
	std::vector<Mesh> meshes;
};

ColladaScene mesh_to_dae(Mesh mesh);
ColladaScene import_dae(std::vector<u8> src);
std::vector<u8> write_collada(const ColladaScene& scene);

#endif
