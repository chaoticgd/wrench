/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include <core/mesh.h>
#include <core/buffer.h>
#include <core/material.h>

// Represents the fields of a material that get written to a COLLADA file, and
// is used to cross-reference COLLADA materials with material assets using the
// name field.
struct ColladaMaterial {
	std::string name;
	MaterialSurface surface;
	s32 collision_id = -1; // Only used by the collision code.
	ColladaMaterial() {}
	ColladaMaterial(const Material& material)
		: name(material.name)
		, surface(material.surface) {}
	Material to_material() {
		Material material;
		material.name = name;
		material.surface = surface;
		return material;
	}
};

struct Joint {
	s32 parent = -1;
	s32 first_child = -1;
	s32 left_sibling = -1;
	s32 right_sibling = -1;
	glm::mat4 inverse_bind_matrix;
	glm::vec3 tip;
};

struct ColladaScene {
	mutable std::vector<std::string> texture_paths;
	std::vector<ColladaMaterial> materials;
	std::vector<Mesh> meshes;
	std::vector<Joint> joints;
	
	Mesh* find_mesh(const std::string& name);
};

// Parse a COLLADA mesh from the provided string.
ColladaScene read_collada(char* src);

// Rewrite SubMesh::material indices so they index into the passed materials array.
void map_lhs_material_indices_to_rhs_list(ColladaScene& scene, const std::vector<Material>& materials);

// Convert a ColladaScene structure into an XML document.
std::vector<u8> write_collada(const ColladaScene& scene);

s32 add_joint(std::vector<Joint>& joints, Joint joint, s32 parent);
void assert_collada_scenes_equal(const ColladaScene& lhs, const ColladaScene& rhs);

#endif
