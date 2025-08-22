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

#include <glm/glm.hpp>
#include <core/util.h>

struct Face
{
	s32 v0;
	s32 v1;
	s32 v2;
	s32 v3 = -1; // -1 for tris.
	Face() {}
	Face(s32 a0, s32 a1, s32 a2, s32 a3 = -1)
		: v0(a0), v1(a1), v2(a2), v3(a3) {}
	bool operator==(const Face& rhs) const {
		return v0 == rhs.v0
			&& v1 == rhs.v1
			&& v2 == rhs.v2
			&& v3 == rhs.v3;
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

struct SkinAttributes
{
	u8 count = 0;
	s8 joints[3] = {0, 0, 0};
	u8 weights[3] = {0, 0, 0};
	
	friend auto operator<=>(const SkinAttributes&, const SkinAttributes&) = default;
};

struct ColourAttribute
{
	u8 r = 0;
	u8 g = 0;
	u8 b = 0;
	u8 a = 0;
	
	friend auto operator<=>(const ColourAttribute&, const ColourAttribute&) = default;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal = {0, 0, 0};
	SkinAttributes skin;
	ColourAttribute colour;
	glm::vec2 tex_coord = {0, 0};
	u16 vertex_index = 0xff; // Only used by the moby code.
	Vertex() {}
	Vertex(const glm::vec3& p) : pos(p) {}
	Vertex(const glm::vec3& p, const glm::vec3& n, const SkinAttributes& s)
		: pos(p), normal(n), skin(s) {}
	Vertex(const glm::vec3& p, const ColourAttribute& c, const glm::vec2& t)
		: pos(p), colour(c), tex_coord(t) {}
	bool operator==(const Vertex& rhs) const {
		return pos == rhs.pos
			&& normal == rhs.normal
			&& skin == rhs.skin
			&& colour == rhs.colour
			&& tex_coord == rhs.tex_coord;
	}
	bool operator<(const Vertex& rhs) const {
		// The moby code relies on the texture coordinates being compared last.
		if(pos.z != rhs.pos.z) return pos.z < rhs.pos.z;
		if(pos.y != rhs.pos.y) return pos.y < rhs.pos.y;
		if(pos.x != rhs.pos.x) return pos.x < rhs.pos.x;
		if(normal.z != rhs.normal.z) return normal.z < rhs.normal.z;
		if(normal.y != rhs.normal.y) return normal.y < rhs.normal.y;
		if(normal.x != rhs.normal.x) return normal.x < rhs.normal.x;
		if(!(skin == rhs.skin)) return skin < rhs.skin;
		if(!(colour == rhs.colour)) return colour < rhs.colour;
		if(tex_coord.y != rhs.tex_coord.y) return tex_coord.y < rhs.tex_coord.y;
		return tex_coord.x < rhs.tex_coord.x;
	}
};

enum MeshFlags
{
	MESH_HAS_QUADS = 1 << 0,
	MESH_HAS_NORMALS = 1 << 1,
	MESH_HAS_VERTEX_COLOURS = 1 << 2,
	MESH_HAS_TEX_COORDS = 1 << 3
};

struct SubMesh
{
	s32 material = -1;
	std::vector<Face> faces;
};

struct Mesh
{
	std::string name;
	u32 flags = 0;
	std::vector<Vertex> vertices;
	std::vector<SubMesh> submeshes;
};

Mesh sort_vertices(Mesh mesh, bool (*compare)(const Vertex& lhs, const Vertex& rhs) = nullptr);

Mesh deduplicate_vertices(Mesh old_mesh);
Mesh deduplicate_faces(Mesh old_mesh); // Removes identical faces and tris that shadow quads.
void remove_zero_area_triangles(Mesh& mesh); // Run deduplicate_vertices first!
void fix_winding_orders_of_triangles_based_on_normals(Mesh& mesh);

bool vec2_equal_eps(const glm::vec2& lhs, const glm::vec2& rhs, f32 eps = 0.00001f);
bool vec3_equal_eps(const glm::vec3& lhs, const glm::vec3& rhs, f32 eps = 0.00001f);

Mesh merge_meshes(const std::vector<Mesh>& meshes, std::string name, u32 flags);

struct BSphereVertexList
{
	const Vertex* vertices;
	size_t vertex_count;
};

glm::vec4 approximate_bounding_sphere(const glm::mat4** cuboids, size_t cuboid_count, const std::pair<const glm::vec4*, size_t>* splines, size_t spline_count);
glm::vec4 approximate_bounding_sphere(const std::vector<Vertex>& vertices);
glm::vec4 approximate_bounding_sphere(const BSphereVertexList* vertex_lists, size_t vertex_list_count);


#endif
