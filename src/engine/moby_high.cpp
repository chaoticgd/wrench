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
#include <core/tristrip_packet.h>

namespace MOBY {

static std::vector<TriStripConstraint> setup_moby_constraints();

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, packet %d has bad " message ".", o_class, (s32) i);

std::vector<GLTF::Mesh> recover_packets(const std::vector<MobyPacket>& packets, s32 o_class, f32 scale, bool animated) {
	std::vector<GLTF::Mesh> output;
	output.reserve(packets.size());
	
	Opt<Vertex> vertex_cache[512]; // The game stores this on the end of the VU1 command buffer.
	Opt<SkinAttributes> blend_cache[64]; // The game stores blended matrices in VU0 memory.
	s32 texture_index = 0;
	
	for(size_t i = 0; i < packets.size(); i++) {
		const MobyPacket& src = packets[i];
		GLTF::Mesh& dest = output.emplace_back();
		
		dest.vertices = unpack_vertices(src.vertex_table, blend_cache, scale, animated);
		
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
			if(index <= 0) {
				if(j + 1 < src.vif.indices.size() && src.vif.indices[j + 1] <= 0) {
					// New triangle strip.
					primitive = &dest.primitives.emplace_back();
					primitive->attributes_bitfield = GLTF::POSITION | GLTF::TEXCOORD_0 | GLTF::NORMAL;
					if(animated) {
						primitive->attributes_bitfield |= GLTF::JOINTS_0 | GLTF::WEIGHTS_0;
					}
					primitive->material = Opt<s32>(texture_index);
					primitive->mode = GLTF::TRIANGLE_STRIP;
				} else {
					// Primitive restarts in the input are encoded with zero
					// area tris in the output.
					VERIFY_SUBMESH(primitive && primitive->indices.size() >= 1, "index buffer");
					primitive->indices.emplace_back(primitive->indices.back());
				}
			}
			
			VERIFY_SUBMESH(primitive, "index buffer");
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

//struct SharedVifData {
//	std::vector<s8> indices;
//	std::vector<s8> secret_indices;
//	std::vector<MobyTexturePrimitive> textures;
//	u8 index_header_first_byte = 0xff;
//};
//
//struct MobyPacket {
//	VertexTable vertex_table;
//	SharedVifData vif;
//	std::vector<MobyTexCoord> sts;
//};

std::vector<MobyPacket> build_packets(const std::vector<GLTF::Mesh> input, const std::vector<EffectiveMaterial>& effectives, const std::vector<Material>& materials, f32 scale) {
	std::vector<MobyPacket> output;
	VU0MatrixAllocator mat_alloc(max_num_joints_referenced_per_packet(input));
	std::vector<std::vector<MatrixLivenessInfo>> liveness = compute_matrix_liveness(input);
	s32 last_effective_material = -1;
	
	for(s32 i = 0; i < (s32) input.size(); i++) {
		const GLTF::Mesh& src = input[i];
		MobyPacket& dest = output.emplace_back();
		
		auto [vertex_table, index_mapping] = MOBY::pack_vertices(i, src.vertices, mat_alloc, liveness[i], scale);
		dest.vertex_table = std::move(vertex_table);
		
		dest.vif.index_header_first_byte = false;
		
		for(const GLTF::MeshPrimitive& primitive : src.primitives) {
			if(!primitive.material.has_value()) {
				continue;
			}
			
			bool use_secret_index = false;
			if(*primitive.material != last_effective_material) {
				s32 material_index = effectives.at(*primitive.material).materials.at(0);
				const Material& material = materials.at(material_index);
				verify(material.surface.type == MaterialSurfaceType::TEXTURE, "Material needs a texture.");
				
				MobyTexturePrimitive& ad_gif = dest.vif.textures.emplace_back();
				ad_gif.d1_tex1_1.data_lo = 0xff92; // lod k
				ad_gif.d1_tex1_1.data_hi = 0x4;
				ad_gif.d1_tex1_1.address = GIF_AD_TEX1_1;
				ad_gif.d1_tex1_1.pad_a = 0x41a0;
				ad_gif.d2_clamp_1.address = GIF_AD_CLAMP_1;
				ad_gif.d3_tex0_1.address = GIF_AD_TEX0_1;
				ad_gif.d3_tex0_1.data_lo = material.surface.texture;
				ad_gif.d4_miptbp1_1.address = 0x34;
				
				last_effective_material = *primitive.material;
				use_secret_index = true;
			}
			
			std::vector<s32> indices = zero_area_tris_to_restart_bit_strip(primitive.indices);
			
			for(size_t j = 0; j < indices.size(); j++) {
				s32 index = indices[j] & ~(1 << 31);
				bool restart_bit = j < 2 || (indices[j] & (1 << 31)) != 0;
				s32 mapped_index = index_mapping.at(index);
				if(use_secret_index) {
					dest.vif.indices.emplace_back(0);
					dest.vif.secret_indices.emplace_back(mapped_index + 1);
					use_secret_index = false;
				} else {
					dest.vif.indices.emplace_back((mapped_index + 1) | (restart_bit << 7));
				}
			}
		}
		
		// These fake indices are required to signal to the microprogram that it
		// should terminate.
		dest.vif.indices.push_back(1);
		dest.vif.indices.push_back(1);
		dest.vif.indices.push_back(1);
		dest.vif.indices.push_back(0);
		
		for(const Vertex& vertex : src.vertices) {
			s16 s = vu_float_to_fixed12(vertex.tex_coord.x);
			s16 t = vu_float_to_fixed12(vertex.tex_coord.y);
			dest.sts.push_back({s, t});
		}
	}
	
	return output;
}

GLTF::Mesh merge_packets(const std::vector<GLTF::Mesh>& packets, Opt<std::string> name) {
	GLTF::Mesh output;
	output.name = name;
	
	for(const GLTF::Mesh& packet : packets) {
		s32 vertex_base = (s32) output.vertices.size();
		output.vertices.insert(output.vertices.end(), BEGIN_END(packet.vertices));
		for(GLTF::MeshPrimitive primitive : packet.primitives) {
			for(s32& index : primitive.indices) {
				index += vertex_base;
			}
			output.primitives.emplace_back(std::move(primitive));
		}
	}
	
	GLTF::deduplicate_vertices(output);
	
	return output;
}

std::vector<GLTF::Mesh> split_packets(const GLTF::Mesh& mesh, const std::vector<s32>& material_to_effective, bool output_broken_indices) {
	TriStripConfig config;
	config.constraints = setup_moby_constraints();
	config.support_index_buffer = true;
	config.support_instancing = false;
	
	u32 attributes_bitfield = GLTF::POSITION | GLTF::TEXCOORD_0 | GLTF::NORMAL;
	for(const GLTF::MeshPrimitive& primitive : mesh.primitives) {
		attributes_bitfield |= primitive.attributes_bitfield;
	}
	
	TriStripPacketGenerator generator(config);
	for(const GLTF::MeshPrimitive& primitive : mesh.primitives) {
		if(!primitive.material.has_value()) {
			continue;
		}
		std::vector<s32> indices = zero_area_tris_to_restart_bit_strip(primitive.indices);
		//verify(*primitive.material >= 0 && *primitive.material < material_to_effective.size(), "Material index out of range.");
		if(!(*primitive.material >= 0 && *primitive.material < material_to_effective.size())) {
			continue;
		}
		s32 effective_material = material_to_effective.at(*primitive.material);
		generator.add_primitive(indices.data(), (s32) indices.size(), GeometryType::TRIANGLE_STRIP, effective_material);
	}
	GeometryPackets geo = generator.get_output();
	
	std::vector<GLTF::Mesh> output;
	for(const GeometryPacket& packet : geo.packets) {
		GLTF::Mesh& dest = output.emplace_back();
		for(s32 i = 0; i < packet.primitive_count; i++) {
			const GeometryPrimitive& src_primitive = geo.primitives[packet.primitive_begin + i];
			GLTF::MeshPrimitive& dest_primitive = dest.primitives.emplace_back();
			
			dest_primitive.attributes_bitfield = attributes_bitfield;
			s32* index_begin = &geo.indices[src_primitive.index_begin];
			s32* index_end = &geo.indices[src_primitive.index_begin + src_primitive.index_count];
			dest_primitive.indices = restart_bit_strip_to_zero_area_tris(std::vector<s32>(index_begin, index_end));
			dest_primitive.material = src_primitive.effective_material;
			dest_primitive.mode = GLTF::TRIANGLE_STRIP;
		}
		GLTF::filter_vertices(dest, mesh.vertices, !output_broken_indices);
	}
	
	return output;
}

static std::vector<TriStripConstraint> setup_moby_constraints() {
	std::vector<TriStripConstraint> constraints;
	
	TriStripConstraint& vu1_vertex_buffer_size = constraints.emplace_back();
	vu1_vertex_buffer_size.constant_cost = 0;
	vu1_vertex_buffer_size.strip_cost = 0;
	vu1_vertex_buffer_size.vertex_cost = 2; // position + colour
	vu1_vertex_buffer_size.index_cost = 0;
	vu1_vertex_buffer_size.material_cost = 0;
	vu1_vertex_buffer_size.max_cost = 0xc2; // buffer size
	
	TriStripConstraint& vu1_unpacked_data_size = constraints.emplace_back();
	vu1_unpacked_data_size.constant_cost = (0x12d + 1 + 1) * 16; // fixed index buffer offset + index header size + microprogram termination indices
	vu1_unpacked_data_size.strip_cost = 0;
	vu1_unpacked_data_size.vertex_cost = 0;
	vu1_unpacked_data_size.index_cost = 4; // indices
	vu1_unpacked_data_size.material_cost = 4 * 16; // ad gif
	vu1_unpacked_data_size.max_cost = 0x164 * 16; // buffer size
	vu1_unpacked_data_size.round_index_cost_up_to_multiple_of = 16;
	
	return constraints;
}

}
