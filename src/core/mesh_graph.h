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

#ifndef CORE_MESH_GRAPH_H
#define CORE_MESH_GRAPH_H

#include <core/mesh.h>
#include <core/material.h>

// A class that keeps track of the relationship between the vertices, edges and
// faces in a mesh. This is used by the tristrip code.

// Strongly typed indices into various arrays, so we don't mix them up.
#define INDEX_WRAPPER_TYPE(Type) \
	struct Type { \
		Type() : index(-1) {} \
		Type(s32 i) : index(i) {} \
		s32 index; \
		bool operator==(const Type& rhs) const { return index == rhs.index; } \
		bool operator!=(const Type& rhs) const { return index != rhs.index; } \
		bool operator<(const Type& rhs) const { return index < rhs.index; } \
	};
INDEX_WRAPPER_TYPE(VertexIndex)
INDEX_WRAPPER_TYPE(EdgeIndex)
INDEX_WRAPPER_TYPE(FaceIndex)
INDEX_WRAPPER_TYPE(MaterialIndex)
#define NULL_VERTEX_INDEX VertexIndex{-1}
#define NULL_EDGE_INDEX EdgeIndex{-1}
#define NULL_FACE_INDEX FaceIndex{-1}
#define NULL_MATERIAL_INDEX MaterialIndex{-1}

class MeshGraph {
	struct VertexInfo {
		std::vector<EdgeIndex> edges;
		s32 ref_count = 0;
	};
	std::vector<VertexInfo> _vertices;
	struct EdgeInfo {
		VertexIndex v[2];
		FaceIndex faces[2] = {NULL_FACE_INDEX, NULL_FACE_INDEX};
	};
	std::vector<EdgeInfo> _edges;
	struct FaceInfo {
		VertexIndex v[3];
		MaterialIndex material;
		s32 strip_index = -1;
		bool in_temp_strip = false;
	};
	std::vector<FaceInfo> _faces;
	
	VertexInfo& vertex_at(VertexIndex vertex) { return _vertices[vertex.index]; }
	const VertexInfo& vertex_at(VertexIndex vertex) const { return _vertices[vertex.index]; }
	EdgeInfo& edge_at(EdgeIndex edge) { return _edges[edge.index]; }
	const EdgeInfo& edge_at(EdgeIndex edge) const { return _edges[edge.index]; }
	FaceInfo& face_at(FaceIndex face) { return _faces[face.index]; }
	const FaceInfo& face_at(FaceIndex face) const { return _faces[face.index]; }
	
public:
	MeshGraph(const Mesh& mesh);
	
	s32 face_count() const { return _faces.size(); }
	VertexIndex face_vertex(FaceIndex face, s32 position) const { return face_at(face).v[position]; }
	MaterialIndex face_material(FaceIndex face) const { return face_at(face).material; }
	
	VertexIndex edge_vertex(EdgeIndex edge, s32 position) const { return edge_at(edge).v[position]; }
	
	EdgeIndex edge_of_face(FaceIndex face, s32 position) const {
		const FaceInfo& info = face_at(face);
		return edge(info.v[position], info.v[(position + 1) % 3]);
	}
	
	EdgeIndex edge(VertexIndex v0, VertexIndex v1) const {
		const VertexInfo& vertex = vertex_at(v0);
		for(EdgeIndex e : vertex.edges) {
			const EdgeInfo& edge = edge_at(e);
			if((edge.v[0] == v0 && edge.v[1] == v1) || (edge.v[0] == v1 && edge.v[1] == v0)) {
				return e;
			}
		}
		return NULL_EDGE_INDEX;
	}
	
	std::pair<FaceIndex, FaceIndex> faces_connected_by_edge(EdgeIndex edge) const {
		const EdgeInfo& info = edge_at(edge);
		return {info.faces[0], info.faces[1]};
	}
	
	FaceIndex other_face(EdgeIndex edge, FaceIndex face) const {
		const EdgeInfo& info = edge_at(edge);
		if(info.faces[0] == face) {
			return info.faces[1];
		} else {
			return info.faces[0];
		}
	}
	
	VertexIndex next_index(std::vector<VertexIndex>& indices, FaceIndex face) {
		assert(indices.size() >= 2);
		VertexIndex v0 = indices[indices.size() - 2];
		VertexIndex v1 = indices[indices.size() - 1];
		for(VertexIndex v : face_at(face).v) {
			if(v != v0 && v != v1) {
				return v;
			}
		}
		return NULL_VERTEX_INDEX;
	}
	
	// Used for determining if a given face is in an existing tristrip or not.
	bool is_in_strip(FaceIndex face) const { return face_at(face).strip_index > -1 || face_at(face).in_temp_strip; }
	void put_in_strip(FaceIndex face, s32 strip_index) { face_at(face).strip_index = strip_index; }
	void put_in_temp_strip(FaceIndex face) { face_at(face).in_temp_strip = true; }
	void discard_temp_strip() { for(FaceInfo& face : _faces) { face.in_temp_strip = false; } }
	
	bool can_be_added_to_strip(FaceIndex face, const EffectiveMaterial& effective) const {
		if(is_in_strip(face)) {
			// If the face is already in a strip it cannot be added to another strip.
			return false;
		}
		s32 material_of_face = face_material(face).index;
		for(s32 material : effective.materials) {
			if(material == material_of_face) {
				// The material is valid for the current strip.
				return true;
			}
		}
		// The face should be added to a different strip instead.
		return false;
	}
	
	FaceIndex face_really_expensive(VertexIndex v0, VertexIndex v1, VertexIndex v2) const {
		for(FaceIndex i = {0}; i < face_count(); i.index++) {
			const FaceInfo& face = face_at(i);
			bool good = true;
			for(VertexIndex test : face.v) {
				if(test != v0 && test != v1 && test != v2) {
					good = false;
					break;
				}
			}
			if(good) {
				return i;
			}
		}
		return NULL_FACE_INDEX;
	}
};

#endif
