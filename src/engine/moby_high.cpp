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

#include "moby_high.h"

#include <core/vif.h>

namespace MOBY {

struct IndexMappingRecord {
	s32 packet = -1;
	s32 index = -1; // The index of the vertex in the vertex table.
	s32 id = -1; // The index of the vertex in the intermediate buffer.
	s32 dedup_out_edge = -1; // If this vertex is a duplicate, this points to the canonical vertex.
};
static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices);

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, packet %d has bad " message ".", o_class, (s32) i);

std::vector<GLTF::Mesh> recover_packets(const std::vector<MobyPacket>& packets, const char* name, s32 o_class, s32 texture_count, f32 scale, bool animated) {
	std::vector<GLTF::Mesh> output;
	output.reserve(packets.size());
	
	Opt<Vertex> vertex_cache[512]; // The game stores this on the end of the VU1 command buffer.
	Opt<SkinAttributes> blend_cache[64]; // The game stores blended matrices in VU0 memory.
	s32 texture_index = 0;
	
	for(size_t i = 0; i < packets.size(); i++) {
		const MobyPacket& src = packets[i];
		GLTF::Mesh& dest = output.emplace_back();
		
		for(const MobyMatrixTransfer& transfer : src.vertex_table.preloop_matrix_transfers) {
			verify(transfer.vu0_dest_addr % 4 == 0, "Unaligned pre-loop joint address 0x%llx.", transfer.vu0_dest_addr);
			if(!animated && transfer.spr_joint_index == 0) {
				// If the mesh isn't animated, use the blend shape matrix (identity matrix).
				blend_cache[transfer.vu0_dest_addr / 0x4] = SkinAttributes{1, {-1, 0, 0}, {255, 0, 0}};
			} else {
				blend_cache[transfer.vu0_dest_addr / 0x4] = SkinAttributes{1, {(s8) transfer.spr_joint_index, 0, 0}, {255, 0, 0}};
			}
			VERBOSE_SKINNING(printf("preloop upload spr[%02hhx] -> %02hhx\n", transfer.spr_joint_index, transfer.vu0_dest_addr));
		}
		
		dest.vertices = unpack_vertices(src.vertex_table, blend_cache, scale);
		
		for(size_t j = 0; j < dest.vertices.size(); j++) {
			Vertex& vertex = dest.vertices[j];
			vertex_cache[vertex.vertex_index & 0x1ff] = vertex;
			
			const MobyTexCoord& tex_coord = src.sts.at(j);
			vertex.tex_coord.s = vu_fixed12_to_float(tex_coord.s);
			vertex.tex_coord.t = vu_fixed12_to_float(tex_coord.t);
		}
		
		for(u16 dupe : src.vertex_table.duplicate_vertices) {
			Opt<Vertex> v = vertex_cache[dupe];
			VERIFY_SUBMESH(v.has_value(), "duplicate vertex");
			
			const MobyTexCoord& tex_coord = src.sts.at(dest.vertices.size());
			v->tex_coord.s = vu_fixed12_to_float(tex_coord.s);
			v->tex_coord.t = vu_fixed12_to_float(tex_coord.t);
			
			dest.vertices.emplace_back(*v);
		}
		
		GLTF::MeshPrimitive* primitive = nullptr;
		s32 ad_gif_index = 0;
		
		for(size_t j = 0; j < src.vif.indices.size(); j++) {
			s8 index = src.vif.indices[j];
			
			if(index == 0) {
				// There's an extra index stored in the index header, in
				// addition to an index stored in some 0x10 byte texture unpack
				// blocks. When a texture is applied, the next index from this
				// list is used as the next vertex in the queue, but the
				// triangle with it as its last index is not actually drawn.
				s8 secret_index = src.vif.secret_indices.at(ad_gif_index);
				if(secret_index == 0) {
					// End of packet.
					VERIFY_SUBMESH(primitive && primitive->indices.size() >= 3, "index buffer");
					// The VU1 microprogram has multiple vertices in flight
					// at a time, so we need to remove the ones that
					// wouldn't have been written to the GS packet.
					primitive->indices.pop_back();
					primitive->indices.pop_back();
					primitive->indices.pop_back();
					break;
				}
				
				index = secret_index - 0x80;
				
				// Switch texture.
				texture_index = src.vif.textures.at(ad_gif_index).d3_tex0_1.data_lo;
				VERIFY_SUBMESH(texture_index >= -1, "ad gifs");
				ad_gif_index++;
			}
			
			// Test if both the current and the next index have the primitive
			// restart bit set. We need to test two indices to filter out swaps.
			if(index <= 0 && j + 1 < src.vif.indices.size() && src.vif.indices[j + 1] <= 0) {
				// New triangle strip.
				primitive = &dest.primitives.emplace_back();
				primitive->attributes_bitfield = GLTF::POSITION | GLTF::TEXCOORD_0 | GLTF::NORMAL | GLTF::JOINTS_0 | GLTF::WEIGHTS_0;
				primitive->material = Opt<s32>(texture_index);
				primitive->mode = GLTF::TRIANGLE_STRIP;
			}
			
			VERIFY_SUBMESH(primitive, "index buffer");
			VERIFY_SUBMESH((index & 0x7f) - 1 < dest.vertices.size(), "index");
			primitive->indices.emplace_back((index & 0x7f) - 1);
		}
	}
	
	return output;
}

struct RichIndex {
	u32 index;
	bool restart;
	bool is_dupe = 0;
};

static std::vector<RichIndex> fake_tristripper(const std::vector<Face>& faces) {
	std::vector<RichIndex> indices;
	for(const Face& face : faces) {
		indices.push_back({(u32) face.v0, 1u});
		indices.push_back({(u32) face.v1, 1u});
		indices.push_back({(u32) face.v2, 0u});
	}
	return indices;
}

struct MidLevelTexture {
	s32 texture;
	s32 starting_index;
};

struct MidLevelVertex {
	s32 canonical;
	s32 tex_coord;
	s32 id = 0xff;
};

struct MidLevelDuplicateVertex {
	s32 index;
	s32 tex_coord;
};

// Intermediate data structure used so the packets can be built in two
// seperate passes.
struct MidLevelSubMesh {
	std::vector<MidLevelVertex> vertices;
	std::vector<RichIndex> indices;
	std::vector<MidLevelTexture> textures;
	std::vector<MidLevelDuplicateVertex> duplicate_vertices;
};

std::vector<MobyPacket> build_packets(const Mesh& mesh, const std::vector<ColladaMaterial>& materials) {
	static const s32 MAX_SUBMESH_TEXTURE_COUNT = 4;
	static const s32 MAX_SUBMESH_STORED_VERTEX_COUNT = 97;
	static const s32 MAX_SUBMESH_TOTAL_VERTEX_COUNT = 0x7f;
	static const s32 MAX_SUBMESH_INDEX_COUNT = 196;
	
	std::vector<IndexMappingRecord> index_mappings(mesh.vertices.size());
	find_duplicate_vertices(index_mappings, mesh.vertices);
	
	// *************************************************************************
	// First pass
	// *************************************************************************
	
	std::vector<MidLevelSubMesh> mid_packets;
	MidLevelSubMesh mid;
	s32 next_id = 0;
	for(s32 i = 0; i < (s32) mesh.submeshes.size(); i++) {
		const SubMesh& high = mesh.submeshes[i];
		
		auto indices = fake_tristripper(high.faces);
		if(indices.size() < 1) {
			continue;
		}
		
		const ColladaMaterial& material = materials.at(high.material);
		s32 texture = -1;
		if(material.name.size() > 4 && memcmp(material.name.data(), "mat_", 4) == 0) {
			texture = strtol(material.name.c_str() + 4, nullptr, 10);
		} else {
			fprintf(stderr, "Invalid material '%s'.", material.name.c_str());
			continue;
		}
		
		if(mid.textures.size() >= MAX_SUBMESH_TEXTURE_COUNT || mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT) {
			mid_packets.emplace_back(std::move(mid));
			mid = MidLevelSubMesh{};
		}
		
		mid.textures.push_back({texture, (s32) mid.indices.size()});
		
		for(size_t j = 0; j < indices.size(); j++) {
			auto new_packet = [&]() {
				mid_packets.emplace_back(std::move(mid));
				mid = MidLevelSubMesh{};
				// Handle splitting the strip up between moby packets.
				if(j - 2 >= 0) {
					if(!indices[j].restart) {
						j -= 3;
						indices[j + 1].restart = 1;
						indices[j + 2].restart = 1;
					} else if(!indices[j + 1].restart) {
						j -= 2;
						indices[j + 1].restart = 1;
						indices[j + 2].restart = 1;
					} else {
						j -= 1;
					}
				} else {
					// If we tried to start a tristrip at the end of the last
					// packet but didn't push any non-restarting indices, go
					// back to the beginning of the strip.
					j = -1;
				}
			};
			
			RichIndex& r = indices[j];
			IndexMappingRecord& mapping = index_mappings[r.index];
			size_t canonical_index = r.index;
			//if(mapping.dedup_out_edge != -1) {
			//	canonical_index = mapping.dedup_out_edge;
			//}
			IndexMappingRecord& canonical = index_mappings[canonical_index];
			
			if(canonical.packet != mid_packets.size()) {
				if(mid.vertices.size() >= MAX_SUBMESH_STORED_VERTEX_COUNT) {
					new_packet();
					continue;
				}
				
				canonical.packet = mid_packets.size();
				canonical.index = mid.vertices.size();
				
				mid.vertices.push_back({(s32) r.index, (s32) r.index});
			} else if(mapping.packet != mid_packets.size()) {
				if(canonical.id == -1) {
					canonical.id = next_id++;
					mid.vertices.at(canonical.index).id = canonical.id;
				}
				mid.duplicate_vertices.push_back({canonical.id, (s32) r.index});
			}
			
			if(mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT - 4) {
				new_packet();
				continue;
			}
			
			mid.indices.push_back({(u32) canonical.index, r.restart, r.is_dupe});
		}
	}
	if(mid.indices.size() > 0) {
		mid_packets.emplace_back(std::move(mid));
	}
	
	// *************************************************************************
	// Second pass
	// *************************************************************************
	
	//std::vector<MobyPacket> low_packets;
	//for(const MidLevelSubMesh& mid : mid_packets) {
	//	MobyPacket low;
	//	
	//	for(const MidLevelVertex& vertex : mid.vertices) {
	//		const Vertex& high_vert = mesh.vertices[vertex.canonical];
	//		low.vertices.emplace_back(high_vert);
	//		
	//		const glm::vec2& tex_coord = mesh.vertices[vertex.tex_coord].tex_coord;
	//		s16 s = tex_coord.x * (INT16_MAX / 8.f);
	//		s16 t = tex_coord.y * (INT16_MAX / 8.f);
	//		low.sts.push_back({s, t});
	//	}
	//	
	//	s32 texture_index = 0;
	//	for(size_t i = 0; i < mid.indices.size(); i++) {
	//		RichIndex cur = mid.indices[i];
	//		u8 out;
	//		if(cur.is_dupe) {
	//			out = mid.vertices.size() + cur.index;
	//		} else {
	//			out = cur.index;
	//		}
	//		if(texture_index < mid.textures.size() && mid.textures.at(texture_index).starting_index >= i) {
	//			verify_fatal(cur.restart);
	//			low.indices.push_back(0);
	//			low.secret_indices.push_back(out + 1);
	//			texture_index++;
	//		} else {
	//			low.indices.push_back(cur.restart ? (out + 0x81) : (out + 1));
	//		}
	//	}
	//	
	//	// These fake indices are required to signal to the microprogram that it
	//	// should terminate.
	//	low.indices.push_back(1);
	//	low.indices.push_back(1);
	//	low.indices.push_back(1);
	//	low.indices.push_back(0);
	//	
	//	for(const MidLevelTexture& tex : mid.textures) {
	//		MobyTexturePrimitive primitive = {0};
	//		primitive.d1_tex1_1.data_lo = 0xff92; // Not sure.
	//		primitive.d1_tex1_1.data_hi = 0x4;
	//		primitive.d1_tex1_1.address = GIF_AD_TEX1_1;
	//		primitive.d1_tex1_1.pad_a = 0x41a0;
	//		primitive.d2_clamp_1.address = GIF_AD_CLAMP_1;
	//		primitive.d3_tex0_1.address = GIF_AD_TEX0_1;
	//		primitive.d3_tex0_1.data_lo = tex.texture;
	//		primitive.d4_miptbp1_1.address = 0x34;
	//		low.textures.push_back(primitive);
	//	}
	//	
	//	for(const MidLevelDuplicateVertex& dupe : mid.duplicate_vertices) {
	//		low.duplicate_vertices.push_back(dupe.index);
	//		
	//		const glm::vec2& tex_coord = mesh.vertices[dupe.tex_coord].tex_coord;
	//		s16 s = vu_float_to_fixed12(tex_coord.s);
	//		s16 t = vu_float_to_fixed12(tex_coord.t);
	//		low.sts.push_back({s, t});
	//	}
	//	
	//	low_packets.emplace_back(std::move(low));
	//}
	//
	//return low_packets;
	return {};
}

static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices) {
	std::vector<size_t> indices(vertices.size());
	for(size_t i = 0; i < vertices.size(); i++) {
		indices[i] = i;
	}
	std::sort(BEGIN_END(indices), [&](size_t l, size_t r) {
		return vertices[l] < vertices[r];
	});
	
	for(size_t i = 1; i < indices.size(); i++) {
		const Vertex& prev = vertices[indices[i - 1]];
		const Vertex& cur = vertices[indices[i]];
		if(vec3_equal_eps(prev.pos, cur.pos) && vec3_equal_eps(prev.normal, cur.normal)) {
			size_t vert = indices[i - 1];
			if(index_mapping[vert].dedup_out_edge != -1) {
				vert = index_mapping[vert].dedup_out_edge;
			}
			index_mapping[indices[i]].dedup_out_edge = vert;
		}
	}
}

GLTF::Mesh recover_mesh(std::vector<GLTF::Mesh>& packets, Opt<std::string> name) {
	GLTF::Mesh output;
	output.name = name;
	
	for(GLTF::Mesh& packet : packets) {
		u32 vertex_base = (u32) output.vertices.size();
		output.vertices.insert(output.vertices.end(), BEGIN_END(packet.vertices));
		for(GLTF::MeshPrimitive& primitive : packet.primitives) {
			for(u32& index : primitive.indices) {
				index += vertex_base;
			}
			output.primitives.emplace_back(std::move(primitive));
		}
	}
	
	GLTF::deduplicate_vertices(output);
	
	return output;
}

std::vector<GLTF::Mesh> build_mesh(const GLTF::Mesh& mesh) {
	return {};
}

}
