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

#include "moby_mesh.h"

#include <core/vif.h>

#define VERBOSE_SKINNING(...) //__VA_ARGS__

// read_moby_submeshes
// read_moby_metal_submeshes
static void sort_moby_vertices_after_reading(MobySubMeshLowLevel& low, MobySubMesh& submesh);
static std::vector<Vertex> unpack_vertices(const MobySubMeshLowLevel& src, Opt<SkinAttributes> blend_buffer[64], f32 scale);
static SkinAttributes read_skin_attributes(Opt<SkinAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count);
static std::vector<MobyVertex> read_vertices(Buffer src, const MobySubMeshEntry& entry, const MobyVertexTableHeaderRac1& header, MobyFormat format);
// recover_moby_mesh
// map_indices

std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, f32 scale, s32 joint_count, MobyFormat format) {
	std::vector<MobySubMesh> submeshes;
	
	Opt<SkinAttributes> blend_buffer[64]; // The game stores blended matrices in VU0 memory.
	
	auto submesh_table = src.read_multiple<MobySubMeshEntry>(table_ofs, count, "moby submesh table");
	for(s32 i = 0; i < (s32) submesh_table.size(); i++) {
		VERBOSE_SKINNING(printf("******** submesh %d\n", i));
		
		const MobySubMeshEntry& entry = submesh_table[i];
		MobySubMesh submesh;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer st_data(unpacks.at(0).data);
		submesh.sts = st_data.read_multiple<MobyTexCoord>(0, st_data.size() / 4, "moby st unpack").copy();
		
		Buffer index_data(unpacks.at(1).data);
		auto index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		submesh.index_header_first_byte = index_header.unknown_0;
		verify(index_header.pad == 0, "Moby has bad index buffer.");
		submesh.secret_indices.push_back(index_header.secret_index);
		submesh.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 3) {
			Buffer texture_data(unpacks.at(2).data);
			verify(texture_data.size() % 0x40 == 0, "Moby has bad texture unpack.");
			for(size_t i = 0; i < texture_data.size() / 0x40; i++) {
				submesh.secret_indices.push_back(texture_data.read<s32>(i * 0x10 + 0xc, "extra index"));
				auto prim = texture_data.read<MobyTexturePrimitive>(i * 0x40, "moby texture primitive");
				verify(prim.d3_tex0.data_lo >= MOBY_TEX_NONE, "Regular moby submesh has a texture index that is too low.");
				submesh.textures.push_back(prim);
			}
		}
		
		// Read vertex table.
		MobyVertexTableHeaderRac1 vertex_header;
		s64 array_ofs = entry.vertex_offset;
		if(format == MobyFormat::RAC1) {
			vertex_header = src.read<MobyVertexTableHeaderRac1>(entry.vertex_offset, "moby vertex header");
			array_ofs += sizeof(MobyVertexTableHeaderRac1);
		} else {
			auto compact_vertex_header = src.read<MobyVertexTableHeaderRac23DL>(entry.vertex_offset, "moby vertex header");
			vertex_header.matrix_transfer_count = compact_vertex_header.matrix_transfer_count;
			vertex_header.two_way_blend_vertex_count = compact_vertex_header.two_way_blend_vertex_count;
			vertex_header.three_way_blend_vertex_count = compact_vertex_header.three_way_blend_vertex_count;
			vertex_header.main_vertex_count = compact_vertex_header.main_vertex_count;
			vertex_header.duplicate_vertex_count = compact_vertex_header.duplicate_vertex_count;
			vertex_header.transfer_vertex_count = compact_vertex_header.transfer_vertex_count;
			vertex_header.vertex_table_offset = compact_vertex_header.vertex_table_offset;
			vertex_header.unknown_e = compact_vertex_header.unknown_e;
			array_ofs += sizeof(MobyVertexTableHeaderRac23DL);
		}
		if(vertex_header.vertex_table_offset / 0x10 > entry.vertex_data_size) {
			printf("warning: Bad vertex table offset or size.\n");
			continue;
		}
		if(entry.transfer_vertex_count != vertex_header.transfer_vertex_count) {
			printf("warning: Conflicting vertex counts.\n");
		}
		if(entry.unknown_d != (0xf + entry.transfer_vertex_count * 6) / 0x10) {
			printf("warning: Weird value in submodel table entry at field 0xd.\n");
			continue;
		}
		if(entry.unknown_e != (3 + entry.transfer_vertex_count) / 4) {
			printf("warning: Weird value in submodel table entry at field 0xe.\n");
			continue;
		}
		
		MobySubMeshLowLevel low{submesh};
		
		low.preloop_matrix_transfers = src.read_multiple<MobyMatrixTransfer>(array_ofs, vertex_header.matrix_transfer_count, "vertex table").copy();
		for(const MobyMatrixTransfer& transfer : low.preloop_matrix_transfers) {
			verify(transfer.vu0_dest_addr % 4 == 0, "Unaligned pre-loop joint address 0x%llx.", transfer.vu0_dest_addr);
			if(joint_count == 0 && transfer.spr_joint_index == 0) {
				// If there aren't any joints, use the blend shape matrix (identity matrix).
				blend_buffer[transfer.vu0_dest_addr / 0x4] = SkinAttributes{1, {-1, 0, 0}, {255, 0, 0}};
			} else {
				blend_buffer[transfer.vu0_dest_addr / 0x4] = SkinAttributes{1, {(s8) transfer.spr_joint_index, 0, 0}, {255, 0, 0}};
			}
			VERBOSE_SKINNING(printf("preloop upload spr[%02hhx] -> %02hhx\n", transfer.spr_joint_index, transfer.vu0_dest_addr));
		}
		
		array_ofs += vertex_header.matrix_transfer_count * 2;
		if(array_ofs % 4 != 0) {
			array_ofs += 2;
		}
		if(array_ofs % 8 != 0) {
			array_ofs += 4;
		}
		for(u16 dupe : src.read_multiple<u16>(array_ofs, vertex_header.duplicate_vertex_count, "vertex table")) {
			submesh.duplicate_vertices.push_back(dupe >> 7);
		}
		
		low.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
		low.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
		
		low.vertices = read_vertices(src, entry, vertex_header, format);
		submesh.vertices = unpack_vertices(low, blend_buffer, scale);
		sort_moby_vertices_after_reading(low, submesh);
		
		submesh.unknown_e = vertex_header.unknown_e;
		if(format == MobyFormat::RAC1) {
			s32 unknown_e_size = entry.vertex_data_size * 0x10 - vertex_header.unknown_e;
			submesh.unknown_e_data = src.read_bytes(entry.vertex_offset + vertex_header.unknown_e, unknown_e_size, "vertex table unknown_e data");
		}
		
		submeshes.emplace_back(std::move(submesh));
	}
	return submeshes;
}

std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count) {
	std::vector<MobyMetalSubMesh> submeshes;
	for(auto& entry : src.read_multiple<MobySubMeshEntry>(table_ofs, count, "moby metal submesh table")) {
		MobyMetalSubMesh submesh;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer index_data(unpacks.at(0).data);
		auto index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		submesh.index_header_first_byte = index_header.unknown_0;
		verify(index_header.pad == 0, "Moby has bad index buffer.");
		submesh.secret_indices.push_back(index_header.secret_index);
		submesh.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 2) {
			Buffer texture_data(unpacks.at(1).data);
			verify(texture_data.size() % 0x40 == 0, "Moby has bad texture unpack.");
			for(size_t i = 0; i < texture_data.size() / 0x40; i++) {
				submesh.secret_indices.push_back(texture_data.read<s32>(i * 0x10 + 0xc, "extra index"));
				auto prim = texture_data.read<MobyTexturePrimitive>(i * 0x40, "moby texture primitive");
				verify(prim.d3_tex0.data_lo == MOBY_TEX_CHROME || prim.d3_tex0.data_lo == MOBY_TEX_GLASS,
					"Metal moby submesh has a bad texture index.");
				submesh.textures.push_back(prim);
			}
		}
		
		// Read vertex table.
		auto vertex_header = src.read<MobyMetalVertexTableHeader>(entry.vertex_offset, "metal vertex table header");
		submesh.vertices = src.read_multiple<MobyMetalVertex>(entry.vertex_offset + 0x10, vertex_header.vertex_count, "metal vertex table").copy();
		submesh.unknown_4 = vertex_header.unknown_4;
		submesh.unknown_8 = vertex_header.unknown_8;
		submesh.unknown_c = vertex_header.unknown_c;
		
		submeshes.emplace_back(std::move(submesh));
	}
	return submeshes;
}

static void sort_moby_vertices_after_reading(MobySubMeshLowLevel& low, MobySubMesh& submesh) {
	assert(low.vertices.size() == submesh.vertices.size());
	
	s32 two_way_end = low.two_way_blend_vertex_count;
	s32 three_way_end = low.two_way_blend_vertex_count + low.three_way_blend_vertex_count;
	
	s32 two_way_index = 0;
	s32 three_way_index = two_way_end;
	s32 next_mapped_index = 0;
	
	std::vector<size_t> mapping(submesh.vertices.size(), SIZE_MAX);
	
	// This rearranges the vertices into an order such that the blended matrices
	// in VU0 memory can be allocated sequentially and the resultant moby class
	// file will match the file from the original game.
	while(two_way_index < two_way_end && three_way_index < three_way_end) {
		u8 two_way_addr = low.vertices[two_way_index].v.two_way_blend.vu0_blended_matrix_store_addr;
		u8 three_way_addr = low.vertices[three_way_index].v.three_way_blend.vu0_blended_matrix_store_addr;
		
		if((two_way_addr <= three_way_addr && three_way_addr != 0xf4) || two_way_addr == 0xf4) {
			mapping[two_way_index++] = next_mapped_index++;
		} else {
			mapping[three_way_index++] = next_mapped_index++;
		}
	}
	while(two_way_index < two_way_end) {
		mapping[two_way_index++] = next_mapped_index++;
	}
	while(three_way_index < three_way_end) {
		mapping[three_way_index++] = next_mapped_index++;
	}
	assert(next_mapped_index == three_way_end);
	
	for(s32 i = three_way_end; i < (s32) low.vertices.size(); i++) {
		mapping[i] = i;
	}
	
	low.vertices = {};
	
	auto old_vertices = std::move(submesh.vertices);
	submesh.vertices = std::vector<Vertex>(old_vertices.size());
	for(size_t i = 0; i < old_vertices.size(); i++) {
		submesh.vertices[mapping[i]] = old_vertices[i];
	}
	
	map_indices(submesh, mapping);
}

static std::vector<Vertex> unpack_vertices(const MobySubMeshLowLevel& src, Opt<SkinAttributes> blend_buffer[64], f32 scale) {
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

static SkinAttributes read_skin_attributes(Opt<SkinAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count) {
	SkinAttributes attribs;
	
	auto load_skin_attribs = [&](u8 addr) {
		verify(addr % 4 == 0, "Unaligned VU0 matrix load address 0x%x.", addr);
		verify(blend_buffer[addr / 0x4].has_value(), "Matrix load from uninitialised VU0 address 0x%llx.", addr);
		return *blend_buffer[addr / 0x4];
	};
	
	auto store_skin_attribs = [&](u8 addr, SkinAttributes attribs) {
		verify(addr % 4 == 0, "Unaligned VU0 matrix store address 0x%x.", addr);
		blend_buffer[addr / 0x4] = attribs;
	};
	
	s8 bits_9_15 = (mv.v.i.low_halfword & 0b1111111000000000) >> 9;
	
	if(ind < two_way_count) {
		u8 transfer_addr = mv.v.two_way_blend.vu0_transferred_matrix_store_addr;
		s8 spr_joint_index = bits_9_15;
		store_skin_attribs(transfer_addr, SkinAttributes{1, {spr_joint_index, 0, 0}, {255, 0, 0}});
		
		verify(mv.v.two_way_blend.vu0_matrix_load_addr_1 != transfer_addr &&
			mv.v.two_way_blend.vu0_matrix_load_addr_2 != transfer_addr,
			"Loading from and storing to the same VU0 address (%02hhx) in the same loop iteration. "
			"Insomniac's exporter never does this.", transfer_addr);
		
		SkinAttributes src_1 = load_skin_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_1);
		SkinAttributes src_2 = load_skin_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_2);
		verify(src_1.count == 1 && src_2.count == 1, "Input to two-way matrix blend operation has already been blended.");
		
		u8 weight_1 = mv.v.two_way_blend.weight_1;
		u8 weight_2 = mv.v.two_way_blend.weight_2;
		
		u8 blend_addr = mv.v.two_way_blend.vu0_blended_matrix_store_addr;
		attribs = SkinAttributes{2, {src_1.joints[0], src_2.joints[0], 0}, {weight_1, weight_2, 0}};
		store_skin_attribs(blend_addr, attribs);
		
		if(transfer_addr != 0xf4) {
			VERBOSE_SKINNING(printf("upload spr[%02hhx] -> %02hhx, ", spr_joint_index, transfer_addr));
		} else {
			VERBOSE_SKINNING(printf("                      "));
		}
		
		if(blend_addr != 0xf4) {
			VERBOSE_SKINNING(printf("use blend(%02hhx,%02hhx;%02hhx,%02hhx;%02hhx,%02hhx) -> %02hhx\n",
				mv.v.two_way_blend.vu0_matrix_load_addr_1,
				mv.v.two_way_blend.vu0_matrix_load_addr_2,
				src_1.joints[0],
				src_2.joints[0],
				mv.v.two_way_blend.weight_1,
				mv.v.two_way_blend.weight_2,
				blend_addr));
		} else {
			VERBOSE_SKINNING(printf("use blend(%02hhx,%02hhx;%02hhx,%02hhx;%02hhx,%02hhx)\n",
				mv.v.two_way_blend.vu0_matrix_load_addr_1,
				mv.v.two_way_blend.vu0_matrix_load_addr_2,
				src_1.joints[0],
				src_2.joints[0],
				mv.v.two_way_blend.weight_1,
				mv.v.two_way_blend.weight_2));
		}
	} else if(ind < two_way_count + three_way_count) {
		s8 vu0_matrix_load_addr_3 = bits_9_15 * 2;
		SkinAttributes src_1 = load_skin_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_1);
		SkinAttributes src_2 = load_skin_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_2);
		SkinAttributes src_3 = load_skin_attribs(vu0_matrix_load_addr_3);
		verify(src_1.count == 1 && src_2.count == 1 && src_3.count == 1,
			"Input to three-way matrix blend operation has already been blended.");
		
		u8 weight_1 = mv.v.three_way_blend.weight_1;
		u8 weight_2 = mv.v.three_way_blend.weight_2;
		u8 weight_3 = mv.v.three_way_blend.weight_3;
		
		u8 blend_addr = mv.v.three_way_blend.vu0_blended_matrix_store_addr;
		attribs = SkinAttributes{3, {src_1.joints[0], src_2.joints[0], src_3.joints[0]}, {weight_1, weight_2, weight_3}};
		store_skin_attribs(blend_addr, attribs);
		
		if(blend_addr != 0xf4) {
			VERBOSE_SKINNING(printf("                      use blend(%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx) -> %02x\n",
				mv.v.three_way_blend.vu0_matrix_load_addr_1,
				mv.v.three_way_blend.vu0_matrix_load_addr_2,
				vu0_matrix_load_addr_3,
				src_1.joints[0],
				src_2.joints[0],
				src_3.joints[0],
				mv.v.three_way_blend.weight_1,
				mv.v.three_way_blend.weight_2,
				mv.v.three_way_blend.weight_3,
				blend_addr));
		} else {
			VERBOSE_SKINNING(printf("                      use blend(%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx)\n",
				mv.v.three_way_blend.vu0_matrix_load_addr_1,
				mv.v.three_way_blend.vu0_matrix_load_addr_2,
				vu0_matrix_load_addr_3,
				src_1.joints[0],
				src_2.joints[0],
				src_3.joints[0],
				mv.v.three_way_blend.weight_1,
				mv.v.three_way_blend.weight_2,
				mv.v.three_way_blend.weight_3));
		}
	} else {
		u8 transfer_addr = mv.v.regular.vu0_transferred_matrix_store_addr;
		s8 spr_joint_index = bits_9_15;
		store_skin_attribs(transfer_addr, SkinAttributes{1, {spr_joint_index, 0, 0}, {255, 0, 0}});
		
		verify(mv.v.regular.vu0_matrix_load_addr != transfer_addr,
			"Loading from and storing to the same VU0 address (%02hhx) in the same loop iteration. "
			"Insomniac's exporter never does this.", transfer_addr);
		
		attribs = load_skin_attribs(mv.v.regular.vu0_matrix_load_addr);
		
		if(transfer_addr != 0xf4) {
			VERBOSE_SKINNING(printf("upload spr[%02hhx] -> %02hhx, use %s %02hhx\n",
				spr_joint_index, transfer_addr,
				attribs.count > 1 ? "blended" : "joint",
				mv.v.regular.vu0_matrix_load_addr));
		} else {
			VERBOSE_SKINNING(printf("                      use %s %02hhx\n",
				attribs.count > 1 ? "blended" : "joint",
				mv.v.regular.vu0_matrix_load_addr));
		}
	}
	
	return attribs;
}

static std::vector<MobyVertex> read_vertices(Buffer src, const MobySubMeshEntry& entry, const MobyVertexTableHeaderRac1& header, MobyFormat format) {
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

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, submesh %d has bad " message ".", o_class, i);

Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, s32 submesh_filter) {
	Mesh mesh;
	mesh.name = name;
	mesh.flags = MESH_HAS_NORMALS | MESH_HAS_TEX_COORDS;
	
	Opt<Vertex> intermediate_buffer[512]; // The game stores this on the end of the VU1 chain.
	
	SubMesh dest;
	dest.material = 0;
	
	for(s32 i = 0; i < submeshes.size(); i++) {
		bool lift_submesh = !MOBY_EXPORT_SUBMESHES_SEPERATELY || submesh_filter == -1 || i == submesh_filter; // This is just for debugging.
		
		const MobySubMesh& src = submeshes[i];
		
		s32 vertex_base = mesh.vertices.size();
		
		for(size_t j = 0; j < src.vertices.size(); j++) {
			Vertex vertex = src.vertices[j];
			
			const MobyTexCoord& tex_coord = src.sts.at(mesh.vertices.size() - vertex_base);
			f32 s = tex_coord.s / (INT16_MAX / 8.f);
			f32 t = -tex_coord.t / (INT16_MAX / 8.f);
			while(s < 0) s += 1;
			while(t < 0) t += 1;
			vertex.tex_coord.s = s;
			vertex.tex_coord.t = t;
			
			intermediate_buffer[vertex.vertex_index & 0x1ff] = vertex;
			mesh.vertices.emplace_back(vertex);
		}
		
		for(u16 dupe : src.duplicate_vertices) {
			Opt<Vertex> v = intermediate_buffer[dupe];
			VERIFY_SUBMESH(v.has_value(), "duplicate vertex");
			
			const MobyTexCoord& tex_coord = src.sts.at(mesh.vertices.size() - vertex_base);
			f32 s = tex_coord.s / (INT16_MAX / 8.f);
			f32 t = -tex_coord.t / (INT16_MAX / 8.f);
			while(s < 0) s += 1;
			while(t < 0) t += 1;
			v->tex_coord.s = s;
			v->tex_coord.t = t;
			mesh.vertices.emplace_back(*v);
		}
		
		s32 index_queue[3] = {0};
		s32 index_pos = 0;
		s32 max_index = 0;
		s32 texture_index = 0;
		bool reverse_winding_order = true;
		for(u8 index : src.indices) {
			VERIFY_SUBMESH(index != 0x80, "index buffer");
			if(index == 0) {
				// There's an extra index stored in the index header, in
				// addition to an index stored in some 0x10 byte texture unpack
				// blocks. When a texture is applied, the next index from this
				// list is used as the next vertex in the queue, but the
				// triangle with it as its last index is not actually drawn.
				u8 secret_index = src.secret_indices.at(texture_index);
				if(secret_index == 0) {
					if(lift_submesh) {
						VERIFY_SUBMESH(dest.faces.size() >= 3, "index buffer");
						// The VU1 microprogram has multiple vertices in flight
						// at a time, so we need to remove the ones that
						// wouldn't have been written to the GS packet.
						dest.faces.pop_back();
						dest.faces.pop_back();
						dest.faces.pop_back();
					}
					break;
				} else {
					index = secret_index + 0x80;
					if(dest.faces.size() > 0) {
						mesh.submeshes.emplace_back(std::move(dest));
					}
					dest = SubMesh();
					s32 texture = src.textures.at(texture_index).d3_tex0.data_lo;
					assert(texture >= -1);
					if(texture == -1) {
						dest.material = 0; // none
					} else if(texture >= texture_count) {
						dest.material = 1; // dummy
					} else {
						dest.material = 2 + texture; // mat[texture]
					}
					texture_index++;
				}
			}
			if(index < 0x80) {
				VERIFY_SUBMESH(vertex_base + index - 1 < mesh.vertices.size(), "index buffer");
				index_queue[index_pos] = vertex_base + index - 1;
				if(lift_submesh) {
					if(reverse_winding_order) {
						s32 v0 = index_queue[(index_pos + 3) % 3];
						s32 v1 = index_queue[(index_pos + 2) % 3];
						s32 v2 = index_queue[(index_pos + 1) % 3];
						dest.faces.emplace_back(v0, v1, v2);
					} else {
						s32 v0 = index_queue[(index_pos + 1) % 3];
						s32 v1 = index_queue[(index_pos + 2) % 3];
						s32 v2 = index_queue[(index_pos + 3) % 3];
						dest.faces.emplace_back(v0, v1, v2);
					}
				}
			} else {
				index_queue[index_pos] = vertex_base + index - 0x81;
			}
			max_index = std::max(max_index, index_queue[index_pos]);
			VERIFY_SUBMESH(index_queue[index_pos] < mesh.vertices.size(), "index buffer");
			index_pos = (index_pos + 1) % 3;
			reverse_winding_order = !reverse_winding_order;
		}
	}
	if(dest.faces.size() > 0) {
		mesh.submeshes.emplace_back(std::move(dest));
	}
	mesh = deduplicate_vertices(std::move(mesh));
	return mesh;
}

void map_indices(MobySubMesh& submesh, const std::vector<size_t>& mapping) {
	assert(submesh.vertices.size() == mapping.size());
	
	// Find the end of the index buffer.
	s32 next_secret_index_pos = 0;
	size_t buffer_end = 0;
	for(size_t i = 0; i < submesh.indices.size(); i++) {
		u8 index = submesh.indices[i];
		if(index == 0) {
			if(next_secret_index_pos >= submesh.secret_indices.size() || submesh.secret_indices[next_secret_index_pos] == 0) {
				assert(i >= 3);
				buffer_end = i - 3;
			}
			next_secret_index_pos++;
		}
	}
	
	// Map the index buffer and the secret indices.
	next_secret_index_pos = 0;
	for(size_t i = 0; i < buffer_end; i++) {
		u8& index = submesh.indices[i];
		if(index == 0) {
			if(next_secret_index_pos < submesh.secret_indices.size()) {
				u8& secret_index = submesh.secret_indices[next_secret_index_pos];
				if(secret_index != 0 && secret_index <= submesh.vertices.size()) {
					secret_index = mapping.at(secret_index - 1) + 1;
				}
			}
			next_secret_index_pos++;
		} else {
			bool restart_bit_set = index >= 0x80;
			if(restart_bit_set) {
				index -= 0x80;
			}
			if(index <= submesh.vertices.size()) {;
				index = mapping.at(index - 1) + 1;
			}
			if(restart_bit_set) {
				index += 0x80;
			}
		}
	}
}
