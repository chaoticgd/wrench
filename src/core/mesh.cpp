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

#include "mesh.h"

#include <limits>
#include <algorithm>
#include <core/timer.h>

Mesh sort_vertices(Mesh src, bool (*compare)(const Vertex& lhs, const Vertex& rhs))
{
	std::vector<s32> vertex_mapping(src.vertices.size());
	for (size_t i = 0; i < src.vertices.size(); i++) {
		vertex_mapping[i] = i;
	}
	
	if (compare) {
		std::sort(BEGIN_END(vertex_mapping), [&](s32 lhs, s32 rhs) {
			return compare(src.vertices[lhs], src.vertices[rhs]);
		});
	} else {
		std::sort(BEGIN_END(vertex_mapping), [&](s32 lhs, s32 rhs) {
			return src.vertices[lhs] < src.vertices[rhs];
		});
	}
	
	std::vector<s32> inverse_mapping(src.vertices.size());
	for (size_t i = 0; i < src.vertices.size(); i++) {
		inverse_mapping[vertex_mapping[i]] = i;
	}
	
	Mesh dest;
	dest.name = std::move(src.name);
	dest.flags = src.flags;
	dest.vertices.reserve(src.vertices.size());
	for (size_t i = 0; i < src.vertices.size(); i++) {
		dest.vertices.push_back(src.vertices[vertex_mapping[i]]);
	}
	dest.submeshes = std::move(src.submeshes);
	for (SubMesh& submesh : dest.submeshes) {
		for (Face& face : submesh.faces) {
			face.v0 = inverse_mapping.at(face.v0);
			face.v1 = inverse_mapping.at(face.v1);
			face.v2 = inverse_mapping.at(face.v2);
			if (face.v3 > -1) {
				face.v3 = inverse_mapping.at(face.v3);
			}
		}
	}
	return dest;
}

Mesh deduplicate_vertices(Mesh src)
{
	Mesh dest;
	dest.name = std::move(src.name);
	dest.flags = src.flags;
	
	// Build a mapping that will allow us to iterate over the vertices as if
	// they were sorted.
	std::vector<s32> vertex_mapping(src.vertices.size());
	for (size_t i = 0; i < src.vertices.size(); i++) {
		vertex_mapping[i] = i;
	}
	
	std::sort(BEGIN_END(vertex_mapping), [&](s32 lhs, s32 rhs) {
		return src.vertices[lhs] < src.vertices[rhs];
	});
	
	std::vector<s32> index_mapping(src.vertices.size());
	std::vector<bool> discard(src.vertices.size(), true);
	
	s32 last_unique_i = 0;
	
	// Process a run of equal vertices.
	auto process_run = [&](s32 begin, s32 end) {
		s32 unique = INT32_MAX;
		for (s32 i = begin; i < end; i++) {
			unique = std::min(unique, vertex_mapping[i]);
		}
		if (unique != INT32_MAX) {
			discard[unique] = false;
			for (s32 i = begin; i < end; i++) {
				index_mapping[vertex_mapping[i]] = unique;
			}
		}
	};
	
	// Iterate over each consecutive pair of vertices as in sorted order.
	s32 i;
	for (i = 1; i < (s32) src.vertices.size(); i++) {
		Vertex& prev = src.vertices[vertex_mapping[i - 1]];
		Vertex& cur = src.vertices[vertex_mapping[i]];
		bool equal = true;
		equal &= vec3_equal_eps(prev.pos, cur.pos);
		equal &= vec3_equal_eps(prev.normal, cur.normal);
		equal &= prev.skin == cur.skin;
		equal &= prev.colour == cur.colour;
		equal &= vec2_equal_eps(prev.tex_coord, cur.tex_coord);
		if (!equal) {
			process_run(last_unique_i, i);
			last_unique_i = i;
		}
	}
	
	process_run(last_unique_i, (s32) src.vertices.size());
	
	// Copy over the unique vertices, preserving their original ordering.
	std::vector<s32> src_to_dest(src.vertices.size(), -1);
	for (size_t i = 0; i < src.vertices.size(); i++) {
		if (!discard[i]) {
			src_to_dest[i] = dest.vertices.size();
			dest.vertices.emplace_back(src.vertices[i]);
		}
	}
	
	// Rewrite the index_mapping list so it points to the destination vertices
	// instead of the source vertices.
	for (s32& index : index_mapping) {
		index = src_to_dest[index];
	}
	
	// Map the indices.
	dest.submeshes = std::move(src.submeshes);
	for (SubMesh& submesh : dest.submeshes) {
		for (Face& face : submesh.faces) {
			face.v0 = index_mapping.at(face.v0);
			face.v1 = index_mapping.at(face.v1);
			face.v2 = index_mapping.at(face.v2);
			if (face.v3 > -1) {
				face.v3 = index_mapping.at(face.v3);
			}
		}
	}
	
	return dest;
}

Mesh deduplicate_faces(Mesh mesh)
{
	start_timer("Deduplicating faces (remember to make this not N^2)");
	for (SubMesh& submesh : mesh.submeshes) {
		auto faces = std::move(submesh.faces);
		std::sort(BEGIN_END(faces));
		
		submesh.faces = std::vector<Face>();
		if (faces.size() > 0) {
			submesh.faces.push_back(faces[0]);
		}
		
		std::vector<Face> unique_tris;
		for (size_t i = 1; i < faces.size(); i++) {
			Face& prev = faces[i - 1];
			Face& cur = faces[i];
			if (!(prev == cur)) {
				if (cur.is_quad()) {
					submesh.faces.push_back(cur);
				} else {
					unique_tris.push_back(cur);
				}
			}
		}
		
		size_t quad_count = submesh.faces.size();
		for (const Face& tri : unique_tris) {
			bool discard = false;
			for (size_t i = 0; i < quad_count; i++) {
				const Face& quad = submesh.faces[i];
				discard |= tri.v0 == quad.v0 && tri.v1 == quad.v1 && tri.v2 == quad.v2;
				discard |= tri.v0 == quad.v1 && tri.v1 == quad.v2 && tri.v2 == quad.v3;
				discard |= tri.v0 == quad.v2 && tri.v1 == quad.v3 && tri.v2 == quad.v0;
				discard |= tri.v0 == quad.v3 && tri.v1 == quad.v0 && tri.v2 == quad.v1;
			}
			if (!discard) {
				submesh.faces.push_back(tri);
			}
		}
	}
	stop_timer();
	return mesh;
}

void remove_zero_area_triangles(Mesh& mesh)
{
	for (SubMesh& submesh : mesh.submeshes) {
		std::vector<Face> old_faces = std::move(submesh.faces);
		submesh.faces = {};
		for (Face& face : old_faces) {
			if (face.is_quad() || !(face.v0 == face.v1 || face.v0 == face.v2 || face.v1 == face.v2)) {
				submesh.faces.emplace_back(face);
			}
		}
	}
}

void fix_winding_orders_of_triangles_based_on_normals(Mesh& mesh)
{
	for (SubMesh& submesh : mesh.submeshes) {
		for (Face& face : submesh.faces) {
			Vertex& v0 = mesh.vertices[face.v0];
			Vertex& v1 = mesh.vertices[face.v1];
			Vertex& v2 = mesh.vertices[face.v2];
			glm::vec3 stored_normal = (v0.normal + v1.normal + v2.normal) / 3.f;
			glm::vec3 calculated_normal = glm::cross(v1.pos - v0.pos, v2.pos - v0.pos);
			if (glm::dot(calculated_normal, stored_normal) < 0.f) {
				std::swap(face.v0, face.v2);
			}
		}
	}
}

bool vec2_equal_eps(const glm::vec2& lhs, const glm::vec2& rhs, f32 eps)
{
	return fabs(lhs.x - rhs.x) < eps && fabs(lhs.y - rhs.y) < eps;
}

bool vec3_equal_eps(const glm::vec3& lhs, const glm::vec3& rhs, f32 eps)
{
	return fabs(lhs.x - rhs.x) < eps && fabs(lhs.y - rhs.y) < eps && fabs(lhs.z - rhs.z) < eps;
}

Mesh merge_meshes(const std::vector<Mesh>& meshes, std::string name, u32 flags)
{
	Mesh merged;
	merged.name = std::move(name);
	merged.flags = flags;
	
	SubMesh* dest = nullptr;
	
	for (const Mesh& mesh : meshes) {
		s32 base = (s32) merged.vertices.size();
		merged.vertices.insert(merged.vertices.end(), BEGIN_END(mesh.vertices));
		for (const SubMesh& src : mesh.submeshes) {
			if (dest == nullptr || src.material != dest->material) {
				dest = &merged.submeshes.emplace_back();
				dest->material = src.material;
			}
			for (const Face& face : src.faces) {
				dest->faces.emplace_back(base + face.v0, base + face.v1, base + face.v2, base + face.v3);
			}
		}
	}
	
	return merged;
}

glm::vec4 approximate_bounding_sphere(
	const glm::mat4** cuboids,
	size_t cuboid_count,
	const std::pair<const glm::vec4*, size_t>* splines,
	size_t spline_count)
{
	std::vector<Vertex> vertices;
	for (size_t i = 0; i < cuboid_count; i++) {
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(-1.f, -1.f, -1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(-1.f, -1.f, +1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(-1.f, +1.f, -1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(-1.f, +1.f, +1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(+1.f, -1.f, -1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(+1.f, -1.f, +1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(+1.f, +1.f, -1.f, 1.f)));
		vertices.emplace_back(glm::vec3(*cuboids[i] * glm::vec4(+1.f, +1.f, +1.f, 1.f)));
	}
	for (size_t i = 0; i < spline_count; i++) {
		for (size_t j = 0; j < splines[i].second; j++) {
			vertices.emplace_back(glm::vec3(splines[i].first[j]));
		}
	}
	return approximate_bounding_sphere(vertices); 
}

glm::vec4 approximate_bounding_sphere(const std::vector<Vertex>& vertices)
{
	BSphereVertexList list;
	list.vertices = vertices.data();
	list.vertex_count = vertices.size();
	return approximate_bounding_sphere(&list, 1);
}

glm::vec4 approximate_bounding_sphere(const BSphereVertexList* vertex_lists, size_t vertex_list_count)
{
	size_t total_vertex_count = 0;
	for (size_t i = 0; i < vertex_list_count; i++) {
		total_vertex_count += vertex_lists[i].vertex_count;
	}
	
	if (total_vertex_count == 0) {
		return glm::vec4(0, 0, 0, 0);
	}
	
	glm::vec3 min = {
		std::numeric_limits<f32>::max(),
		std::numeric_limits<f32>::max(),
		std::numeric_limits<f32>::max()
	};
	for (size_t i = 0; i < vertex_list_count; i++) {
		for (size_t j = 0; j < vertex_lists[i].vertex_count; j++) {
			min.x = std::min(vertex_lists[i].vertices[j].pos.x, min.x);
			min.y = std::min(vertex_lists[i].vertices[j].pos.y, min.y);
			min.z = std::min(vertex_lists[i].vertices[j].pos.z, min.z);
		}
	}
	
	glm::vec3 max = {
		std::numeric_limits<f32>::min(),
		std::numeric_limits<f32>::min(),
		std::numeric_limits<f32>::min()
	};
	for (size_t i = 0; i < vertex_list_count; i++) {
		for (size_t j = 0; j < vertex_lists[i].vertex_count; j++) {
			max.x = std::max(vertex_lists[i].vertices[j].pos.x, max.x);
			max.y = std::max(vertex_lists[i].vertices[j].pos.y, max.y);
			max.z = std::max(vertex_lists[i].vertices[j].pos.z, max.z);
		}
	}
	
	glm::vec3 centre = (min + max) / 2.f;
	
	f32 radius = 0.f;
	for (size_t i = 0; i < vertex_list_count; i++) {
		for (size_t j = 0; j < vertex_lists[i].vertex_count; j++) {
			glm::vec3 delta = vertex_lists[i].vertices[j].pos - centre;
			radius = std::max(sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z), radius);
		}
	}
	
	return glm::vec4(centre, radius);
}
