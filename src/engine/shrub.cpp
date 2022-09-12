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

#include "shrub.h"

#include <core/vif.h>

ShrubClass read_shrub_class(Buffer src) {
	ShrubClass shrub;
	
	ShrubClassHeader header = src.read<ShrubClassHeader>(0, "shrub header");
	
	for(auto& entry : src.read_multiple<ShrubPacketEntry>(sizeof(ShrubClassHeader), header.packet_count, "packet entry")) {
		ShrubPacket& packet = shrub.packets.emplace_back();
		
		Buffer command_buffer = src.subbuf(entry.offset, entry.size);
		std::vector<VifPacket> command_list = read_vif_command_list(command_buffer);
		std::vector<VifPacket> unpacks = filter_vif_unpacks(command_list);
		
		verify(unpacks.size() == 3, "Wrong number of unpacks.");
		Buffer header_unpack = unpacks[0].data;
		auto header = header_unpack.read<ShrubPacketHeader>(0, "packet header");
		auto gif_tags = header_unpack.read_multiple<ShrubVertexGifTag>(0x10, header.gif_tag_count, "gif tags");
		auto ad_gif = header_unpack.read_multiple<ShrubTexturePrimitive>(0x10 + header.gif_tag_count * 0x10, header.texture_count, "gs ad data");
		
		auto part_1 = Buffer(unpacks[1].data).read_multiple<ShrubVertexPart1>(0, header.vertex_count, "vertices");
		auto part_2 = Buffer(unpacks[2].data).read_multiple<ShrubVertexPart2>(0, header.vertex_count, "sts");
		
		ShrubVertexPrimitive* prim = nullptr;
		
		// Interpret the data in the order it would be written to the GS packet.
		s32 next_gif_tag = 0;
		s32 next_ad_gif = 0;
		s32 next_vertex = 0;
		s32 next_offset = 0;
		while(next_gif_tag < gif_tags.size() || next_ad_gif < ad_gif.size() || next_vertex < part_1.size()) {
			// GIF tags for the vertices (not the AD data).
			if(next_gif_tag < gif_tags.size() && gif_tags[next_gif_tag].gs_packet_offset == next_offset) {
				prim = nullptr;
				
				next_gif_tag++;
				next_offset += 1;
				
				continue;
			}
			
			// AD data to change the texture.
			if(next_ad_gif < ad_gif.size() && ad_gif[next_ad_gif].gs_packet_offset == next_offset) {
				packet.primitives.emplace_back(ad_gif[next_ad_gif]);
				prim = nullptr;
				
				next_ad_gif++;
				next_offset += 5;
				
				continue;
			}
			
			// Normal vertices.
			if(next_vertex < part_1.size() && part_1[next_vertex].gs_packet_offset == next_offset) {
				if(prim == nullptr) {
					prim = &packet.primitives.emplace_back().emplace<1>();
				}
				
				ShrubVertex& vertex = prim->vertices.emplace_back();
				vertex.part_1 = part_1[next_vertex];
				vertex.part_2 = part_2[next_vertex];
				
				verify(vertex.part_2.unknown_4 == 0x1000, "Weird shrub data.");
				
				next_vertex++;
				next_offset += 3;
				
				continue;
			}
			
			// Padding vertices at the end of a small packet, all of which write over the same address.
			if(next_vertex < part_1.size() && part_1[next_vertex].gs_packet_offset == next_offset - 3) {
				break;
			}
			
			verify_not_reached("Bad shrub data.");
		}
	}
	
	shrub.palette = src.read_multiple<ShrubVec4>(header.palette_offset, 24, "shrub palette").copy();
	shrub.bounding_sphere = header.bounding_sphere;
	shrub.scale = header.scale;
	
	return shrub;
}

void write_shrub_class(OutBuffer dest, const ShrubClass& shrub) {
	
}

ColladaScene recover_shrub_class(const ShrubClass& shrub) {
	ColladaScene scene;
	
	Mesh& mesh = scene.meshes.emplace_back();
	mesh.name = "mesh";
	mesh.flags |= MESH_HAS_TEX_COORDS;
	mesh.flags |= MESH_HAS_VERTEX_COLOURS;
	
	s32 texture_index = -1;
	s32 max_tex_idx = 0;
	SubMesh* submesh = nullptr;
	
	for(const ShrubPacket& packet : shrub.packets) {
		for(const ShrubPrimitive& primitive : packet.primitives) {
			if(const ShrubTexturePrimitive* prim = std::get_if<ShrubTexturePrimitive>(&primitive)) {
				texture_index = prim->xyzf2_2.data_lo;
			}
			
			if(const ShrubVertexPrimitive* prim = std::get_if<ShrubVertexPrimitive>(&primitive)) {
				if(submesh == nullptr || texture_index != submesh->material) {
					submesh = &mesh.submeshes.emplace_back();
					submesh->material = texture_index;
					max_tex_idx = std::max(max_tex_idx, texture_index);
				}
				
				size_t first_index = mesh.vertices.size();
				
				for(const ShrubVertex& vertex : prim->vertices) {
					f32 x = vertex.part_1.x * shrub.scale * (1.f / 256.f);
					f32 y = vertex.part_1.y * shrub.scale * (1.f / 256.f);
					f32 z = vertex.part_1.z * shrub.scale * (1.f / 256.f);
					ColourAttribute colour{1,1,1,1};
					f32 s = vertex.part_2.s * (1.f / 4096.f);
					f32 t = vertex.part_2.t * (1.f / 4096.f);
					mesh.vertices.emplace_back(glm::vec3(x, y, z), colour, glm::vec2(s, t));
				}
				
				for(size_t i = first_index; i < mesh.vertices.size() - 2; i++) {
					submesh->faces.emplace_back(i, i + 1, i + 2);
				}
			}
		}
	}
	
	for(s32 i = 0; i <= max_tex_idx; i++) {
		Material& mat = scene.materials.emplace_back();
		mat.name = stringf("texture_%d", i);
		mat.texture = i;
		
		scene.texture_paths.emplace_back(stringf("%d.png", i));
	}
	
	Mesh& bsphere_ind = scene.meshes.emplace_back();
	bsphere_ind.name="bpshere";
	SubMesh& sub = bsphere_ind.submeshes.emplace_back();
	auto bs = shrub.bounding_sphere;
	sub.material = 0;
	bsphere_ind.vertices.emplace_back(glm::vec3(bs.x + bs.w, bs.y, bs.z));
	bsphere_ind.vertices.emplace_back(glm::vec3(bs.x - bs.w, bs.y, bs.z));
	bsphere_ind.vertices.emplace_back(glm::vec3(bs.x, bs.y + bs.w, bs.z));
	bsphere_ind.vertices.emplace_back(glm::vec3(bs.x, bs.y - bs.w, bs.z));
	bsphere_ind.vertices.emplace_back(glm::vec3(bs.x, bs.y, bs.z + bs.w));
	bsphere_ind.vertices.emplace_back(glm::vec3(bs.x, bs.y, bs.z - bs.w));
	sub.faces.emplace_back(0, 0, 0);
	sub.faces.emplace_back(1, 1, 1);
	sub.faces.emplace_back(2, 2, 2);
	sub.faces.emplace_back(3, 3, 3);
	sub.faces.emplace_back(4, 4, 4);
	sub.faces.emplace_back(5, 5, 5);
	
	
	return scene;
}
