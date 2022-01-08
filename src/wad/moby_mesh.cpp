/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#define WRENCH_PI 3.14159265358979323846

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh, s64 class_header_ofs);
static Vertex recover_vertex(const MobyVertex& vertex, const SkinAttributes& skin, const glm::vec2& tex_coord, f32 scale);
static MobyVertex build_vertex(const Vertex& v, s32 id, f32 inverse_scale);
static SkinAttributes recover_blend_attributes(Opt<SkinAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count);
struct IndexMappingRecord {
	s32 submesh = -1;
	s32 index = -1; // The index of the vertex in the vertex table.
	s32 id = -1; // The index of the vertex in the intermediate buffer.
	s32 dedup_out_edge = -1; // If this vertex is a duplicate, this points to the canonical vertex.
};
static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices);
static f32 acotf(f32 x);

std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, MobyFormat format) {
	std::vector<MobySubMesh> submeshes;
	for(auto& entry : src.read_multiple<MobySubMeshEntry>(table_ofs, count, "moby submesh table")) {
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
		submesh.preloop_matrix_transfers = src.read_multiple<MobyMatrixTransfer>(array_ofs, vertex_header.matrix_transfer_count, "vertex table").copy();
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
		s64 vertex_ofs = entry.vertex_offset + vertex_header.vertex_table_offset;
		s32 in_file_vertex_count = vertex_header.two_way_blend_vertex_count + vertex_header.three_way_blend_vertex_count + vertex_header.main_vertex_count;
		submesh.vertices = src.read_multiple<MobyVertex>(vertex_ofs, in_file_vertex_count, "vertex table").copy();
		vertex_ofs += in_file_vertex_count * 0x10;
		submesh.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
		submesh.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
		submesh.unknown_e = vertex_header.unknown_e;
		if(format == MobyFormat::RAC1) {
			s32 unknown_e_size = entry.vertex_data_size * 0x10 - vertex_header.unknown_e;
			submesh.unknown_e_data = src.read_bytes(entry.vertex_offset + vertex_header.unknown_e, unknown_e_size, "vertex table unknown_e data");
		}
		
		// Fix vertex indices (see comment in write_moby_submeshes).
		for(size_t i = 7; i < submesh.vertices.size(); i++) {
			submesh.vertices[i - 7].vertex_index = submesh.vertices[i].vertex_index;
		}
		s32 trailing_vertex_count = 0;
		if(format == MobyFormat::RAC1) {
			trailing_vertex_count = (vertex_header.unknown_e - vertex_header.vertex_table_offset) / 0x10 - in_file_vertex_count;
		} else {
			trailing_vertex_count = entry.vertex_data_size - vertex_header.vertex_table_offset / 0x10 - in_file_vertex_count;
		}
		verify(trailing_vertex_count < 7, "Bad moby vertex table.");
		vertex_ofs += std::max(7 - in_file_vertex_count, 0) * 0x10;
		for(s64 i = std::max(7 - in_file_vertex_count, 0); i < trailing_vertex_count; i++) {
			MobyVertex vertex = src.read<MobyVertex>(vertex_ofs, "vertex table");
			vertex_ofs += 0x10;
			s64 dest_index = in_file_vertex_count + i - 7;
			submesh.vertices.at(dest_index).vertex_index = vertex.vertex_index;
		}
		MobyVertex last_vertex = src.read<MobyVertex>(vertex_ofs - 0x10, "vertex table");
		for(s32 i = std::max(7 - in_file_vertex_count - trailing_vertex_count, 0); i < 6; i++) {
			s64 dest_index = in_file_vertex_count + trailing_vertex_count + i - 7;
			if(dest_index < submesh.vertices.size()) {
				submesh.vertices[dest_index].vertex_index = last_vertex.trailing.vertex_indices[i];
			}
		}
		
		submeshes.emplace_back(std::move(submesh));
	}
	return submeshes;
}

void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes, MobyFormat format, s64 class_header_ofs) {
	static const s32 ST_UNPACK_ADDR_QUADWORDS = 0xc2;
	
	for(const MobySubMesh& submesh : submeshes) {
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		
		VifPacket st_unpack;
		st_unpack.code.interrupt = 0;
		st_unpack.code.cmd = (VifCmd) 0b1110000; // UNPACK
		st_unpack.code.num = submesh.sts.size();
		st_unpack.code.unpack.vnvl = VifVnVl::V2_16;
		st_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		st_unpack.code.unpack.usn = VifUsn::SIGNED;
		st_unpack.code.unpack.addr = ST_UNPACK_ADDR_QUADWORDS;
		st_unpack.data.resize(submesh.sts.size() * 4);
		memcpy(st_unpack.data.data(), submesh.sts.data(), submesh.sts.size() * 4);
		write_vif_packet(dest, st_unpack);
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh, class_header_ofs);
		
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Umm.. "adjust" vertex indices (see comment below).
		std::vector<MobyVertex> vertices = submesh.vertices;
		std::vector<u16> trailing_vertex_indices(std::max(7 - (s32) vertices.size(), 0), 0);
		for(s32 i = std::max((s32) vertices.size() - 7, 0); i < vertices.size(); i++) {
			trailing_vertex_indices.push_back(vertices[i].vertex_index);
		}
		for(s32 i = vertices.size() - 1; i >= 7; i--) {
			vertices[i].vertex_index = vertices[i - 7].vertex_index;
		}
		for(s32 i = 0; i < std::min(7, (s32) vertices.size()); i++) {
			vertices[i].vertex_index = 0;
		}
		
		// Write vertex table.
		s64 vertex_header_ofs;
		if(format == MobyFormat::RAC1) {
			vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac1>();
		} else {
			vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac23DL>();
		}
		MobyVertexTableHeaderRac1 vertex_header;
		vertex_header.matrix_transfer_count = submesh.preloop_matrix_transfers.size();
		vertex_header.two_way_blend_vertex_count = submesh.two_way_blend_vertex_count;
		vertex_header.three_way_blend_vertex_count = submesh.three_way_blend_vertex_count;
		vertex_header.main_vertex_count =
			submesh.vertices.size() -
			submesh.two_way_blend_vertex_count -
			submesh.three_way_blend_vertex_count;
		vertex_header.duplicate_vertex_count = submesh.duplicate_vertices.size();
		vertex_header.transfer_vertex_count =
			vertex_header.two_way_blend_vertex_count +
			vertex_header.three_way_blend_vertex_count +
			vertex_header.main_vertex_count +
			vertex_header.duplicate_vertex_count;
		vertex_header.unknown_e = submesh.unknown_e;
		dest.write_multiple(submesh.preloop_matrix_transfers);
		dest.pad(0x8);
		for(u16 dupe : submesh.duplicate_vertices) {
			dest.write<u16>(dupe << 7);
		}
		dest.pad(0x10);
		vertex_header.vertex_table_offset = dest.tell() - vertex_header_ofs;
		
		// Write out the remaining vertex indices after the rest of the proper
		// vertices (since the vertex index stored in each vertex corresponds to
		// the vertex 7 vertices prior for some reason). The remaining indices
		// are written out into the padding vertices and then when that space
		// runs out they're written into the second part of the last padding
		// vertex (hence there is at least one padding vertex). Now I see why
		// they call it Insomniac Games.
		s32 trailing = 0;
		for(; vertices.size() % 4 != 2 && trailing < trailing_vertex_indices.size(); trailing++) {
			MobyVertex vertex = {0};
			if(submesh.vertices.size() + trailing >= 7) {
				vertex.vertex_index = trailing_vertex_indices[trailing];
			}
			vertices.push_back(vertex);
		}
		assert(trailing < trailing_vertex_indices.size());
		MobyVertex last_vertex = {0};
		if(submesh.vertices.size() + trailing >= 7) {
			last_vertex.vertex_index = trailing_vertex_indices[trailing];
		}
		for(s32 i = trailing + 1; i < trailing_vertex_indices.size(); i++) {
			if(submesh.vertices.size() + i >= 7) {
				last_vertex.trailing.vertex_indices[i - trailing - 1] = trailing_vertex_indices[i];
			}
		}
		vertices.push_back(last_vertex);
		dest.write_multiple(vertices);
		
		if(format == MobyFormat::RAC1) {
			vertex_header.unknown_e = dest.tell() - vertex_header_ofs;
			dest.write_multiple(submesh.unknown_e_data);
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
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.transfer_vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.transfer_vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.transfer_vertex_count;
		
		vertex_header.unknown_e = 0;
		dest.pad(0x10);
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
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

void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes, s64 class_header_ofs) {
	for(const MobyMetalSubMesh& submesh : submeshes) {
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh, class_header_ofs);
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Write vertex table.
		MobyMetalVertexTableHeader vertex_header;
		vertex_header.vertex_count = submesh.vertices.size();
		vertex_header.unknown_4 = submesh.unknown_4;
		vertex_header.unknown_8 = submesh.unknown_8;
		vertex_header.unknown_c = submesh.unknown_c;
		s64 vertex_header_ofs = dest.write(vertex_header);
		dest.write_multiple(submesh.vertices);
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.vertex_count;
		
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
}

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh, s64 class_header_ofs) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	std::vector<u8> indices;
	OutBuffer index_buffer(indices);
	s64 index_header_ofs = index_buffer.alloc<MobyIndexHeader>();
	index_buffer.write_multiple(submesh.indices);
	
	MobyIndexHeader index_header = {0};
	index_header.unknown_0 = submesh.index_header_first_byte;
	if(submesh.textures.size() > 0) {
		index_header.texture_unpack_offset_quadwords = indices.size() / 4;
	}
	if(submesh.secret_indices.size() >= 1) {
		index_header.secret_index = submesh.secret_indices[0];
	}
	index_buffer.write(index_header_ofs, index_header);
	
	VifPacket index_unpack;
	index_unpack.code.interrupt = 0;
	index_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
	index_unpack.code.num = indices.size() / 4;
	index_unpack.code.unpack.vnvl = VifVnVl::V4_8;
	index_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	index_unpack.code.unpack.usn = VifUsn::SIGNED;
	index_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS;
	index_unpack.data = std::move(indices);
	write_vif_packet(dest, index_unpack);
	
	s64 rel_texture_unpack_ofs = 0;
	if(submesh.textures.size() > 0) {
		while(dest.tell() % 0x10 != 0xc) {
			dest.write<u8>(0);
		}
		
		VifPacket texture_unpack;
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
		texture_unpack.code.num = submesh.textures.size() * 4;
		texture_unpack.code.unpack.vnvl = VifVnVl::V4_32;
		texture_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = VifUsn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + index_unpack.code.num;
		
		assert(submesh.secret_indices.size() >= submesh.textures.size());
		for(size_t i = 0; i < submesh.textures.size(); i++) {
			MobyTexturePrimitive primitive = submesh.textures[i];
			OutBuffer(texture_unpack.data).write(primitive);
		}
		for(size_t i = 1; i < submesh.secret_indices.size(); i++) {
			OutBuffer(texture_unpack.data).write((i - 1) * 0x10 + 0xc, submesh.secret_indices[i]);
		}
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsageTableEntry gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			for(const MobyTexturePrimitive& prim : submesh.textures) {
				assert(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = prim.d3_tex0.data_lo;
			}
			for(s32 i = gif_index; i < 12; i++) {
				gif_entry.texture_indices[i] = 0xff;
			}
			gif_usage->push_back(gif_entry);
		}
		
		dest.pad(0x10);
		rel_texture_unpack_ofs = (dest.tell() - abs_texture_unpack_ofs + 0x4) / 0x10;
	}
	
	return rel_texture_unpack_ofs;
}

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, submesh %d has bad " message ".", o_class, i);

Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, s32 joint_count, f32 scale, s32 submesh_filter) {
	Mesh mesh;
	mesh.name = name;
	mesh.flags = MESH_HAS_NORMALS | MESH_HAS_TEX_COORDS;
	
	Opt<SkinAttributes> blend_buffer[64]; // The game stores this in VU0 memory.
	Opt<Vertex> intermediate_buffer[256]; // The game stores this on the end of the VU1 chain.
	
	SubMesh dest;
	dest.material = 0;
	
	for(s32 i = 0; i < submeshes.size(); i++) {
		bool lift_submesh = !MOBY_EXPORT_SUBMESHES_SEPERATELY || submesh_filter == -1 || i == submesh_filter; // This is just for debugging.
		
		const MobySubMesh& src = submeshes[i];
		
		for(const MobyMatrixTransfer& transfer : src.preloop_matrix_transfers) {
			verify(transfer.vu0_dest_addr % 4 == 0, "Unaligned pre-loop joint address 0x%llx.", transfer.vu0_dest_addr);
			if(joint_count == 0 && transfer.spr_joint_index == 0) {
				// If there aren't any joints, use the blend shape matrix (identity matrix).
				blend_buffer[transfer.vu0_dest_addr / 4] = SkinAttributes{1, {-1, 0, 0}, {255, 0, 0}};
			} else {
				blend_buffer[transfer.vu0_dest_addr / 4] = SkinAttributes{1, {(s8) transfer.spr_joint_index, 0, 0}, {255, 0, 0}};
			}
		}
		
		s32 vertex_base = mesh.vertices.size();
		
		for(size_t j = 0; j < src.vertices.size(); j++) {
			const MobyVertex& mv = src.vertices[j];
			
			SkinAttributes skin = recover_blend_attributes(blend_buffer, mv, j, src.two_way_blend_vertex_count, src.three_way_blend_vertex_count);
			for(s32 k = 0; k < 3; k++) {
				verify(skin.joints[k] < joint_count || (joint_count == 0 && skin.joints[k] == 0),
					"Joint index (%hd) greater than or equal to non-zero joint count (%d).",
					skin.joints[k], joint_count);
			}
			
			const MobyTexCoord& tex_coord = src.sts.at(mesh.vertices.size() - vertex_base);
			f32 s = tex_coord.s / (INT16_MAX / 8.f);
			f32 t = -tex_coord.t / (INT16_MAX / 8.f);
			while(s < 0) s += 1;
			while(t < 0) t += 1;
			Vertex v = recover_vertex(mv, skin, glm::vec2(s, t), scale);
			
			intermediate_buffer[mv.vertex_index] = v;
			mesh.vertices.emplace_back(v);
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

struct RichIndex {
	u32 index : 30;
	u32 restart : 1;
	u32 is_dupe : 1 = 0;
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

// Intermediate data structure used so the submeshes can be built in two
// seperate passes.
struct MidLevelSubMesh {
	std::vector<MidLevelVertex> vertices;
	std::vector<RichIndex> indices;
	std::vector<MidLevelTexture> textures;
	std::vector<MidLevelDuplicateVertex> duplicate_vertices;
};

std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials, f32 scale) {
	static const s32 MAX_SUBMESH_TEXTURE_COUNT = 4;
	static const s32 MAX_SUBMESH_STORED_VERTEX_COUNT = 97;
	static const s32 MAX_SUBMESH_TOTAL_VERTEX_COUNT = 0x7f;
	static const s32 MAX_SUBMESH_INDEX_COUNT = 196;
	
	std::vector<IndexMappingRecord> index_mappings(mesh.vertices.size());
	find_duplicate_vertices(index_mappings, mesh.vertices);
	
	f32 inverse_scale = 1024.f / scale;
	
	// *************************************************************************
	// First pass
	// *************************************************************************
	
	std::vector<MidLevelSubMesh> mid_submeshes;
	MidLevelSubMesh mid;
	s32 next_id = 0;
	for(s32 i = 0; i < (s32) mesh.submeshes.size(); i++) {
		const SubMesh& high = mesh.submeshes[i];
		
		auto indices = fake_tristripper(high.faces);
		if(indices.size() < 1) {
			continue;
		}
		
		const Material& material = materials.at(high.material);
		s32 texture = -1;
		if(material.name.size() > 4 && memcmp(material.name.data(), "mat_", 4) == 0) {
			texture = strtol(material.name.c_str() + 4, nullptr, 10);
		} else {
			fprintf(stderr, "Invalid material '%s'.", material.name.c_str());
			continue;
		}
		
		if(mid.textures.size() >= MAX_SUBMESH_TEXTURE_COUNT || mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT) {
			mid_submeshes.emplace_back(std::move(mid));
			mid = MidLevelSubMesh{};
		}
		
		mid.textures.push_back({texture, (s32) mid.indices.size()});
		
		for(size_t j = 0; j < indices.size(); j++) {
			auto new_submesh = [&]() {
				mid_submeshes.emplace_back(std::move(mid));
				mid = MidLevelSubMesh{};
				// Handle splitting the strip up between moby submeshes.
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
					// submesh but didn't push any non-restarting indices, go
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
			
			if(canonical.submesh != mid_submeshes.size()) {
				if(mid.vertices.size() >= MAX_SUBMESH_STORED_VERTEX_COUNT) {
					new_submesh();
					continue;
				}
				
				canonical.submesh = mid_submeshes.size();
				canonical.index = mid.vertices.size();
				
				mid.vertices.push_back({(s32) r.index, (s32) r.index});
			} else if(mapping.submesh != mid_submeshes.size()) {
				if(canonical.id == -1) {
					canonical.id = next_id++;
					mid.vertices.at(canonical.index).id = canonical.id;
				}
				mid.duplicate_vertices.push_back({canonical.id, (s32) r.index});
			}
			
			if(mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT - 4) {
				new_submesh();
				continue;
			}
			
			mid.indices.push_back({(u32) canonical.index, r.restart, r.is_dupe});
		}
	}
	if(mid.indices.size() > 0) {
		mid_submeshes.emplace_back(std::move(mid));
	}
	
	// *************************************************************************
	// Second pass
	// *************************************************************************
	
	std::vector<MobySubMesh> low_submeshes;
	for(const MidLevelSubMesh& mid : mid_submeshes) {
		MobySubMesh low;
		
		for(const MidLevelVertex& vertex : mid.vertices) {
			const Vertex& high_vert = mesh.vertices[vertex.canonical];
			low.vertices.emplace_back(build_vertex(high_vert, vertex.id, inverse_scale));
			
			const glm::vec2& tex_coord = mesh.vertices[vertex.tex_coord].tex_coord;
			s16 s = tex_coord.x * (INT16_MAX / 8.f);
			s16 t = tex_coord.y * (INT16_MAX / 8.f);
			low.sts.push_back({s, t});
		}
		
		s32 texture_index = 0;
		for(size_t i = 0; i < mid.indices.size(); i++) {
			RichIndex cur = mid.indices[i];
			u8 out;
			if(cur.is_dupe) {
				out = mid.vertices.size() + cur.index;
			} else {
				out = cur.index;
			}
			if(texture_index < mid.textures.size() && mid.textures.at(texture_index).starting_index >= i) {
				assert(cur.restart);
				low.indices.push_back(0);
				low.secret_indices.push_back(out + 1);
				texture_index++;
			} else {
				low.indices.push_back(cur.restart ? (out + 0x81) : (out + 1));
			}
		}
		
		// These fake indices are required to signal to the microprogram that it
		// should terminate.
		low.indices.push_back(1);
		low.indices.push_back(1);
		low.indices.push_back(1);
		low.indices.push_back(0);
		
		for(const MidLevelTexture& tex : mid.textures) {
			MobyTexturePrimitive primitive = {0};
			primitive.d1_xyzf2.data_lo = 0xff92; // Not sure.
			primitive.d1_xyzf2.data_hi = 0x4;
			primitive.d1_xyzf2.address = 0x4;
			primitive.d1_xyzf2.pad_a = 0x41a0;
			primitive.d2_clamp.address = 0x08;
			primitive.d3_tex0.address = 0x06;
			primitive.d3_tex0.data_lo = tex.texture;
			primitive.d4_xyzf2.address = 0x34;
			low.textures.push_back(primitive);
		}
		
		for(const MidLevelDuplicateVertex& dupe : mid.duplicate_vertices) {
			low.duplicate_vertices.push_back(dupe.index);
			
			const glm::vec2& tex_coord = mesh.vertices[dupe.tex_coord].tex_coord;
			s16 s = tex_coord.x * (INT16_MAX / 8.f);
			s16 t = tex_coord.y * (INT16_MAX / 8.f);
			low.sts.push_back({s, t});
		}
		
		low_submeshes.emplace_back(std::move(low));
	}
	
	return low_submeshes;
}

static Vertex recover_vertex(const MobyVertex& vertex, const SkinAttributes& skin, const glm::vec2& tex_coord, f32 scale) {
	f32 px = vertex.v.x * (scale / 1024.f);
	f32 py = vertex.v.y * (scale / 1024.f);
	f32 pz = vertex.v.z * (scale / 1024.f);
	f32 normal_azimuth_radians = vertex.v.normal_angle_azimuth * (WRENCH_PI / 128.f);
	f32 normal_elevation_radians = vertex.v.normal_angle_elevation * (WRENCH_PI / 128.f);
	// There's a cosine/sine lookup table at the top of the scratchpad, this is
	// done on the EE core.
	f32 cos_azimuth = cosf(normal_azimuth_radians);
	f32 sin_azimuth = sinf(normal_azimuth_radians);
	f32 cos_elevation = cosf(normal_elevation_radians);
	f32 sin_elevation = sinf(normal_elevation_radians);
	// This bit is done on VU0.
	f32 nx = sin_azimuth * cos_elevation;
	f32 ny = cos_azimuth * cos_elevation;
	f32 nz = sin_elevation;
	return Vertex(glm::vec3(px, py, pz), glm::vec3(nx, ny, nz), skin, tex_coord);
}

static MobyVertex build_vertex(const Vertex& src, s32 id, f32 inverse_scale) {
	MobyVertex dest = {0};
	dest.vertex_index = id;
	dest.v.x = src.pos.x * inverse_scale;
	dest.v.y = src.pos.y * inverse_scale;
	dest.v.z = src.pos.z * inverse_scale;
	f32 normal_angle_azimuth_radians;
	if(src.normal.x != 0) {
		normal_angle_azimuth_radians = acotf(src.normal.y / src.normal.x);
		if(src.normal.x < 0) {
			normal_angle_azimuth_radians += WRENCH_PI;
		}
	} else {
		normal_angle_azimuth_radians = WRENCH_PI / 2;
	}
	f32 normal_angle_elevation_radians = asinf(src.normal.z);
	dest.v.normal_angle_azimuth = normal_angle_azimuth_radians * (128.f / WRENCH_PI);
	dest.v.normal_angle_elevation = normal_angle_elevation_radians * (128.f / WRENCH_PI);
	return dest;
}

static SkinAttributes recover_blend_attributes(Opt<SkinAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count) {
	SkinAttributes attribs;
	
	auto load_blend_attribs = [&](s32 addr) {
		verify(blend_buffer[addr / 4].has_value(), "Matrix load from uninitialised VU0 address 0x%llx.", addr);
		return *blend_buffer[addr / 4];
	};
	
	if(ind < two_way_count) {
		u8 transfer_addr = mv.v.two_way_blend.vu0_transferred_matrix_store_addr;
		verify(transfer_addr % 4 == 0, "Unaligned joint address 0x%llx.", transfer_addr);
		blend_buffer[transfer_addr / 4] = SkinAttributes{1, {(s8) (mv.v.two_way_blend.spr_joint_index_mul_2 / 2), 0, 0}, {255, 0, 0}};
		
		SkinAttributes src_1 = load_blend_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_1);
		SkinAttributes src_2 = load_blend_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_2);
		verify(src_1.count == 1 && src_2.count == 1, "Input to two-way matrix blend operation has already been blended.");
		
		u8 weight_1 = mv.v.two_way_blend.weight_1;
		u8 weight_2 = mv.v.two_way_blend.weight_2;
		
		attribs = SkinAttributes{2, {src_1.joints[0], src_2.joints[1], 0}, {weight_1, weight_2, 0}};
		
		u8 blend_addr = mv.v.two_way_blend.vu0_blended_matrix_store_addr;
		verify(blend_addr % 4 == 0, "Unaligned joint address 0x%llx.", blend_addr);
		blend_buffer[blend_addr / 4] = attribs;
	} else if(ind < two_way_count + three_way_count) {
		SkinAttributes src_1 = load_blend_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_1);
		SkinAttributes src_2 = load_blend_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_2);
		SkinAttributes src_3 = load_blend_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_3);
		verify(src_1.count == 1 && src_2.count == 1, "Input to three-way matrix blend operation has already been blended.");
		
		u8 weight_1 = mv.v.three_way_blend.weight_1;
		u8 weight_2 = mv.v.three_way_blend.weight_2;
		u8 weight_3 = mv.v.three_way_blend.weight_3;
		
		attribs = SkinAttributes{3, {src_1.joints[0], src_2.joints[0], src_3.joints[0]}, {weight_1, weight_2, weight_3}};
		
		u8 blend_addr = mv.v.three_way_blend.vu0_blended_matrix_store_addr;
		verify(blend_addr % 4 == 0, "Unaligned joint address 0x%llx.", blend_addr);
		blend_buffer[blend_addr / 4] = attribs;
	} else {
		u8 transfer_addr = mv.v.regular.vu0_transferred_matrix_store_addr;
		verify(transfer_addr % 4 == 0, "Unaligned joint address 0x%llx.", transfer_addr);
		blend_buffer[transfer_addr / 4] = SkinAttributes{1, {(s8) (mv.v.regular.spr_joint_index_mul_2 / 2), 0, 0}, {255, 0, 0}};
		
		attribs = load_blend_attribs(mv.v.regular.vu0_matrix_load_addr);
	}
	
	return attribs;
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

static f32 acotf(f32 x) {
	return WRENCH_PI / 2 - atanf(x);
}
