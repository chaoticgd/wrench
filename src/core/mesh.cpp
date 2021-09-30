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

Mesh sort_vertices(Mesh mesh) {
	std::vector<s32> vertex_mapping(mesh.vertices.size());
	for(size_t i = 0; i < mesh.vertices.size(); i++) {
		vertex_mapping[i] = i;
	}
	
	std::sort(BEGIN_END(vertex_mapping), [&](s32 lhs, s32 rhs) {
		return mesh.vertices[lhs] < mesh.vertices[rhs];
	});
	
	std::vector<s32> inverse_mapping(mesh.vertices.size());
	for(size_t i = 0; i < mesh.vertices.size(); i++) {
		inverse_mapping[vertex_mapping[i]] = i;
	}
	
	Mesh new_mesh;
	new_mesh.vertices.reserve(mesh.vertices.size());
	for(size_t i = 0; i < mesh.vertices.size(); i++) {
		new_mesh.vertices.push_back(mesh.vertices[vertex_mapping[i]]);
	}
	new_mesh.tris = std::move(mesh.tris);
	for(TriFace& face : new_mesh.tris) {
		face.v0 = inverse_mapping.at(face.v0);
		face.v1 = inverse_mapping.at(face.v1);
		face.v2 = inverse_mapping.at(face.v2);
	}
	new_mesh.quads = std::move(mesh.quads);
	for(QuadFace& face : new_mesh.quads) {
		face.v0 = inverse_mapping.at(face.v0);
		face.v1 = inverse_mapping.at(face.v1);
		face.v2 = inverse_mapping.at(face.v2);
		face.v3 = inverse_mapping.at(face.v3);
	}
	return new_mesh;
}

Mesh deduplicate_vertices(Mesh old_mesh) {
	old_mesh = sort_vertices(std::move(old_mesh));
	
	Mesh new_mesh;
	std::vector<s32> index_mapping(old_mesh.vertices.size());
	if(old_mesh.vertices.size() > 0) {
		new_mesh.vertices.push_back(old_mesh.vertices[0]);
		index_mapping[0] = 0;
	}
	for(size_t i = 1; i < old_mesh.vertices.size(); i++) {
		Vertex& prev = old_mesh.vertices[i - 1];
		Vertex& cur = old_mesh.vertices[i];
		if(!(prev == cur)) {
			new_mesh.vertices.push_back(old_mesh.vertices[i]);
		}
		index_mapping[i] = new_mesh.vertices.size() - 1;
	}
	for(const TriFace& old_tri : old_mesh.tris) {
		TriFace new_tri;
		new_tri.v0 = index_mapping.at(old_tri.v0);
		new_tri.v1 = index_mapping.at(old_tri.v1);
		new_tri.v2 = index_mapping.at(old_tri.v2);
		new_tri.collision_type = old_tri.collision_type;
		new_mesh.tris.push_back(new_tri);
	}
	for(const QuadFace& old_quad : old_mesh.quads) {
		QuadFace new_quad;
		new_quad.v0 = index_mapping.at(old_quad.v0);
		new_quad.v1 = index_mapping.at(old_quad.v1);
		new_quad.v2 = index_mapping.at(old_quad.v2);
		new_quad.v3 = index_mapping.at(old_quad.v3);
		new_quad.collision_type = old_quad.collision_type;
		new_mesh.quads.push_back(new_quad);
	}
	new_mesh.is_collision_mesh = old_mesh.is_collision_mesh;
	return new_mesh;
}

Mesh deduplicate_faces(Mesh old_mesh) {
	std::sort(BEGIN_END(old_mesh.tris));
	std::sort(BEGIN_END(old_mesh.quads));
	
	Mesh new_mesh;
	new_mesh.vertices = std::move(old_mesh.vertices);
	if(old_mesh.tris.size() > 0) {
		new_mesh.tris.push_back(old_mesh.tris[0]);
	}
	for(size_t i = 1; i < old_mesh.tris.size(); i++) {
		TriFace& prev = old_mesh.tris[i - 1];
		TriFace& cur = old_mesh.tris[i];
		if(!(prev == cur)) {
			new_mesh.tris.push_back(cur);
		}
	}
	if(old_mesh.quads.size() > 0) {
		new_mesh.quads.push_back(old_mesh.quads[0]);
	}
	for(size_t i = 1; i < old_mesh.quads.size(); i++) {
		QuadFace& prev = old_mesh.quads[i - 1];
		QuadFace& cur = old_mesh.quads[i];
		if(!(prev == cur)) {
			new_mesh.quads.push_back(cur);
		}
	}
	new_mesh.is_collision_mesh = old_mesh.is_collision_mesh;
	return new_mesh;
}
