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

struct ColourF {
	f32 r;
	f32 g;
	f32 b;
	f32 a;
	
	bool operator==(const ColourF& rhs) const {
		return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
	}
};

struct Material {
	std::string name;
	std::optional<ColourF> colour;
	std::optional<s32> texture;
};

struct Joint {
	s32 parent = -1;
	s32 first_child = -1;
	s32 left_sibling = -1;
	s32 right_sibling = -1;
	glm::mat4 matrix;
};

struct ColladaScene {
	mutable std::vector<std::string> texture_paths;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Joint> joints;
};

ColladaScene mesh_to_dae(Mesh mesh);
ColladaScene read_collada(std::vector<u8> src); // Throws ParseError.
std::vector<u8> write_collada(const ColladaScene& scene);
s32 add_joint(std::vector<Joint>& joints, Joint joint, s32 parent);

#endif
