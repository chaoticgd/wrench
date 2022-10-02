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
#include <core/tristrip.h>

static TriStripConstraints setup_shrub_constraints();
static f32 compute_optimal_scale(const Mesh& mesh);

ShrubClass read_shrub_class(Buffer src) {
	ShrubClass shrub;
	
	ShrubClassHeader header = src.read<ShrubClassHeader>(0, "shrub header");
	
	shrub.bounding_sphere = header.bounding_sphere;
	shrub.mip_distance = header.mip_distance;
	shrub.mode_bits = header.mode_bits;
	shrub.scale = header.scale;
	shrub.o_class = header.o_class;
	
	for(auto& entry : src.read_multiple<ShrubPacketEntry>(sizeof(ShrubClassHeader), header.packet_count, "packet entry")) {
		ShrubPacket& packet = shrub.packets.emplace_back();
		
		Buffer command_buffer = src.subbuf(entry.offset, entry.size);
		std::vector<VifPacket> command_list = read_vif_command_list(command_buffer);
		std::vector<VifPacket> unpacks = filter_vif_unpacks(command_list);
		
		verify(unpacks.size() == 3, "Wrong number of unpacks.");
		Buffer header_unpack = unpacks[0].data;
		auto packet_header = header_unpack.read<ShrubPacketHeader>(0, "packet header");
		auto gif_tags = header_unpack.read_multiple<ShrubVertexGifTag>(0x10, packet_header.gif_tag_count, "gif tags");
		auto ad_gif = header_unpack.read_multiple<ShrubTexturePrimitive>(0x10 + packet_header.gif_tag_count * 0x10, packet_header.texture_count, "gs ad data");
		
		auto part_1 = Buffer(unpacks[1].data).read_multiple<ShrubVertexPart1>(0, packet_header.vertex_count, "vertices");
		auto part_2 = Buffer(unpacks[2].data).read_multiple<ShrubVertexPart2>(0, packet_header.vertex_count, "sts");
		
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
				vertex.x = part_1[next_vertex].x;
				vertex.y = part_1[next_vertex].y;
				vertex.z = part_1[next_vertex].z;
				vertex.s = part_2[next_vertex].s;
				vertex.t = part_2[next_vertex].t;
				vertex.colour_index = part_2[next_vertex].colour_index_and_stop_cond & 0b0111111111111111;
				
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
	
	if(header.billboard_offset > 0) {
		shrub.billboard = src.read<ShrubBillboard>(header.billboard_offset, "shrub billboard");
	}
	shrub.palette = src.read_multiple<ShrubVec4>(header.palette_offset, 24, "shrub palette").copy();
	
	return shrub;
}

void write_shrub_class(OutBuffer dest, const ShrubClass& shrub) {
	s64 header_ofs = dest.alloc<ShrubClassHeader>();
	ShrubClassHeader header = {};
	
	// Fill in the header.
	header.bounding_sphere = shrub.bounding_sphere;
	header.mip_distance = shrub.mip_distance;
	header.mode_bits = shrub.mode_bits;
	header.scale = shrub.scale;
	header.o_class = shrub.o_class;
	header.packet_count = (s32) shrub.packets.size();
	
	s64 packet_list_ofs = dest.alloc_multiple<ShrubPacketEntry>(shrub.packets.size());
	
	// Write out the VIF command lists.
	for(const ShrubPacket& packet : shrub.packets) {
		ShrubPacketEntry entry;
		
		dest.pad(0x10, 0);
		s64 begin_ofs = dest.tell();
		entry.offset = begin_ofs - header_ofs;
		
		// Write command list prologue.
		dest.write<u32>(0x01000404); // stcycl
		dest.write<u32>(0x00000000); // nop
		dest.write<u32>(0x05000000); // stmod
		
		s32 offset = 0;
		
		// Transform the data into the order it will be written out in.
		ShrubPacketHeader packet_header = {};
		packet_header.vertex_offset = 1;
		std::vector<ShrubVertexGifTag> gif_tags;
		std::vector<ShrubTexturePrimitive> textures;
		std::vector<ShrubVertexPart1> part_1;
		std::vector<ShrubVertexPart2> part_2;
		for(const ShrubPrimitive& primitive : packet.primitives) {
			if(const ShrubTexturePrimitive* prim = std::get_if<ShrubTexturePrimitive>(&primitive)) {
				packet_header.texture_count++;
				packet_header.vertex_offset += 4;
				
				ShrubTexturePrimitive& texture = textures.emplace_back(*prim);
				texture.gs_packet_offset = offset;
				offset += 5;
			}
			if(const ShrubVertexPrimitive* prim = std::get_if<ShrubVertexPrimitive>(&primitive)) {
				packet_header.gif_tag_count++;
				packet_header.vertex_offset += 1;
				packet_header.vertex_count += (s32) prim->vertices.size();
				
				ShrubVertexGifTag& gif = gif_tags.emplace_back();
				gif.tag = {};
				gif.tag.set_nloop((u32) prim->vertices.size());
				gif.tag.mid = 0x303e4000;
				gif.tag.regs = 0x00000412;
				gif.gs_packet_offset = offset;
				offset += 1;
				
				for(const ShrubVertex& vertex : prim->vertices) {
					ShrubVertexPart1& p1 = part_1.emplace_back();
					p1.x = vertex.x;
					p1.y = vertex.y;
					p1.z = vertex.z;
					p1.gs_packet_offset = offset;
					ShrubVertexPart2& p2 = part_2.emplace_back();
					p2.s = vertex.s;
					p2.t = vertex.t;
					p2.unknown_4 = 0x1000;
					p2.colour_index_and_stop_cond = vertex.colour_index;
					offset += 3;
				}
			}
		}
		
		assert(gif_tags.size() >= 1);
		gif_tags.back().tag.set_eop(true);
		
		// Insert padding vertices if there are less than 6 real vertices.
		s32 vertex_count = (s32) part_1.size();
		for(s32 i = 0; i < std::max(0, 6 - vertex_count); i++) {
			part_1.emplace_back(part_1.back());
			part_2.emplace_back(part_2.back());
			packet_header.vertex_count++;
		}
		
		// Write the stopping condition bit.
		part_2[part_2.size() - 4].colour_index_and_stop_cond |= 0b1000000000000000;
		
		// Write header/gif tag/ad data unpack.
		VifCode header_code;
		header_code.interrupt = 0;
		header_code.cmd = (VifCmd) 0b1100000;
		header_code.num = 1 + packet_header.gif_tag_count + packet_header.texture_count * 4;
		header_code.unpack.vnvl = VifVnVl::V4_32;
		header_code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		header_code.unpack.usn = VifUsn::SIGNED;
		header_code.unpack.addr = 0;
		dest.write<u32>(header_code.encode_unpack());
		dest.write(packet_header);
		dest.write_multiple(gif_tags);
		dest.write_multiple(textures);
		
		dest.write<u32>(0x05000000); // stmod
		
		// Write the primary vertex table.
		VifCode part_1_code;
		part_1_code.interrupt = 0;
		part_1_code.cmd = (VifCmd) 0b1100000;
		part_1_code.num = part_1.size();
		part_1_code.unpack.vnvl = VifVnVl::V4_16;
		part_1_code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		part_1_code.unpack.usn = VifUsn::SIGNED;
		part_1_code.unpack.addr = packet_header.vertex_offset;
		dest.write<u32>(part_1_code.encode_unpack());
		dest.write_multiple(part_1);
		
		dest.write<u32>(0x05000000); // stmod
		
		// Write the secondary vertex table.
		VifCode part_2_code;
		part_2_code.interrupt = 0;
		part_2_code.cmd = (VifCmd) 0b1100000;
		part_2_code.num = part_2.size();
		part_2_code.unpack.vnvl = VifVnVl::V4_16;
		part_2_code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		part_2_code.unpack.usn = VifUsn::SIGNED;
		part_2_code.unpack.addr = packet_header.vertex_offset + part_1.size();
		dest.write<u32>(part_2_code.encode_unpack());
		dest.write_multiple(part_2);
		
		entry.size = dest.tell() - begin_ofs;
		
		dest.write(packet_list_ofs, entry);
		packet_list_ofs += sizeof(ShrubPacketEntry);
	}
	
	// Write out the billboard.
	if(shrub.billboard.has_value()) {
		dest.pad(0x10, 0);
		header.billboard_offset = (s32) (dest.tell() - header_ofs);
		dest.write(*shrub.billboard);
	}
	
	// Write out the palette.
	dest.pad(0x10, 0);
	header.palette_offset = (s32) (dest.tell() - header_ofs);
	dest.write_multiple(shrub.palette);
	
	dest.write(header_ofs, header);
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
				texture_index = prim->d4_tex0_1.data_lo;
			}
			
			if(const ShrubVertexPrimitive* prim = std::get_if<ShrubVertexPrimitive>(&primitive)) {
				if(submesh == nullptr || texture_index != submesh->material) {
					submesh = &mesh.submeshes.emplace_back();
					submesh->material = texture_index;
					max_tex_idx = std::max(max_tex_idx, texture_index);
				}
				
				size_t first_index = mesh.vertices.size();
				for(const ShrubVertex& vertex : prim->vertices) {
					f32 x = vertex.x * shrub.scale * (1.f / 1024.f);
					f32 y = vertex.y * shrub.scale * (1.f / 1024.f);
					f32 z = vertex.z * shrub.scale * (1.f / 1024.f);
					ColourAttribute colour{1,1,1,1};
					f32 s = vertex.s * (1.f / 4096.f);
					f32 t = vertex.t * (-1.f / 4096.f);
					mesh.vertices.emplace_back(glm::vec3(x, y, z), colour, glm::vec2(s, t));
				}
				
				for(size_t i = first_index; i < mesh.vertices.size() - 2; i++) {
					submesh->faces.emplace_back(i, i + 1, i + 2);
				}
			}
		}
	}
	
	for(s32 i = 0; i <= max_tex_idx; i++) {
		ColladaMaterial& mat = scene.materials.emplace_back();
		mat.name = stringf("texture_%d", i);
		mat.surface = MaterialSurface(i);
		
		scene.texture_paths.emplace_back(stringf("%d.png", i));
	}
	
	mesh = deduplicate_vertices(std::move(mesh));
	remove_zero_area_triangles(mesh);
	
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

ShrubClass build_shrub_class(const Mesh& mesh, const std::vector<Material>& materials, f32 mip_distance, u16 mode_bits, s16 o_class, Opt<ShrubBillboard> billboard) {
	ShrubClass shrub = {};
	shrub.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(mesh.vertices));
	shrub.mip_distance = mip_distance;
	shrub.mode_bits = mode_bits;
	shrub.scale = compute_optimal_scale(mesh);
	shrub.o_class = o_class;
	
	TriStripConfig config;
	// Make sure the packets that get written out aren't too big to fit in VU1 memory.
	config.constraints = setup_shrub_constraints();
	config.support_instancing = true; // Make sure extra AD GIFs are added.
	
	// Generate the strips.
	TriStripPackets output = weave_tristrips(mesh, materials, config);
	
	// Build the shrub packets.
	for(const TriStripPacket& tpacket : output.packets) {
		ShrubPacket& packet = shrub.packets.emplace_back();
		for(s32 i = 0; i < tpacket.strip_count; i++) {
			const TriStrip& strip = output.strips[tpacket.strip_begin + i];
			if(strip.material != -1) {
				const Material& material = materials[strip.material];
				verify(material.surface.type == MaterialSurfaceType::TEXTURE,
					"A shrub material does not have a texture.");
				
				ShrubTexturePrimitive& prim = packet.primitives.emplace_back().emplace<0>();
				prim.d1_tex1_1.address = GIF_AD_TEX1_1;
				prim.d1_tex1_1.data_lo = 0xff92;
				prim.d1_tex1_1.data_hi = 0x04;
				prim.d2_clamp_1.address = GIF_AD_CLAMP_1;
				prim.d3_miptbp1_1.address = GIF_AD_MIPTBP1_1;
				prim.d3_miptbp1_1.data_lo = material.surface.texture;
				prim.d4_tex0_1.address = GIF_AD_TEX0_1;
				prim.d4_tex0_1.data_lo = material.surface.texture;
			}
			ShrubVertexPrimitive& prim = packet.primitives.emplace_back().emplace<1>();
			for(s32 j = 0; j < strip.index_count; j++) {
				ShrubVertex& dest = prim.vertices.emplace_back();
				const Vertex& src = mesh.vertices.at(output.indices.at(strip.index_begin + j));
				f32 x = src.pos.x * (1.f / shrub.scale) * 1024.f;
				f32 y = src.pos.y * (1.f / shrub.scale) * 1024.f;
				f32 z = src.pos.z * (1.f / shrub.scale) * 1024.f;
				assert(x >= INT16_MIN && x <= INT16_MAX
					&& y >= INT16_MIN && y <= INT16_MAX
					&& z >= INT16_MIN && z <= INT16_MAX);
				dest.x = (s16) x;
				dest.y = (s16) y;
				dest.z = (s16) z;
				dest.s = src.tex_coord.x * 4096.f;
				dest.t = -src.tex_coord.y * 4096.f;
			}
		}
	}
	
	shrub.palette.resize(24);
	shrub.palette[0] = ShrubVec4{INT16_MAX,INT16_MAX,INT16_MAX,INT16_MAX};
	
	return shrub;
}

static TriStripConstraints setup_shrub_constraints() {
	TriStripConstraints c;
	
	// First VIF packet size
	c.num_constraints++;
	c.constant_cost[0] = 1; // header
	c.strip_cost[0] = 1; // gif tag
	c.vertex_cost[0] = 0;
	c.index_cost[0] = 0;
	c.material_cost[0] = 4; // ad data
	c.max_cost[0] = 255; // max value of num field
	
	// Second and third VIF packet sizes
	c.num_constraints++;
	c.constant_cost[1] = 0;
	c.strip_cost[1] = 0;
	c.vertex_cost[1] = 0;
	c.index_cost[1] = 1;
	c.material_cost[1] = 0;
	c.max_cost[1] = 255; // max value of num field
	
	// Unpacked data size
	// max GIF tag nloop in original files is 44
	c.num_constraints++;
	c.constant_cost[2] = 1; // header
	c.strip_cost[2] = 1; // gif tag
	c.vertex_cost[2] = 0; // non-indexed
	c.index_cost[2] = 2; // second and third unpacks
	c.material_cost[2] = 4; // ad data
	c.max_cost[2] = 118; // buffer size
	
	// GS packet size
	c.num_constraints++;
	c.constant_cost[3] = 0;
	c.strip_cost[3] = 1; // gif tag
	c.vertex_cost[3] = 0; // non-indexed
	c.index_cost[3] = 3; // st rgbaq xyzf2
	c.material_cost[3] = 5; // gif tag + ad data
	c.max_cost[3] = 168; // max GS packet size in original files
	
	return c;
}

static f32 compute_optimal_scale(const Mesh& mesh) {
	// Compute minimum axis-aligned bounding box.
	f32 xmin = 0.f, xmax = 0.f;
	f32 zmin = 0.f, zmax = 0.f;
	f32 ymin = 0.f, ymax = 0.f;
	for(const Vertex& v : mesh.vertices) {
		xmin = std::min(xmin, v.pos.x);
		ymin = std::min(ymin, v.pos.y);
		zmin = std::min(zmin, v.pos.z);
		
		xmax = std::max(xmax, v.pos.x);
		ymax = std::max(ymax, v.pos.y);
		zmax = std::max(zmax, v.pos.z);
	}
	// Find the largest vertex position component we have to represent.
	f32 required_range = std::max({
		fabsf(xmin), fabsf(xmax),
		fabsf(ymin), fabsf(ymax),
		fabsf(zmin), fabsf(zmax)});
	// Calculate a scale such that said value is quantized to the largest
	// representable value.
	return required_range * (1024.f / (INT16_MAX - 1.f));
}
