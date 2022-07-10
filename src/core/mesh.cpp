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
#include "timer.h"

Mesh sort_vertices(Mesh src, bool (*compare)(const Vertex& lhs, const Vertex& rhs)) {
	std::vector<s32> vertex_mapping(src.vertices.size());
	for(size_t i = 0; i < src.vertices.size(); i++) {
		vertex_mapping[i] = i;
	}
	
	if(compare) {
		std::sort(BEGIN_END(vertex_mapping), [&](s32 lhs, s32 rhs) {
			return compare(src.vertices[lhs], src.vertices[rhs]);
		});
	} else {
		std::sort(BEGIN_END(vertex_mapping), [&](s32 lhs, s32 rhs) {
			return src.vertices[lhs] < src.vertices[rhs];
		});
	}
	
	std::vector<s32> inverse_mapping(src.vertices.size());
	for(size_t i = 0; i < src.vertices.size(); i++) {
		inverse_mapping[vertex_mapping[i]] = i;
	}
	
	Mesh dest;
	dest.name = std::move(src.name);
	dest.flags = src.flags;
	dest.vertices.reserve(src.vertices.size());
	for(size_t i = 0; i < src.vertices.size(); i++) {
		dest.vertices.push_back(src.vertices[vertex_mapping[i]]);
	}
	dest.submeshes = std::move(src.submeshes);
	for(SubMesh& submesh : dest.submeshes) {
		for(Face& face : submesh.faces) {
			face.v0 = inverse_mapping.at(face.v0);
			face.v1 = inverse_mapping.at(face.v1);
			face.v2 = inverse_mapping.at(face.v2);
			if(face.v3 > -1) {
				face.v3 = inverse_mapping.at(face.v3);
			}
		}
	}
	return dest;
}

Mesh deduplicate_vertices(Mesh src) {
	src = sort_vertices(std::move(src));
	
	Mesh dest;
	dest.name = std::move(src.name);
	dest.flags = src.flags;
	std::vector<s32> index_mapping(src.vertices.size());
	if(src.vertices.size() > 0) {
		dest.vertices.push_back(src.vertices[0]);
		index_mapping[0] = 0;
	}
	for(size_t i = 1; i < src.vertices.size(); i++) {
		Vertex& prev = src.vertices[i - 1];
		Vertex& cur = src.vertices[i];
		bool discard = true;
		discard &= vec3_equal_eps(prev.pos, cur.pos);
		discard &= vec3_equal_eps(prev.normal, cur.normal);
		discard &= vec2_equal_eps(prev.tex_coord, cur.tex_coord);
		if(!discard) {
			dest.vertices.push_back(src.vertices[i]);
		}
		index_mapping[i] = dest.vertices.size() - 1;
	}
	dest.submeshes = std::move(src.submeshes);
	for(SubMesh& submesh : dest.submeshes) {
		for(Face& face : submesh.faces) {
			face.v0 = index_mapping.at(face.v0);
			face.v1 = index_mapping.at(face.v1);
			face.v2 = index_mapping.at(face.v2);
			if(face.v3 > -1) {
				face.v3 = index_mapping.at(face.v3);
			}
		}
	}
	return dest;
}

Mesh deduplicate_faces(Mesh mesh) {
	start_timer("Deduplicating faces (remember to make this not N^2)");
	for(SubMesh& submesh : mesh.submeshes) {
		auto faces = std::move(submesh.faces);
		std::sort(BEGIN_END(faces));
		
		submesh.faces = std::vector<Face>();
		if(faces.size() > 0) {
			submesh.faces.push_back(faces[0]);
		}
		
		std::vector<Face> unique_tris;
		for(size_t i = 1; i < faces.size(); i++) {
			Face& prev = faces[i - 1];
			Face& cur = faces[i];
			if(!(prev == cur)) {
				if(cur.is_quad()) {
					submesh.faces.push_back(cur);
				} else {
					unique_tris.push_back(cur);
				}
			}
		}
		
		size_t quad_count = submesh.faces.size();
		for(const Face& tri : unique_tris) {
			bool discard = false;
			for(size_t i = 0; i < quad_count; i++) {
				const Face& quad = submesh.faces[i];
				discard |= tri.v0 == quad.v0 && tri.v1 == quad.v1 && tri.v2 == quad.v2;
				discard |= tri.v0 == quad.v1 && tri.v1 == quad.v2 && tri.v2 == quad.v3;
				discard |= tri.v0 == quad.v2 && tri.v1 == quad.v3 && tri.v2 == quad.v0;
				discard |= tri.v0 == quad.v3 && tri.v1 == quad.v0 && tri.v2 == quad.v1;
			}
			if(!discard) {
				submesh.faces.push_back(tri);
			}
		}
	}
	stop_timer();
	return mesh;
}

bool vec2_equal_eps(const glm::vec2& lhs, const glm::vec2& rhs, f32 eps) {
	return fabs(lhs.x - rhs.x) < eps && fabs(lhs.y - rhs.y) < eps;
}

bool vec3_equal_eps(const glm::vec3& lhs, const glm::vec3& rhs, f32 eps) {
	return fabs(lhs.x - rhs.x) < eps && fabs(lhs.y - rhs.y) < eps && fabs(lhs.z - rhs.z) < eps;
}

Mesh merge_meshes(const std::vector<Mesh>& meshes, std::string name, u32 flags) {
	Mesh merged;
	merged.name = std::move(name);
	merged.flags = flags;
	
	SubMesh* dest = nullptr;
	
	for(const Mesh& mesh : meshes) {
		s32 base = (s32) merged.vertices.size();
		merged.vertices.insert(merged.vertices.end(), BEGIN_END(mesh.vertices));
		for(const SubMesh& src : mesh.submeshes) {
			if(dest == nullptr || src.material != dest->material) {
				dest = &merged.submeshes.emplace_back();
				dest->material = src.material;
			}
			for(const Face& face : src.faces) {
				dest->faces.emplace_back(base + face.v0, base + face.v1, base + face.v2, base + face.v3);
			}
		}
	}
	
	return merged;
}

glm::vec4 approximate_bounding_sphere(const std::vector<Vertex>& vertices) {
	if(vertices.size() == 0) {
		return glm::vec4(0, 0, 0, 0);
	}
	
	glm::vec3 min = vertices[0].pos;
	for(const Vertex& vertex : vertices) {
		min.x = std::min(vertex.pos.x, min.x);
		min.y = std::min(vertex.pos.y, min.y);
		min.z = std::min(vertex.pos.z, min.z);
	}
	
	glm::vec3 max = vertices[0].pos;
	for(const Vertex& vertex : vertices) {
		max.x = std::max(vertex.pos.x, max.x);
		max.y = std::max(vertex.pos.y, max.y);
		max.z = std::max(vertex.pos.z, max.z);
	}
	
	glm::vec3 centre = (min + max) / 2.f;
	
	f32 radius = 0.f;
	for(const Vertex& vertex : vertices) {
		glm::vec3 delta = vertex.pos - centre;
		radius = std::max(sqrtf(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z), radius);
	}
	
	return glm::vec4(centre, radius);
}
