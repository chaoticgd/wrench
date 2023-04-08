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

//#define TFRAG_DEBUG_RECOVER_ALL_LODS
//#define TFRAG_DEBUG_RAINBOW_STRIPS
//#define TFRAG_DEBUG_POLES

static TfragLod extract_highest_tfrag_lod(const Tfrag& tfrag);
static TfragLod extract_medium_tfrag_lod(const Tfrag& tfrag);
static TfragLod extract_low_tfrag_lod(const Tfrag& tfrag);
void recover_tfrag_lod(Mesh& mesh, const TfragLod& lod, const Tfrag& tfrag, s32 texture_count);
static s32 recover_tfrag_vertices(Mesh& mesh, const TfragLod& lod, s32 strip_index);

static void create_debug_pole_vertices(Mesh& mesh, const TfragLod& lod, s32 strip_index);
static void create_debug_pole_faces(Mesh& mesh, const TfragLod& lod, const Tfrag& tfrag, s32 vertex_base);

ColladaScene recover_tfrags(const Tfrags& tfrags) {
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
	
	if(texture_count == 0) {
		ColladaMaterial& dummy = scene.materials.emplace_back();
		dummy.name = "dummy";
		dummy.surface = MaterialSurface(glm::vec4(1.f, 1.f, 1.f, 1.f));
	}

#if defined(TFRAG_DEBUG_RAINBOW_STRIPS) || defined(TFRAG_DEBUG_POLES)
	u32 mesh_flags = MESH_HAS_VERTEX_COLOURS;
#else
	u32 mesh_flags = MESH_HAS_TEX_COORDS;
#endif

	Mesh& high_mesh = scene.meshes.emplace_back();
	high_mesh.name = "mesh";
	high_mesh.flags |= mesh_flags;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragLod lod = extract_highest_tfrag_lod(tfrag);
		recover_tfrag_lod(high_mesh, lod, tfrag, texture_count);
	}

#ifdef TFRAG_DEBUG_RECOVER_ALL_LODS
	Mesh& medium_mesh = scene.meshes.emplace_back();
	medium_mesh.name = "medium_lod";
	medium_mesh.flags |= mesh_flags;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragLod lod = extract_medium_tfrag_lod(tfrag);
		recover_tfrag_lod(medium_mesh, lod, tfrag, texture_count);
	}
	
	Mesh& low_mesh = scene.meshes.emplace_back();
	low_mesh.name = "low_lod";
	low_mesh.flags |= mesh_flags;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragLod lod = extract_low_tfrag_lod(tfrag);
		recover_tfrag_lod(low_mesh, lod, tfrag, texture_count);
	}
#endif
	
	return scene;
}

static TfragLod extract_highest_tfrag_lod(const Tfrag& tfrag) {
	TfragLod lod;
	lod.bsphere = tfrag.bsphere;
	lod.base_position = tfrag.base_position;
	lod.common_textures = tfrag.common_textures;
	lod.strips = tfrag.lod_0_strips;
	lod.indices.insert(lod.indices.end(), BEGIN_END(tfrag.lod_0_indices));
	lod.unknown_indices.insert(lod.unknown_indices.end(), BEGIN_END(tfrag.lod_0_unknown_indices));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.lod_01_vertex_info));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.lod_0_vertex_info));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.common_positions));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.lod_01_positions));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.lod_0_positions));
	lod.rgbas = tfrag.rgbas;
	lod.light = tfrag.light;
	lod.msphere = tfrag.msphere;
	lod.cube = tfrag.cube;
	return lod;
}

static TfragLod extract_medium_tfrag_lod(const Tfrag& tfrag) {
	TfragLod lod;
	lod.bsphere = tfrag.bsphere;
	lod.base_position = tfrag.base_position;
	lod.common_textures = tfrag.common_textures;
	lod.strips = tfrag.lod_1_strips;
	lod.indices.insert(lod.indices.end(), BEGIN_END(tfrag.lod_1_indices));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.lod_01_vertex_info));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.common_positions));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.lod_01_positions));
	lod.rgbas = tfrag.rgbas;
	lod.light = tfrag.light;
	lod.msphere = tfrag.msphere;
	lod.cube = tfrag.cube;
	return lod;
}

static TfragLod extract_low_tfrag_lod(const Tfrag& tfrag) {
	TfragLod lod;
	lod.bsphere = tfrag.bsphere;
	lod.base_position = tfrag.base_position;
	lod.common_textures = tfrag.common_textures;
	lod.strips = tfrag.lod_2_strips;
	lod.indices.insert(lod.indices.end(), BEGIN_END(tfrag.lod_2_indices));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.common_positions));
	lod.rgbas = tfrag.rgbas;
	lod.light = tfrag.light;
	lod.msphere = tfrag.msphere;
	lod.cube = tfrag.cube;
	return lod;
}

void recover_tfrag_lod(Mesh& mesh, const TfragLod& lod, const Tfrag& tfrag, s32 texture_count) {
	SubMesh* submesh = nullptr;
	s32 next_texture = 0;
	
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
	static s32 strip_index = 0;
#else
	s32 vertex_base = recover_tfrag_vertices(mesh, lod, 0);
#endif
	
	s32 index_offset = 0;
	for(const TfragStrip& strip : lod.strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if(vertex_count <= 0) {
			if(vertex_count == 0) {
				break;
			} else if(strip.end_of_packet_flag >= 0 && texture_count != 0) {
				next_texture = lod.common_textures.at(strip.ad_gif_offset / 0x5).d1_tex0_1.data_lo;
			}
			vertex_count += 128;
		}
		
		if(submesh == nullptr || next_texture != submesh->material) {
			submesh = &mesh.submeshes.emplace_back();
			submesh->material = next_texture;
		}
		
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
		s32 vertex_base = recover_tfrag_vertices(mesh, lod, strip_index++);
#endif
		
		s32 queue[2] = {};
		for(s32 i = 0; i < vertex_count; i++) {
			verify_fatal(index_offset < lod.indices.size());
			s32 index = lod.indices[index_offset++];
			verify_fatal(index >= 0 && index < lod.vertex_info.size());
			if(i >= 2) {
				submesh->faces.emplace_back(queue[0], queue[1], vertex_base + index);
			}
			queue[0] = queue[1];
			queue[1] = vertex_base + index;
		}
	}

#ifdef TFRAG_DEBUG_POLES
	create_debug_pole_faces(mesh, lod, tfrag, vertex_base);
#endif
}

static s32 recover_tfrag_vertices(Mesh& mesh, const TfragLod& lod, s32 strip_index) {
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
	static const u8 colours[12][4] = {
		{255, 0,   0,   255},
		{255, 255, 0,   255},
		{0,   255, 0,   255},
		{0,   255, 255, 255},
		{0,   0,   255, 255},
		{255, 0,   255, 255},
		{128, 0,   0,   255},
		{128, 128, 0,   255},
		{0,   128, 0,   255},
		{0,   128, 128, 255},
		{0,   0,   128, 255},
		{128, 0,   128, 255}
	};
#endif
	
	s32 vertex_base = (s32) mesh.vertices.size();
	for(const TfragVertexInfo& src : lod.vertex_info) {
		Vertex& dest = mesh.vertices.emplace_back();
		s16 index = src.vertex_data_offsets[1] / 2;
		verify_fatal(index >= 0 && index < lod.positions.size());
		const TfragVertexPosition& pos = lod.positions[index];
		dest.pos.x = (lod.base_position.vif1_r0 + pos.x) / 1024.f;
		dest.pos.y = (lod.base_position.vif1_r1 + pos.y) / 1024.f;
		dest.pos.z = (lod.base_position.vif1_r2 + pos.z) / 1024.f;
		dest.tex_coord.s = vu_fixed12_to_float(src.s);
		dest.tex_coord.t = vu_fixed12_to_float(src.t);
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
		dest.colour.r = colours[strip_index % 12][0];
		dest.colour.g = colours[strip_index % 12][1];
		dest.colour.b = colours[strip_index % 12][2];
		dest.colour.a = colours[strip_index % 12][3];
#elif defined(TFRAG_DEBUG_POLES)
		dest.colour.r = 255;
		dest.colour.g = 255;
		dest.colour.b = 255;
		dest.colour.a = 255;
#endif
	}
	
#ifdef TFRAG_DEBUG_POLES
	create_debug_pole_vertices(mesh, lod, strip_index);
#endif
	
	return vertex_base;
}

static void create_debug_pole_vertices(Mesh& mesh, const TfragLod& lod, s32 strip_index) {
	// Parent-child relationships.
	for(s32 elevation = 0; elevation < 2; elevation++) {
		for(const TfragVertexPosition& pos : lod.positions) {
			Vertex& dest = mesh.vertices.emplace_back();
			dest.pos.x = (lod.base_position.vif1_r0 + pos.x) / 1024.f;
			dest.pos.y = (lod.base_position.vif1_r1 + pos.y) / 1024.f;
			dest.pos.z = (lod.base_position.vif1_r2 + pos.z) / 1024.f + (elevation ? 0.5f : 0.f);
			dest.tex_coord.s = 0;
			dest.tex_coord.t = 0;
			dest.colour.r = 0;
			dest.colour.g = 0;
			dest.colour.b = 255;
			dest.colour.a = 255;
		}
	}
	
	// Vertical poles.
	for(s32 wideness = 0; wideness < 3; wideness++) {
		static const f32 offsets[3] = {0.05f, 0.15f, 0.4f};
		for(s32 colour = 0; colour < 3; colour++) {
			for(s32 side = 0; side < 2; side++) {
				for(const TfragVertexInfo& src : lod.vertex_info) {
					Vertex& dest = mesh.vertices.emplace_back();
					s16 index = src.vertex_data_offsets[1] / 2;
					verify_fatal(index >= 0 && index < lod.positions.size());
					const TfragVertexPosition& pos = lod.positions[index];
					dest.pos.x = (lod.base_position.vif1_r0 + pos.x) / 1024.f + (offsets[wideness] * ((side == 0) ? -1 : 1));
					dest.pos.y = (lod.base_position.vif1_r1 + pos.y) / 1024.f + (offsets[wideness] * ((side == 0) ? -1 : 1));
					dest.pos.z = (lod.base_position.vif1_r2 + pos.z) / 1024.f + 1.f;
					dest.tex_coord.s = 0;
					dest.tex_coord.t = 0;
					dest.colour.r = (colour == 0) ? 255 : 0;
					dest.colour.g = (colour == 1) ? 255 : 0;
					dest.colour.b = (colour == 2) ? 255 : 0;
					dest.colour.a = 255;
				}
			}
		}
	}
}

#define POLE_VERTEX_INDEX(wideness, side, colour, vertex_count) \
	lod.vertex_info.size() + \
	2*lod.positions.size() + \
	lod.vertex_info.size() * (2*3*wideness + 2*colour + side) // poles

static void create_debug_pole_faces(Mesh& mesh, const TfragLod& lod, const Tfrag& tfrag, s32 vertex_base) {
	SubMesh& pole_submesh = mesh.submeshes.emplace_back();
	pole_submesh.material = 0;
	for(size_t i = 0; i < lod.vertex_info.size(); i++) {
		// Parent-child relationships.
		if(lod.vertex_info[i].vertex_data_offsets[0] != 4096) {
			s32 pos_0 = lod.vertex_info[i].vertex_data_offsets[0] / 2;
			s32 pos_1 = lod.vertex_info[i].vertex_data_offsets[1] / 2;
			Face& face_pc = pole_submesh.faces.emplace_back();
			face_pc.v0 = vertex_base + lod.vertex_info.size() + pos_0;
			face_pc.v1 = vertex_base + lod.vertex_info.size() + pos_1;
			face_pc.v2 = vertex_base + lod.vertex_info.size() + lod.positions.size() + pos_1;
		}
		
		// Vertical poles.
		s32 wideness = 0;
		if(i < tfrag.common_vertex_info.size()) {
			wideness = 2;
		} else if(i < tfrag.common_vertex_info.size() + tfrag.lod_01_vertex_info.size()) {
			wideness = 1;
		} else {
			wideness = 0;
		}
		s32 colour = 0;
		for(u8 unk : lod.unknown_indices) {
			if(unk == i) {
				colour = 1;
				break;
			}
		}
		Face& face_vp = pole_submesh.faces.emplace_back();
		face_vp.v0 = vertex_base + i;
		face_vp.v1 = vertex_base + POLE_VERTEX_INDEX(wideness, 0, colour, lod.vertex_info.size()) + i;
		face_vp.v2 = vertex_base + POLE_VERTEX_INDEX(wideness, 1, colour, lod.vertex_info.size()) + i;
	}
}