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

#include "moby_vertex.h"

#include <engine/moby_packet.h>

namespace MOBY {
	
static void pack_common_attributes(MobyVertex& dest, const Vertex& src, f32 inverse_scale);

std::vector<MobyVertex> read_vertices(Buffer src, const MobyPacketEntry& entry, const MobyVertexTableHeaderRac1& header, MobyFormat format) {
	s64 vertex_ofs = entry.vertex_offset + header.vertex_table_offset;
	s32 in_file_vertex_count = header.two_way_blend_vertex_count + header.three_way_blend_vertex_count + header.main_vertex_count;
	std::vector<MobyVertex> vertices = src.read_multiple<MobyVertex>(vertex_ofs, in_file_vertex_count, "vertex table").copy();
	vertex_ofs += in_file_vertex_count * 0x10;
	
	// Fix vertex indices (see comment in write_vertices).
	for(size_t i = 7; i < vertices.size(); i++) {
		vertices[i - 7].v.i.low_halfword = (vertices[i - 7].v.i.low_halfword & ~0x1ff) | (vertices[i].v.i.low_halfword & 0x1ff);
	}
	s32 trailing_vertex_count = 0;
	if(format == MobyFormat::RAC1) {
		trailing_vertex_count = (header.unknown_e - header.vertex_table_offset) / 0x10 - in_file_vertex_count;
	} else {
		trailing_vertex_count = entry.vertex_data_size - header.vertex_table_offset / 0x10 - in_file_vertex_count;
	}
	verify(trailing_vertex_count < 7, "Bad moby vertex table.");
	vertex_ofs += std::max(7 - in_file_vertex_count, 0) * 0x10;
	for(s64 i = std::max(7 - in_file_vertex_count, 0); i < trailing_vertex_count; i++) {
		MobyVertex vertex = src.read<MobyVertex>(vertex_ofs, "vertex table");
		vertex_ofs += 0x10;
		s64 dest_index = in_file_vertex_count + i - 7;
		vertices.at(dest_index).v.i.low_halfword = (vertices[dest_index].v.i.low_halfword & ~0x1ff) | (vertex.v.i.low_halfword & 0x1ff);
	}
	MobyVertex last_vertex = src.read<MobyVertex>(vertex_ofs - 0x10, "vertex table");
	for(s32 i = std::max(7 - in_file_vertex_count - trailing_vertex_count, 0); i < 6; i++) {
		s64 dest_index = in_file_vertex_count + trailing_vertex_count + i - 7;
		if(dest_index < vertices.size()) {
			vertices[dest_index].v.i.low_halfword = (vertices[dest_index].v.i.low_halfword & ~0x1ff) | (last_vertex.trailing.vertex_indices[i] & 0x1ff);
		}
	}
	
	return vertices;
}

MobyVertexTableHeaderRac1 write_vertices(OutBuffer& dest, const MobyPacket& packet, const MobyPacketLowLevel& low, MobyFormat format) {
	s64 vertex_header_ofs;
	if(format == MobyFormat::RAC1) {
		vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac1>();
	} else {
		vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac23DL>();
	}
	
	MobyVertexTableHeaderRac1 vertex_header;
	vertex_header.matrix_transfer_count = low.preloop_matrix_transfers.size();
	vertex_header.two_way_blend_vertex_count = low.two_way_blend_vertex_count;
	vertex_header.three_way_blend_vertex_count = low.three_way_blend_vertex_count;
	vertex_header.main_vertex_count = low.main_vertex_count;
	
	std::vector<MobyVertex> vertices = low.vertices;
	dest.write_multiple(low.preloop_matrix_transfers);
	dest.pad(0x8);
	for(u16 dupe : packet.duplicate_vertices) {
		dest.write<u16>(dupe << 7);
	}
	vertex_header.duplicate_vertex_count = packet.duplicate_vertices.size();
	dest.pad(0x10);
	vertex_header.vertex_table_offset = dest.tell() - vertex_header_ofs;
	
	// Write out the remaining vertex indices after the rest of the proper
	// vertices (since the vertex index stored in each vertex corresponds to
	// the vertex 7 vertices prior for some reason). The remaining indices
	// are written out into the padding vertices and then when that space
	// runs out they're written into the second part of the last padding
	// vertex (hence there is at least one padding vertex). Now I see why
	// they call it Insomniac Games.
	std::vector<u16> trailing_vertex_indices(std::max(7 - (s32) vertices.size(), 0), 0);
	for(s32 i = std::max((s32) vertices.size() - 7, 0); i < vertices.size(); i++) {
		trailing_vertex_indices.push_back(vertices[i].v.i.low_halfword & 0x1ff);
	}
	for(s32 i = vertices.size() - 1; i >= 7; i--) {
		vertices[i].v.i.low_halfword = (vertices[i].v.i.low_halfword & ~0x1ff) | (vertices[i - 7].v.i.low_halfword & 0xff);
	}
	for(s32 i = 0; i < std::min(7, (s32) vertices.size()); i++) {
		vertices[i].v.i.low_halfword = vertices[i].v.i.low_halfword & ~0x1ff;
	}
	
	s32 trailing = 0;
	for(; vertices.size() % 4 != 2 && trailing < trailing_vertex_indices.size(); trailing++) {
		MobyVertex vertex = {0};
		if(packet.vertices.size() + trailing >= 7) {
			vertex.v.i.low_halfword = trailing_vertex_indices[trailing];
		}
		vertices.push_back(vertex);
	}
	verify_fatal(trailing < trailing_vertex_indices.size());
	MobyVertex last_vertex = {0};
	if(packet.vertices.size() + trailing >= 7) {
		last_vertex.v.i.low_halfword = trailing_vertex_indices[trailing];
	}
	for(s32 i = trailing + 1; i < trailing_vertex_indices.size(); i++) {
		if(packet.vertices.size() + i >= 7) {
			last_vertex.trailing.vertex_indices[i - trailing - 1] = trailing_vertex_indices[i];
		}
	}
	vertices.push_back(last_vertex);
	
	// Write all the vertices.
	dest.write_multiple(vertices);
	
	// Fill in rest of the vertex header.
	vertex_header.transfer_vertex_count =
		vertex_header.two_way_blend_vertex_count +
		vertex_header.three_way_blend_vertex_count +
		vertex_header.main_vertex_count +
		vertex_header.duplicate_vertex_count;
	vertex_header.unknown_e = packet.unknown_e;
	
	if(format == MobyFormat::RAC1) {
		vertex_header.unknown_e = dest.tell() - vertex_header_ofs;
		dest.write_multiple(packet.unknown_e_data);
		dest.write(vertex_header_ofs, vertex_header);
	} else {
		MobyVertexTableHeaderRac23DL compact_vertex_header;
		compact_vertex_header.matrix_transfer_count = vertex_header.matrix_transfer_count;
		compact_vertex_header.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
		compact_vertex_header.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
		compact_vertex_header.main_vertex_count = vertex_header.main_vertex_count;
		compact_vertex_header.duplicate_vertex_count = vertex_header.duplicate_vertex_count;
		compact_vertex_header.transfer_vertex_count = vertex_header.transfer_vertex_count;
		compact_vertex_header.vertex_table_offset = vertex_header.vertex_table_offset;
		compact_vertex_header.unknown_e = vertex_header.unknown_e;
		dest.write(vertex_header_ofs, compact_vertex_header);
	}
	
	return vertex_header;
}

std::vector<Vertex> unpack_vertices(const MobyPacketLowLevel& src, Opt<SkinAttributes> blend_buffer[64], f32 scale) {
	std::vector<Vertex> vertices;
	vertices.reserve(src.vertices.size());
	for(size_t i = 0; i < src.vertices.size(); i++) {
		const MobyVertex& vertex = src.vertices[i];
		
		f32 px = vertex.v.x * (scale / 1024.f);
		f32 py = vertex.v.y * (scale / 1024.f);
		f32 pz = vertex.v.z * (scale / 1024.f);
		
		// The normals are stored in spherical coordinates, then there's a
		// cosine/sine lookup table at the top of the scratchpad.
		f32 normal_azimuth_radians = vertex.v.normal_angle_azimuth * (WRENCH_PI / 128.f);
		f32 normal_elevation_radians = vertex.v.normal_angle_elevation * (WRENCH_PI / 128.f);
		f32 cos_azimuth = cosf(normal_azimuth_radians);
		f32 sin_azimuth = sinf(normal_azimuth_radians);
		f32 cos_elevation = cosf(normal_elevation_radians);
		f32 sin_elevation = sinf(normal_elevation_radians);
		
		// This bit is done on VU0.
		f32 nx = sin_azimuth * cos_elevation;
		f32 ny = cos_azimuth * cos_elevation;
		f32 nz = sin_elevation;
		
		s32 two_way_count = src.two_way_blend_vertex_count;
		s32 three_way_count = src.three_way_blend_vertex_count;
		SkinAttributes skin = read_skin_attributes(blend_buffer, vertex, i, two_way_count, three_way_count);
		
		vertices.emplace_back(glm::vec3(px, py, pz), glm::vec3(nx, ny, nz), skin);
		vertices.back().vertex_index = vertex.v.i.low_halfword & 0x1ff;
	}
	return vertices;
}

MobyPacketLowLevel pack_vertices(s32 smi, const MobyPacket& packet, VU0MatrixAllocator& mat_alloc, const std::vector<MatrixLivenessInfo>& liveness, f32 scale) {
	MobyPacketLowLevel dest{packet};
	dest.index_mapping.resize(packet.vertices.size());
	
	f32 inverse_scale = 1024.f / scale;
	
	std::vector<bool> first_uses(packet.vertices.size(), false);
	
	// Pack vertices that should issue a 2-way matrix blend operation on VU0.
	for(size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		if(vertex.skin.count == 2) {
			MatrixAllocation allocation;
			if(liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation(vertex.skin, smi);
				if(alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if(allocation.first_use) {
				first_uses[i] = true;
				dest.two_way_blend_vertex_count++;
				dest.index_mapping[i] = dest.vertices.size();
				
				MobyVertex mv = {0};
				mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
				pack_common_attributes(mv, vertex, inverse_scale);
				
				SkinAttributes load_1 = {1, {vertex.skin.joints[0], 0, 0}, {255, 0, 0}};
				SkinAttributes load_2 = {1, {vertex.skin.joints[1], 0, 0}, {255, 0, 0}};
				
				auto alloc_1 = mat_alloc.get_allocation(load_1, smi);
				auto alloc_2 = mat_alloc.get_allocation(load_2, smi);
				verify_fatal(alloc_1 && alloc_2);
				
				mv.v.two_way_blend.vu0_matrix_load_addr_1 = alloc_1->address;
				mv.v.two_way_blend.vu0_matrix_load_addr_2 = alloc_2->address;
				mv.v.two_way_blend.weight_1 = vertex.skin.weights[0];
				mv.v.two_way_blend.weight_2 = vertex.skin.weights[1];
				mv.v.two_way_blend.vu0_transferred_matrix_store_addr = 0xf4;
				if(liveness[i].population_count > 1) {
					mv.v.two_way_blend.vu0_blended_matrix_store_addr = allocation.address;
				} else {
					mv.v.two_way_blend.vu0_blended_matrix_store_addr = 0xf4;
				}
				
				dest.vertices.emplace_back(mv);
			}
		}
	}
	
	// Pack vertices that should issue a 3-way matrix blend operation on VU0.
	for(size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		if(vertex.skin.count == 3) {
			MatrixAllocation allocation;
			if(liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation(vertex.skin, smi);
				if(alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if(allocation.first_use) {
				first_uses[i] = true;
				dest.three_way_blend_vertex_count++;
				dest.index_mapping[i] = dest.vertices.size();
				
				MobyVertex mv = {0};
				mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
				pack_common_attributes(mv, vertex, inverse_scale);
				
				SkinAttributes load_1 = {1, {vertex.skin.joints[0], 0, 0}, {255, 0, 0}};
				SkinAttributes load_2 = {1, {vertex.skin.joints[1], 0, 0}, {255, 0, 0}};
				SkinAttributes load_3 = {1, {vertex.skin.joints[2], 0, 0}, {255, 0, 0}};
				
				auto alloc_1 = mat_alloc.get_allocation(load_1, smi);
				auto alloc_2 = mat_alloc.get_allocation(load_2, smi);
				auto alloc_3 = mat_alloc.get_allocation(load_3, smi);
				verify_fatal(alloc_1 && alloc_2 && alloc_3);
				
				mv.v.three_way_blend.vu0_matrix_load_addr_1 = alloc_1->address;
				mv.v.three_way_blend.vu0_matrix_load_addr_2 = alloc_2->address;
				mv.v.three_way_blend.low_halfword |= (alloc_3->address / 2) << 9;
				mv.v.three_way_blend.weight_1 = vertex.skin.weights[0];
				mv.v.three_way_blend.weight_2 = vertex.skin.weights[1];
				mv.v.three_way_blend.weight_3 = vertex.skin.weights[2];
				if(liveness[i].population_count > 1) {
					mv.v.three_way_blend.vu0_blended_matrix_store_addr = allocation.address;
				} else {
					mv.v.three_way_blend.vu0_blended_matrix_store_addr = 0xf4;
				}
				
				dest.vertices.emplace_back(mv);
			}
		}
	}
	
	// Pack vertices that use unblended matrices.
	for(size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		if(vertex.skin.count == 1) {
			dest.main_vertex_count++;
			dest.index_mapping[i] = dest.vertices.size();
			
			auto alloc = mat_alloc.get_allocation(vertex.skin, smi);
			verify_fatal(alloc);
			
			MobyVertex mv = {0};
			mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
			pack_common_attributes(mv, vertex, inverse_scale);
			mv.v.regular.vu0_matrix_load_addr = alloc->address;
			mv.v.regular.vu0_transferred_matrix_store_addr = 0xf4;
			//mv.v.regular.unused_5 = smi;
			
			dest.vertices.emplace_back(mv);
		}
	}
	
	// Pack vertices that use previously blended matrices.
	for(size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		if(vertex.skin.count > 1 && !first_uses[i]) {
			dest.main_vertex_count++;
			dest.index_mapping[i] = dest.vertices.size();
			
			auto alloc = mat_alloc.get_allocation(vertex.skin, smi);
			verify_fatal(alloc);
			
			MobyVertex mv = {0};
			mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
			pack_common_attributes(mv, vertex, inverse_scale);
			mv.v.regular.vu0_matrix_load_addr = alloc->address;
			mv.v.regular.vu0_transferred_matrix_store_addr = 0xf4;
			//mv.v.regular.unused_5 = smi;
			
			dest.vertices.emplace_back(mv);
		}
	}
	
	return dest;
}

static void pack_common_attributes(MobyVertex& dest, const Vertex& src, f32 inverse_scale) {
	dest.v.x = roundf(src.pos.x * inverse_scale);
	dest.v.y = roundf(src.pos.y * inverse_scale);
	dest.v.z = roundf(src.pos.z * inverse_scale);
	glm::vec3 normal = glm::normalize(src.normal);
	f32 normal_angle_azimuth_radians = atan2f(normal.x, normal.y);
	f32 normal_angle_elevation_radians = asinf(normal.z);
	dest.v.normal_angle_azimuth = roundf(normal_angle_azimuth_radians * (128.f / WRENCH_PI));
	dest.v.normal_angle_elevation = roundf(normal_angle_elevation_radians * (128.f / WRENCH_PI));
	// If the normal vector is pointing vertically upwards, the azimuth doesn't
	// matter so we set it to match the behaviour of Insomniac's exporter.
	if(dest.v.normal_angle_elevation == 0x40) {
		dest.v.normal_angle_azimuth += 0x80;
	}
}

}
