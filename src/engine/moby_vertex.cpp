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

packed_struct(RacVertexTableHeader,
	/* 0x00 */ u32 matrix_transfer_count;
	/* 0x04 */ u32 two_way_blend_vertex_count;
	/* 0x08 */ u32 three_way_blend_vertex_count;
	/* 0x0c */ u32 main_vertex_count;
	/* 0x10 */ u32 duplicate_vertex_count;
	/* 0x14 */ u32 transfer_vertex_count; // transfer_vertex_count == two_way_blend_vertex_count + three_way_blend_vertex_count + main_vertex_count + duplicate_vertex_count
	/* 0x18 */ u32 vertex_table_offset;
	/* 0x1c */ u32 unknown_e;
)

packed_struct(GcUyaDlVertexTableHeader,
	/* 0x0 */ u16 matrix_transfer_count;
	/* 0x2 */ u16 two_way_blend_vertex_count;
	/* 0x4 */ u16 three_way_blend_vertex_count;
	/* 0x6 */ u16 main_vertex_count;
	/* 0x8 */ u16 duplicate_vertex_count;
	/* 0xa */ u16 transfer_vertex_count; // transfer_vertex_count == two_way_blend_vertex_count + three_way_blend_vertex_count + main_vertex_count + duplicate_vertex_count
	/* 0xc */ u16 vertex_table_offset;
	/* 0xe */ u16 unknown_e;
)

packed_struct(MobyMetalVertexTableHeader,
	/* 0x0 */ s32 vertex_count;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 unknown_8;
	/* 0xc */ s32 unknown_c;
)
	
static void pack_common_attributes(MobyVertex& dest, const Vertex& src, f32 inverse_scale);

VertexTable read_vertex_table(Buffer src, s64 header_offset, s32 transfer_vertex_count, s32 vertex_data_size, s32 d, s32 e, MobyFormat format)
{
	VertexTable output;
	
	// Read vertex table.
	RacVertexTableHeader header;
	s64 array_ofs = header_offset;
	if (format == MobyFormat::RAC1) {
		header = src.read<RacVertexTableHeader>(header_offset, "moby vertex header");
		array_ofs += sizeof(RacVertexTableHeader);
	} else {
		auto compact_vertex_header = src.read<GcUyaDlVertexTableHeader>(header_offset, "moby vertex header");
		header.matrix_transfer_count = compact_vertex_header.matrix_transfer_count;
		header.two_way_blend_vertex_count = compact_vertex_header.two_way_blend_vertex_count;
		header.three_way_blend_vertex_count = compact_vertex_header.three_way_blend_vertex_count;
		header.main_vertex_count = compact_vertex_header.main_vertex_count;
		header.duplicate_vertex_count = compact_vertex_header.duplicate_vertex_count;
		header.transfer_vertex_count = compact_vertex_header.transfer_vertex_count;
		header.vertex_table_offset = compact_vertex_header.vertex_table_offset;
		header.unknown_e = compact_vertex_header.unknown_e;
		array_ofs += sizeof(GcUyaDlVertexTableHeader);
	}
	verify(header.vertex_table_offset / 0x10 <= vertex_data_size,
		"Bad vertex table offset or size.");
	verify(transfer_vertex_count == header.transfer_vertex_count,
		"Conflicting vertex counts.\n");
	verify(d == (0xf + transfer_vertex_count * 6) / 0x10,
		"Weird value in submodel table entry at field 0xd.\n");
	verify(e == (3 + transfer_vertex_count) / 4,
		"Weird value in submodel table entry at field 0xe.\n");
	
	s64 vertex_ofs = header_offset + header.vertex_table_offset;
	s32 in_file_vertex_count = header.two_way_blend_vertex_count + header.three_way_blend_vertex_count + header.main_vertex_count;
	output.vertices = src.read_multiple<MobyVertex>(vertex_ofs, in_file_vertex_count, "vertex table").copy();
	vertex_ofs += in_file_vertex_count * 0x10;
	
	output.preloop_matrix_transfers = src.read_multiple<MobyMatrixTransfer>(array_ofs, header.matrix_transfer_count, "vertex table").copy();
	array_ofs += header.matrix_transfer_count * sizeof(MobyMatrixTransfer);
	
	if (array_ofs % 4 != 0) {
		array_ofs += 2;
	}
	if (array_ofs % 8 != 0) {
		array_ofs += 4;
	}
	for (u16 dupe : src.read_multiple<u16>(array_ofs, header.duplicate_vertex_count, "vertex table")) {
		output.duplicate_vertices.push_back(dupe >> 7);
	}
	
	output.two_way_blend_vertex_count = header.two_way_blend_vertex_count;
	output.three_way_blend_vertex_count = header.three_way_blend_vertex_count;
	output.main_vertex_count = header.main_vertex_count;
	
	output.unknown_e = header.unknown_e;
	if (format == MobyFormat::RAC1) {
		s32 unknown_e_size = vertex_data_size * 0x10 - header.unknown_e;
		output.unknown_e_data = src.read_bytes(header_offset + header.unknown_e, unknown_e_size, "vertex table unknown_e data");
	}
	
	// Fix vertex indices (see comment in write_vertex_table).
	std::vector<MobyVertex>& vertices = output.vertices;
	for (size_t i = 7; i < vertices.size(); i++) {
		vertices[i - 7].v.i.low_halfword = (vertices[i - 7].v.i.low_halfword & ~0x1ff) | (vertices[i].v.i.low_halfword & 0x1ff);
	}
	s32 epilogue_vertex_count = 0;
	if (format == MobyFormat::RAC1) {
		epilogue_vertex_count = (header.unknown_e - header.vertex_table_offset) / 0x10 - in_file_vertex_count;
	} else {
		epilogue_vertex_count = vertex_data_size - header.vertex_table_offset / 0x10 - in_file_vertex_count;
	}
	verify(epilogue_vertex_count < 7, "Bad moby vertex table.");
	vertex_ofs += std::max(7 - in_file_vertex_count, 0) * 0x10;
	for (s64 i = std::max(7 - in_file_vertex_count, 0); i < epilogue_vertex_count; i++) {
		MobyVertex vertex = src.read<MobyVertex>(vertex_ofs, "vertex table");
		vertex_ofs += 0x10;
		s64 dest_index = in_file_vertex_count + i - 7;
		vertices.at(dest_index).v.i.low_halfword = (vertices[dest_index].v.i.low_halfword & ~0x1ff) | (vertex.v.i.low_halfword & 0x1ff);
	}
	MobyVertex last_vertex = src.read<MobyVertex>(vertex_ofs - 0x10, "vertex table");
	for (s32 i = std::max(7 - in_file_vertex_count - epilogue_vertex_count, 0); i < 6; i++) {
		s64 dest_index = in_file_vertex_count + epilogue_vertex_count + i - 7;
		if (dest_index < vertices.size()) {
			vertices[dest_index].v.i.low_halfword = (vertices[dest_index].v.i.low_halfword & ~0x1ff) | (last_vertex.epilogue.vertex_indices[i] & 0x1ff);
		}
	}
	
	return output;
}

u32 write_vertex_table(OutBuffer& dest, const VertexTable& src, MobyFormat format)
{
	s64 vertex_header_ofs;
	if (format == MobyFormat::RAC1) {
		vertex_header_ofs = dest.alloc<RacVertexTableHeader>();
	} else {
		vertex_header_ofs = dest.alloc<GcUyaDlVertexTableHeader>();
	}
	
	RacVertexTableHeader vertex_header;
	vertex_header.matrix_transfer_count = src.preloop_matrix_transfers.size();
	vertex_header.two_way_blend_vertex_count = src.two_way_blend_vertex_count;
	vertex_header.three_way_blend_vertex_count = src.three_way_blend_vertex_count;
	vertex_header.main_vertex_count = src.main_vertex_count;
	
	std::vector<MobyVertex> vertices = src.vertices;
	dest.write_multiple(src.preloop_matrix_transfers);
	dest.pad(0x8);
	for (u16 dupe : src.duplicate_vertices) {
		dest.write<u16>(dupe << 7);
	}
	vertex_header.duplicate_vertex_count = src.duplicate_vertices.size();
	dest.pad(0x10);
	vertex_header.vertex_table_offset = dest.tell() - vertex_header_ofs;
	
	// Write out the remaining vertex indices after the rest of the proper
	// vertices (since the vertex index stored in each vertex corresponds to
	// the vertex 7 vertices prior for some reason). The remaining indices
	// are written out into the padding vertices and then when that space
	// runs out they're written into the second part of the last padding
	// vertex (hence there is at least one padding vertex). Now I see why
	// they call it Insomniac Games.
	std::vector<u16> epilogue_vertex_indices(std::max(7 - (s32) vertices.size(), 0), 0);
	for (s32 i = std::max((s32) vertices.size() - 7, 0); i < vertices.size(); i++) {
		epilogue_vertex_indices.push_back(vertices[i].v.i.low_halfword & 0x1ff);
	}
	for (s32 i = vertices.size() - 1; i >= 7; i--) {
		vertices[i].v.i.low_halfword = (vertices[i].v.i.low_halfword & ~0x1ff) | (vertices[i - 7].v.i.low_halfword & 0xff);
	}
	for (s32 i = 0; i < std::min(7, (s32) vertices.size()); i++) {
		vertices[i].v.i.low_halfword = vertices[i].v.i.low_halfword & ~0x1ff;
	}
	
	s32 epilogue = 0;
	for (; vertices.size() % 4 != 2 && epilogue < epilogue_vertex_indices.size(); epilogue++) {
		MobyVertex vertex = {0};
		if (src.vertices.size() + epilogue >= 7) {
			vertex.v.i.low_halfword = epilogue_vertex_indices[epilogue];
		}
		vertices.push_back(vertex);
	}
	verify_fatal(epilogue < epilogue_vertex_indices.size());
	MobyVertex last_vertex = {};
	if (src.vertices.size() + epilogue >= 7) {
		last_vertex.v.i.low_halfword = epilogue_vertex_indices[epilogue];
	}
	for (s32 i = epilogue + 1; i < epilogue_vertex_indices.size(); i++) {
		if (src.vertices.size() + i >= 7) {
			last_vertex.epilogue.vertex_indices[i - epilogue - 1] = epilogue_vertex_indices[i];
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
	vertex_header.unknown_e = src.unknown_e;
	
	if (format == MobyFormat::RAC1) {
		vertex_header.unknown_e = dest.tell() - vertex_header_ofs;
		dest.write_multiple(src.unknown_e_data);
		dest.write(vertex_header_ofs, vertex_header);
	} else {
		GcUyaDlVertexTableHeader compact_vertex_header;
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
	
	return vertex_header.transfer_vertex_count;
}

MetalVertexTable read_metal_vertex_table(Buffer src, s64 header_offset)
{
	auto vertex_header = src.read<MobyMetalVertexTableHeader>(header_offset, "metal vertex table header");
	
	MetalVertexTable output;
	output.vertices = src.read_multiple<MetalVertex>(header_offset + 0x10, vertex_header.vertex_count, "metal vertex table").copy();
	output.unknown_4 = vertex_header.unknown_4;
	output.unknown_8 = vertex_header.unknown_8;
	output.unknown_c = vertex_header.unknown_c;
	return output;
}

u32 write_metal_vertex_table(OutBuffer& dest, const MetalVertexTable& src)
{
	MobyMetalVertexTableHeader vertex_header;
	vertex_header.vertex_count = src.vertices.size();
	vertex_header.unknown_4 = src.unknown_4;
	vertex_header.unknown_8 = src.unknown_8;
	vertex_header.unknown_c = src.unknown_c;
	dest.write(vertex_header);
	dest.write_multiple(src.vertices);
	return (u32) vertex_header.vertex_count;
}

std::vector<Vertex> unpack_vertices(
	const VertexTable& input, Opt<SkinAttributes> blend_cache[64], f32 scale, bool animated)
{
	std::vector<Vertex> output;
	output.resize(input.vertices.size());
	
	prepare_skin_matrices(input.preloop_matrix_transfers, blend_cache, animated);
	
	for (size_t i = 0; i < input.vertices.size(); i++) {
		const MobyVertex& src = input.vertices[i];
		Vertex& dest = output[i];
		
		dest.pos.x = src.v.x * (scale / 1024.f);
		dest.pos.y = src.v.y * (scale / 1024.f);
		dest.pos.z = src.v.z * (scale / 1024.f);
		
		// The normals are stored in spherical coordinates, then there's a
		// cosine/sine lookup table at the top of the scratchpad.
		f32 normal_azimuth_radians = src.v.normal_angle_azimuth * (WRENCH_PI / 128.f);
		f32 normal_elevation_radians = src.v.normal_angle_elevation * (WRENCH_PI / 128.f);
		f32 cos_azimuth = cosf(normal_azimuth_radians);
		f32 sin_azimuth = sinf(normal_azimuth_radians);
		f32 cos_elevation = cosf(normal_elevation_radians);
		f32 sin_elevation = sinf(normal_elevation_radians);
		
		// This bit is done on VU0.
		dest.normal.x = sin_azimuth * cos_elevation;
		dest.normal.y = cos_azimuth * cos_elevation;
		dest.normal.z = sin_elevation;
		
		s32 two_way_count = input.two_way_blend_vertex_count;
		s32 three_way_count = input.three_way_blend_vertex_count;
		dest.skin = read_skin_attributes(blend_cache, src, i, two_way_count, three_way_count);
		
		dest.vertex_index = src.v.i.low_halfword & 0x1ff;
	}
	
	return output;
}

PackVerticesOutput pack_vertices(
	s32 smi,
	const std::vector<Vertex>& input_vertices,
	VU0MatrixAllocator& mat_alloc,
	const std::vector<MatrixLivenessInfo>& liveness,
	f32 scale)
{
	PackVerticesOutput output;
	output.index_mapping.resize(input_vertices.size(), 0xff);
	
	f32 inverse_scale = 1024.f / scale;
	std::vector<bool> first_uses(input_vertices.size(), false);
	
	// Pack vertices that should issue a 2-way matrix blend operation on VU0.
	for (size_t i = 0; i < input_vertices.size(); i++) {
		const Vertex& vertex = input_vertices[i];
		if (false&&vertex.skin.count == 2) {
			MatrixAllocation allocation;
			if (liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation(vertex.skin, smi);
				if (alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if (allocation.first_use) {
				first_uses[i] = true;
				output.vertex_table.two_way_blend_vertex_count++;
				output.index_mapping[i] = output.vertex_table.vertices.size();
				
				MobyVertex& mv = output.vertex_table.vertices.emplace_back();
				mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
				pack_common_attributes(mv, vertex, inverse_scale);
				
				SkinAttributes load_1 = {1, {vertex.skin.joints[0], 0, 0}, {255, 0, 0}};
				SkinAttributes load_2 = {1, {vertex.skin.joints[1], 0, 0}, {255, 0, 0}};
				
				auto alloc_1 = mat_alloc.get_allocation(load_1, smi);
				auto alloc_2 = mat_alloc.get_allocation(load_2, smi);
				//verify_fatal(alloc_1 && alloc_2);
				
				//mv.v.two_way_blend.vu0_matrix_load_addr_1 = alloc_1->address;
				//mv.v.two_way_blend.vu0_matrix_load_addr_2 = alloc_2->address;
				//mv.v.two_way_blend.weight_1 = vertex.skin.weights[0];
				//mv.v.two_way_blend.weight_2 = vertex.skin.weights[1];
				//mv.v.two_way_blend.vu0_transferred_matrix_store_addr = 0xf4;
				//if (liveness[i].population_count > 1) {
				//	mv.v.two_way_blend.vu0_blended_matrix_store_addr = allocation.address;
				//} else {
				//	mv.v.two_way_blend.vu0_blended_matrix_store_addr = 0xf4;
				//}
			}
		}
	}
	
	// Pack vertices that should issue a 3-way matrix blend operation on VU0.
	for (size_t i = 0; i < input_vertices.size(); i++) {
		const Vertex& vertex = input_vertices[i];
		if (false&&vertex.skin.count == 3) {
			MatrixAllocation allocation;
			if (liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation(vertex.skin, smi);
				if (alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if (allocation.first_use) {
				first_uses[i] = true;
				output.vertex_table.three_way_blend_vertex_count++;
				output.index_mapping[i] = output.vertex_table.vertices.size();
				
				MobyVertex mv = output.vertex_table.vertices.emplace_back();
				mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
				pack_common_attributes(mv, vertex, inverse_scale);
				
				SkinAttributes load_1 = {1, {vertex.skin.joints[0], 0, 0}, {255, 0, 0}};
				SkinAttributes load_2 = {1, {vertex.skin.joints[1], 0, 0}, {255, 0, 0}};
				SkinAttributes load_3 = {1, {vertex.skin.joints[2], 0, 0}, {255, 0, 0}};
				
				auto alloc_1 = mat_alloc.get_allocation(load_1, smi);
				auto alloc_2 = mat_alloc.get_allocation(load_2, smi);
				auto alloc_3 = mat_alloc.get_allocation(load_3, smi);
				//verify_fatal(alloc_1 && alloc_2 && alloc_3);
				
				//mv.v.three_way_blend.vu0_matrix_load_addr_1 = alloc_1->address;
				//mv.v.three_way_blend.vu0_matrix_load_addr_2 = alloc_2->address;
				//mv.v.three_way_blend.low_halfword |= (alloc_3->address / 2) << 9;
				//mv.v.three_way_blend.weight_1 = vertex.skin.weights[0];
				//mv.v.three_way_blend.weight_2 = vertex.skin.weights[1];
				//mv.v.three_way_blend.weight_3 = vertex.skin.weights[2];
				//if (liveness[i].population_count > 1) {
				//	mv.v.three_way_blend.vu0_blended_matrix_store_addr = allocation.address;
				//} else {
				//	mv.v.three_way_blend.vu0_blended_matrix_store_addr = 0xf4;
				//}
			}
		}
	}
	
	// Pack vertices that use unblended matrices.
	for (size_t i = 0; i < input_vertices.size(); i++) {
		const Vertex& vertex = input_vertices[i];
		if (false&&vertex.skin.count == 1) {
			output.vertex_table.main_vertex_count++;
			output.index_mapping[i] = output.vertex_table.vertices.size();
			
			auto alloc = mat_alloc.get_allocation(vertex.skin, smi);
			//verify_fatal(alloc);
			
			MobyVertex& mv = output.vertex_table.vertices.emplace_back();
			//mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
			pack_common_attributes(mv, vertex, inverse_scale);
			//mv.v.regular.vu0_matrix_load_addr = alloc->address;
			//mv.v.regular.vu0_transferred_matrix_store_addr = 0xf4;
			//mv.v.regular.unused_5 = smi;
		}
	}
	
	// Pack vertices that use previously blended matrices.
	for (size_t i = 0; i < input_vertices.size(); i++) {
		const Vertex& vertex = input_vertices[i];
		if (true||vertex.skin.count > 1 && !first_uses[i]) {
			output.vertex_table.main_vertex_count++;
			output.index_mapping[i] = output.vertex_table.vertices.size();
			
			auto alloc = mat_alloc.get_allocation(vertex.skin, smi);
			//verify_fatal(alloc);
			
			MobyVertex& mv = output.vertex_table.vertices.emplace_back();
			//mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
			pack_common_attributes(mv, vertex, inverse_scale);
			//mv.v.regular.vu0_matrix_load_addr = alloc->address;
			//mv.v.regular.vu0_transferred_matrix_store_addr = 0xf4;
			//mv.v.regular.unused_5 = smi;
		}
	}
	
	return output;
}

static void pack_common_attributes(MobyVertex& dest, const Vertex& src, f32 inverse_scale)
{
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
	if (dest.v.normal_angle_elevation == 0x40) {
		dest.v.normal_angle_azimuth += 0x80;
	}
}

}
