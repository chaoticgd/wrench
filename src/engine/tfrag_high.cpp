/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include "tfrag_high.h"

enum class TfaceType : u8 {
	QUAD, TRI
};

struct Tface {
	TfaceType type;
	u8 side_divisions[3] = {};
	s32 faces[16] = {};
};

struct TfragFace {
	s32 ad_gif;
	s32 indices[4];
};

static std::vector<TfragFace> recover_faces(const Tfrag& tfrag);

struct VertexInfoEx {
	const TfragVertexInfo* info;
	s32 parent = -1;
	s32 grand_parent = -1;
	// [Child]  Child
	//       \    |
	//        \   |
	//  Child---Parent
	bool is_diagonal_leaf;
};

ColladaScene recover_tfrags(const Tfrags& tfrags) {
	if(tfrag_debug_output_enabled()) {
		return recover_tfrags_debug(tfrags);
	}
	
	s32 texture_count = 0;
	for(const Tfrag& tfrag : tfrags.fragments) {
		for(const TfragTexturePrimitive& primitive : tfrag.common_textures) {
			texture_count = std::max(texture_count, primitive.d1_tex0_1.data_lo + 1);
		}
	}
	
	ColladaScene scene;
	
	for(s32 i = 0; i < texture_count; i++) {
		ColladaMaterial& material = scene.materials.emplace_back();
		material.name = stringf("%d", i);
		material.surface = MaterialSurface(i);
		
		scene.texture_paths.emplace_back(stringf("%d.png", i));
	}
	
	Mesh& lost_found = scene.meshes.emplace_back();
	lost_found.name = "lost_and_found";
	lost_found.flags = MESH_HAS_QUADS | MESH_HAS_TEX_COORDS;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		// Append vertex positions from different LODs.
		std::vector<TfragVertexPosition> positions;
		positions.insert(positions.end(), BEGIN_END(tfrag.common_positions));
		positions.insert(positions.end(), BEGIN_END(tfrag.lod_01_positions));
		positions.insert(positions.end(), BEGIN_END(tfrag.lod_0_positions));
		
		// Append vertex information from different LODs and determine
		// parent-child relationships.
		std::vector<VertexInfoEx> vertices;
		vertices.reserve(
			tfrag.common_vertex_info.size() +
			tfrag.lod_01_vertex_info.size() +
			tfrag.lod_0_vertex_info.size());
		for(size_t i = 0; i < tfrag.common_vertex_info.size(); i++) {
			VertexInfoEx& dest = vertices.emplace_back();
			dest.info = &tfrag.common_vertex_info[i];
		}
		for(size_t i = 0; i < tfrag.lod_01_vertex_info.size(); i++) {
			VertexInfoEx& dest = vertices.emplace_back();
			dest.info = &tfrag.lod_01_vertex_info[i];
			if(i < tfrag.lod_01_parent_indices.size()) {
				dest.parent = tfrag.lod_01_parent_indices[i];
			}
		}
		for(size_t i = 0; i < tfrag.lod_0_vertex_info.size(); i++) {
			VertexInfoEx& dest = vertices.emplace_back();
			dest.info = &tfrag.lod_0_vertex_info[i];
			if(i < tfrag.lod_0_parent_indices.size()) {
				dest.parent = tfrag.lod_0_parent_indices[i];
				if(dest.parent > -1 && dest.parent < vertices.size()) {
					dest.grand_parent = vertices[dest.parent].parent;
				}
			}
		}
		
		// Construct a list of the highest LOD faces.
		std::vector<TfragFace> faces = recover_faces(tfrag);
		
		// Detect diangonal leaves.
		for(const TfragFace& face : faces) {
			// Enumerate parent vertices.
			s32 parents[4] = {-1, -1, -1, -1};
			for(s32 i = 0; i < 4; i++) {
				if(parents[face.indices[i]] > -1) {
					parents[i] = parents[face.indices[i]];
				}
			}
			
			// Detect diagonals.
			s32 diagonal = -1;
			if(parents[0] != -1 && parents[3] == parents[0] && parents[0] == parents[1]) diagonal = face.indices[0];
			if(parents[1] != -1 && parents[0] == parents[1] && parents[1] == parents[2]) diagonal = face.indices[1];
			if(parents[2] != -1 && parents[1] == parents[2] && parents[2] == parents[3]) diagonal = face.indices[2];
			if(parents[3] != -1 && parents[2] == parents[3] && parents[3] == parents[0]) diagonal = face.indices[3];
			
			// Mark diagonal leaves.
			if(diagonal > -1) {
				vertices.at(diagonal).is_diagonal_leaf = true;
			}
		}
		
		// Create vertices and faces.
		size_t tface_base = scene.meshes.size();
		size_t vertex_count =
			tfrag.common_vertex_info.size() +
			tfrag.lod_01_vertex_info.size() +
			tfrag.lod_0_vertex_info.size();
		std::vector<s32> tfaces(tfrag.common_vertex_info.size(), -1);
		for(const TfragFace& face : faces) {
			// Identify which tface this face is a part of.
			s32 tface_index = -1;
			for(s32 i = 0; i < 4; i++) {
				if(vertices.at(face.indices[i]).is_diagonal_leaf) {
					tface_index = (vertices[face.indices[i]].grand_parent > -1) ?
						vertices[face.indices[i]].grand_parent :
						vertices[face.indices[i]].parent;
					break;
				}
			}
			
			// Create a mesh for this tface if it doesn't already exist.
			Mesh* mesh = nullptr;
			if(tface_index > -1) {
				verify(tface_index < tfaces.size(), "Bad tfaces.");
				s32& mesh_index = tfaces.at(tface_index);
				if(mesh_index == -1) {
					mesh_index = (s32) scene.meshes.size();
					mesh = &scene.meshes.emplace_back();
					mesh->name = stringf("tface_%d", mesh_index);
					mesh->flags |= MESH_HAS_QUADS | MESH_HAS_TEX_COORDS;
				} else {
					mesh = &scene.meshes.at(mesh_index);
				}
			} else {
				mesh = &scene.meshes.at(0);
			}
			
			// Add four new vertices.
			s32 vertex_base = (s32) mesh->vertices.size();
			for(s32 i = 0; i < 4; i++) {
				Vertex& dest = mesh->vertices.emplace_back();
				const TfragVertexInfo& src = *vertices.at(face.indices[i]).info;
				s16 index = src.vertex_data_offsets[1] / 2;
				verify_fatal(index >= 0 && index < positions.size());
				const TfragVertexPosition& pos = positions[index];
				dest.pos.x = (tfrag.base_position.vif1_r0 + pos.x) / 1024.f;
				dest.pos.y = (tfrag.base_position.vif1_r1 + pos.y) / 1024.f;
				dest.pos.z = (tfrag.base_position.vif1_r2 + pos.z) / 1024.f;
				dest.tex_coord.s = vu_fixed12_to_float(src.s);
				dest.tex_coord.t = vu_fixed12_to_float(src.t);
			}
	
			// Try to find a submesh with the right material.
			SubMesh* submesh = nullptr;
			s32 material = tfrag.common_textures.at(face.ad_gif).d1_tex0_1.data_lo;
			for(size_t i = 0; i < mesh->submeshes.size(); i++) {
				if(mesh->submeshes[i].material == material) {
					submesh = &mesh->submeshes[i];
				}
			}
			
			// Create a submesh if no suitable ones were found.
			if(submesh == nullptr) {
				submesh = &mesh->submeshes.emplace_back();
				submesh->material = material;
			}
			
			// Add the new face.
			Face& f = submesh->faces.emplace_back();
			f.v0 = vertex_base + 0;
			f.v1 = vertex_base + 1;
			f.v2 = vertex_base + 2;
			f.v3 = vertex_base + 3;
		}
	}
	
	return scene;
}

static std::vector<TfragFace> recover_faces(const Tfrag& tfrag) {
	std::vector<TfragFace> tfaces;
	s32 active_ad_gif = -1;
	s32 next_strip = 0;
	for(const TfragStrip& strip : tfrag.lod_0_strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if(vertex_count <= 0) {
			if(vertex_count == 0) {
				break;
			} else if(strip.end_of_packet_flag >= 0) {
				active_ad_gif = strip.ad_gif_offset / 0x5;
			}
			vertex_count += 128;
		}
		
		verify(vertex_count % 2 == 0, "Triangle strip contains non-quad faces.");
		
		for(s32 i = 0; i < vertex_count - 2; i += 2) {
			TfragFace& face = tfaces.emplace_back();
			face.ad_gif = active_ad_gif;
			for(s32 j = 0; j < 4; j++) {
				// 1 - 3    1 - 4
				// | / | -> |   |
				// 2 - 4    2 - 3
				s32 index = next_strip + i + (j ^ (j > 1));
				face.indices[j] = tfrag.lod_0_indices.at(index);
			}
		}
		
		next_strip += vertex_count;
	}
	return tfaces;
}
