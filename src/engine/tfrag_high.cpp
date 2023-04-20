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

#include <set>

#define MAX_TFACES_TOUCHING_VERTEX 8
#define INIT_TFACE_INDICES \
	{-1, -1, -1, -1, -1, -1, -1, -1}

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

static std::vector<TfragFace> recover_faces(const std::vector<TfragStrip>& strips, const std::vector<u8>& indices);

struct TfragVertexEx {
	const TfragVertexPosition* position;
	s32 parents[2] = {-1, -1};
	s32 tfaces[MAX_TFACES_TOUCHING_VERTEX] = INIT_TFACE_INDICES;
	
	bool push_tface(s32 tface) {
		for(s32 i = 0; i < ARRAY_SIZE(tfaces); i++) {
			if(tfaces[i] == -1) {
				tfaces[i] = tface;
				return true;
			}
		}
		return false;
	}
	
	bool set_tfaces(const TfragVertexEx& left_parent, TfragVertexEx& right_parent) {
		for(s32 i = 0; i < ARRAY_SIZE(tfaces); i++) {
			if(left_parent.tfaces[i] > -1) {
				for(s32 j = 0; j < ARRAY_SIZE(tfaces); j++) {
					if(left_parent.tfaces[i] == right_parent.tfaces[j] && !push_tface(left_parent.tfaces[i])) {
						return false;
					}
				}
			}
		}
		return true;
	}
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
	lost_found.flags = MESH_HAS_QUADS | MESH_HAS_TEX_COORDS | MESH_HAS_VERTEX_COLOURS;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		// Enumerate vertex positions from different LODs.
		std::vector<TfragVertexEx> vertices;
		for(const TfragVertexPosition& position : tfrag.common_positions) {
			vertices.emplace_back().position = &position;
		}
		for(const TfragVertexPosition& position : tfrag.lod_01_positions) {
			vertices.emplace_back().position = &position;
		}
		for(const TfragVertexPosition& position : tfrag.lod_0_positions) {
			vertices.emplace_back().position = &position;
		}
		
		// Enumerate vertex infos.
		std::vector<TfragVertexInfo> vertex_infos;
		vertex_infos.insert(vertex_infos.end(), BEGIN_END(tfrag.common_vertex_info));
		vertex_infos.insert(vertex_infos.end(), BEGIN_END(tfrag.lod_01_vertex_info));
		vertex_infos.insert(vertex_infos.end(), BEGIN_END(tfrag.lod_0_vertex_info));
		
		// Determine parent-child relationships.
		for(size_t i = 0; i < tfrag.lod_01_vertex_info.size(); i++) {
			const TfragVertexInfo& info = tfrag.lod_01_vertex_info[i];
			TfragVertexEx& vertex = vertices.at(info.vertex / 2);
			if(i < tfrag.lod_01_parent_indices.size()) {
				vertex.parents[0] = vertex_infos.at(tfrag.lod_01_parent_indices[i]).vertex / 2;
			}
			vertex.parents[1] = info.parent / 2;
		}
		for(size_t i = 0; i < tfrag.lod_0_vertex_info.size(); i++) {
			const TfragVertexInfo& info = tfrag.lod_0_vertex_info[i];
			TfragVertexEx& vertex = vertices.at(info.vertex / 2);
			if(i < tfrag.lod_0_parent_indices.size()) {
				vertex.parents[0] = vertex_infos.at(tfrag.lod_0_parent_indices[i]).vertex / 2;
			}
			vertex.parents[1] = info.parent / 2;
		}
		
		// Mark all the LOD 2 vertices as belonging to particular tfaces.
		std::vector<TfragFace> lod_2_faces = recover_faces(tfrag.lod_2_strips, tfrag.lod_2_indices);
		for(size_t i = 0; i < lod_2_faces.size(); i++) {
			for(s32 index : lod_2_faces[i].indices) {
				if(index > -1) {
					TfragVertexEx& vertex = vertices.at(vertex_infos.at(index).vertex / 2);
					verify(vertex.push_tface(i), "Overloaded vertex (lod 2).");
				}
			}
		}
		
		// Propagate tface information to LOD 1 vertices.
		for(size_t i = 0; i < tfrag.lod_01_positions.size(); i++) {
			TfragVertexEx& vertex = vertices.at(tfrag.common_positions.size() + i);
			TfragVertexEx& left_parent = vertices.at(vertex.parents[0]);
			TfragVertexEx& right_parent = vertices.at(vertex.parents[1]);
			vertex.set_tfaces(left_parent, right_parent);
		}
		
		// Propagate tface information to LOD 0 vertices.
		for(size_t i = 0; i < tfrag.lod_0_positions.size(); i++) {
			TfragVertexEx& vertex = vertices.at(tfrag.common_positions.size() + tfrag.lod_01_positions.size() + i);
			TfragVertexEx& left_parent = vertices.at(vertex.parents[0]);
			TfragVertexEx& right_parent = vertices.at(vertex.parents[1]);
			vertex.set_tfaces(left_parent, right_parent);
		}
		
		// Create vertices and faces.
		std::vector<s32> tfaces(lod_2_faces.size(), -1);
		std::vector<TfragFace> lod_0_faces = recover_faces(tfrag.lod_0_strips, tfrag.lod_0_indices);
		for(const TfragFace& face : lod_0_faces) {
			// Identify which tface this face is a part of.
			s32 tface_index = -1;
			if(face.indices[3] > -1) {
				s32 tface_indices[MAX_TFACES_TOUCHING_VERTEX] = INIT_TFACE_INDICES;
				memcpy(tface_indices, vertices.at(vertex_infos.at(face.indices[0]).vertex / 2).tfaces, sizeof(tface_indices));
				for(s32 i = 1; i < ARRAY_SIZE(face.indices); i++) {
					TfragVertexEx& vertex = vertices.at(vertex_infos.at(face.indices[i]).vertex / 2);
					for(s32 j = 0; j < ARRAY_SIZE(tface_indices); j++) {
						bool found = false;
						for(s32 k = 0; k < ARRAY_SIZE(vertex.tfaces); k++) {
							if(vertex.tfaces[k] == tface_indices[j]) {
								found = true;
							}
						}
						if(!found) {
							tface_indices[j] = -1;
						}
					}
				}
				
				for(s32 i = 0; i < ARRAY_SIZE(tface_indices); i++) {
					bool matches = true;
					for(s32 j = 0; j < ARRAY_SIZE(tface_indices); j++) {
						if((i == j) ? (tface_indices[j] == -1) : (tface_indices[j] != -1 && tface_indices[j] != tface_indices[i])) {
							matches = false;
						}
					}
					if(matches) {
						tface_index = tface_indices[i];
						break;
					}
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
					mesh->flags |= MESH_HAS_QUADS | MESH_HAS_TEX_COORDS | MESH_HAS_VERTEX_COLOURS;
				} else {
					mesh = &scene.meshes.at(mesh_index);
				}
			} else {
				mesh = &scene.meshes.at(0);
			}
			
			// Add four new vertices.
			s32 vertex_base = (s32) mesh->vertices.size();
			for(s32 i = 0; i < ARRAY_SIZE(face.indices); i++) {
				if(face.indices[i] > -1) {
					Vertex& dest = mesh->vertices.emplace_back();
					const TfragVertexInfo& src = vertex_infos.at(face.indices[i]);
					s16 index = src.vertex / 2;
					verify_fatal(index >= 0 && index < vertices.size());
					const TfragVertexPosition& pos = *vertices[index].position;
					dest.pos.x = (tfrag.base_position.vif1_r0 + pos.x) / 1024.f;
					dest.pos.y = (tfrag.base_position.vif1_r1 + pos.y) / 1024.f;
					dest.pos.z = (tfrag.base_position.vif1_r2 + pos.z) / 1024.f;
					dest.tex_coord.s = vu_fixed12_to_float(src.s);
					dest.tex_coord.t = vu_fixed12_to_float(src.t);
					const TfragRgba& colour = tfrag.rgbas.at(index);
					dest.colour.r = colour.r;
					dest.colour.g = colour.g;
					dest.colour.b = colour.b;
					if(colour.a < 0x80) {
						dest.colour.a = colour.a * 2;
					} else {
						dest.colour.a = 255;
					}
				}
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
			f.v3 = (face.indices[3] > -1) ? (vertex_base + 3) : -1;
		}
	}
	
	return scene;
}

static std::vector<TfragFace> recover_faces(const std::vector<TfragStrip>& strips, const std::vector<u8>& indices) {
	std::vector<TfragFace> tfaces;
	s32 active_ad_gif = -1;
	s32 next_strip = 0;
	for(const TfragStrip& strip : strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if(vertex_count <= 0) {
			if(vertex_count == 0) {
				break;
			} else if(strip.end_of_packet_flag >= 0) {
				active_ad_gif = strip.ad_gif_offset / 0x5;
			}
			vertex_count += 128;
		}
		if(vertex_count % 2 == 0) {
			for(s32 i = 0; i < vertex_count - 2; i += 2) {
				TfragFace& face = tfaces.emplace_back();
				face.ad_gif = active_ad_gif;
				for(s32 j = 0; j < 4; j++) {
					// 1 - 3    1 - 4
					// | / | -> |   |
					// 2 - 4    2 - 3
					s32 index = next_strip + i + (j ^ (j > 1));
					face.indices[j] = indices.at(index);
				}
			}
		} else {
			for(s32 i = 0; i < vertex_count - 2; i++) {
				TfragFace& face = tfaces.emplace_back();
				face.ad_gif = active_ad_gif;
				face.indices[0] = indices.at(next_strip + i + 0);
				face.indices[1] = indices.at(next_strip + i + 1);
				face.indices[2] = indices.at(next_strip + i + 2);
				face.indices[3] = -1;
			}
		}
		
		next_strip += vertex_count;
	}
	return tfaces;
}
