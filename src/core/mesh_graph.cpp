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

#include "mesh_graph.h"

MeshGraph::MeshGraph(const Mesh& mesh) {
	_vertices.resize(mesh.vertices.size());
	
	// Collect faces.
	for(const SubMesh& submesh : mesh.submeshes) {
		for(const Face& face : submesh.faces) {
			assert(!face.is_quad());
			if(face.is_quad()) {
				FaceInfo& info_1 = _faces.emplace_back();
				info_1.v[0] = face.v0;
				info_1.v[1] = face.v1;
				info_1.v[2] = face.v2;
				info_1.material = {submesh.material};
				
				FaceInfo& info_2 = _faces.emplace_back();
				info_2.v[0] = face.v2;
				info_2.v[1] = face.v3;
				info_2.v[2] = face.v0;
				info_2.material = {submesh.material};
			} else {
				FaceInfo& info = _faces.emplace_back();
				info.v[0] = face.v0;
				info.v[1] = face.v1;
				info.v[2] = face.v2;
				info.material = {submesh.material};
			}
		}
	}
	
	// Generate edge and vertex info structs.
	for(FaceIndex i = {0}; i.index < face_count(); i.index++) {
		FaceInfo& face = face_at(i);
		
		// Iterate over all the edges that make up the face.
		for(s32 j = 0; j < 3; j++) {
			VertexIndex v0 = face.v[j];
			VertexIndex v1 = face.v[(j + 1) % 3];
			if(v1 < v0) {
				std::swap(v0, v1);
			}
			
			// Try to find an edge info record for this edge.
			EdgeIndex index = edge(v0, v1);
			
			// Create an edge info record if it doesn't already exist, or
			// fill in the second face index if it does.
			if(index == NULL_EDGE_INDEX) {
				index = EdgeIndex((s32) _edges.size());
				EdgeInfo& info = _edges.emplace_back();
				info.v[0] = v0;
				info.v[1] = v1;
				vertex_at(v0).edges.emplace_back(index);
				vertex_at(v1).edges.emplace_back(index);
			}
			
			EdgeInfo& info = edge_at(index);
			
			if(info.faces[0] == NULL_FACE_INDEX) {
				info.faces[0] = i;
			} else if(info.faces[1] == NULL_FACE_INDEX) {
				info.faces[1] = i;
			} else {
				// The current face has an edge that connects three or more
				// faces. We should remove it from the graph so it doesn't cause
				// problems later.
				for(s32 k = j - 1; k >= 0; k--) {
					VertexIndex r0 = face.v[k];
					VertexIndex r1 = face.v[(k + 1) % 3];
					if(r1 < r0) {
						std::swap(r0, r1);
					}
					
					EdgeIndex remove_index = edge(r0, r1);
					assert(remove_index != NULL_EDGE_INDEX);
					
					EdgeInfo& remove_info = edge_at(remove_index);
					for(FaceIndex& remove_face : remove_info.faces) {
						if(remove_face == i) {
							remove_face = NULL_FACE_INDEX;
						}
					}
				}
				
				// We need to handle this separately later.
				face.is_evil = true;
				
				break;
			}
		}
	}
}
