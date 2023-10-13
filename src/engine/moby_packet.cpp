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

#include "moby_packet.h"

#include <core/vif.h>

namespace MOBY {
	
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobyPacketBase& packet, s64 class_header_ofs);

std::vector<MobyPacket> read_moby_packets(Buffer src, s64 table_ofs, s64 count, f32 scale, bool animated, MobyFormat format) {
	std::vector<MobyPacket> packets;
	
	Opt<SkinAttributes> blend_buffer[64]; // The game stores blended matrices in VU0 memory.
	
	auto packet_table = src.read_multiple<MobyPacketEntry>(table_ofs, count, "moby packet table");
	for(s32 i = 0; i < (s32) packet_table.size(); i++) {
		VERBOSE_SKINNING(printf("******** packet %d\n", i));
		
		const MobyPacketEntry& entry = packet_table[i];
		MobyPacket packet;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer st_data(unpacks.at(0).data);
		packet.sts = st_data.read_multiple<MobyTexCoord>(0, st_data.size() / 4, "moby st unpack").copy();
		
		Buffer index_data(unpacks.at(1).data);
		auto index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		packet.index_header_first_byte = index_header.unknown_0;
		verify(index_header.pad == 0, "Moby has bad index buffer.");
		packet.secret_indices.push_back(index_header.secret_index);
		packet.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 3) {
			Buffer texture_data(unpacks.at(2).data);
			verify(texture_data.size() % 0x40 == 0, "Moby has bad texture unpack.");
			for(size_t i = 0; i < texture_data.size() / 0x40; i++) {
				packet.secret_indices.push_back(texture_data.read<s32>(i * 0x10 + 0xc, "extra index"));
				auto prim = texture_data.read<MobyTexturePrimitive>(i * 0x40, "moby texture primitive");
				verify(prim.d3_tex0_1.data_lo >= MOBY_TEX_NONE, "Regular moby packet has a texture index that is too low.");
				packet.textures.push_back(prim);
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
		
		MobyPacketLowLevel low{packet};
		
		low.preloop_matrix_transfers = src.read_multiple<MobyMatrixTransfer>(array_ofs, vertex_header.matrix_transfer_count, "vertex table").copy();
		for(const MobyMatrixTransfer& transfer : low.preloop_matrix_transfers) {
			verify(transfer.vu0_dest_addr % 4 == 0, "Unaligned pre-loop joint address 0x%llx.", transfer.vu0_dest_addr);
			if(!animated && transfer.spr_joint_index == 0) {
				// If the mesh isn't animated, use the blend shape matrix (identity matrix).
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
			packet.duplicate_vertices.push_back(dupe >> 7);
		}
		
		low.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
		low.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
		
		low.vertices = read_vertices(src, entry, vertex_header, format);
		packet.vertices = unpack_vertices(low, blend_buffer, scale);
		//sort_moby_vertices_after_reading(low, packet);
		
		packet.unknown_e = vertex_header.unknown_e;
		if(format == MobyFormat::RAC1) {
			s32 unknown_e_size = entry.vertex_data_size * 0x10 - vertex_header.unknown_e;
			packet.unknown_e_data = src.read_bytes(entry.vertex_offset + vertex_header.unknown_e, unknown_e_size, "vertex table unknown_e data");
		}
		
		packets.emplace_back(std::move(packet));
	}
	return packets;
}

void write_moby_packets(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const MobyPacket* packets_in, size_t packet_count, f32 scale, MobyFormat format, s64 class_header_ofs) {
	static const s32 ST_UNPACK_ADDR_QUADWORDS = 0xc2;
	
	// TODO: Make it so we don't have to copy the packets here.
	std::vector<MobyPacket> packets;
	for(size_t i = 0; i < packet_count; i++) {
		packets.push_back(packets_in[i]);
	}
	
	// Fixup joint indices.
	for(MobyPacket& packet : packets) {
		for(Vertex& vertex : packet.vertices) {
			for(s32 i = 0; i < vertex.skin.count; i++) {
				if(vertex.skin.joints[i] == -1) {
					vertex.skin.joints[i] = 0;
				}
			}
		}
	}
	
	s32 max_joints_per_packet = max_num_joints_referenced_per_packet(packets);
	
	std::vector<std::vector<MatrixLivenessInfo>> liveness = compute_matrix_liveness(packets);
	verify_fatal(liveness.size() == packets.size());
	
	std::vector<MobyPacketLowLevel> low_packets;
	MobyPacketLowLevel* last = nullptr;
	VU0MatrixAllocator matrix_allocator(max_joints_per_packet);
	for(size_t i = 0; i < packets.size(); i++) {
		VERBOSE_MATRIX_ALLOCATION(printf("**** packet %d ****\n", i));
		matrix_allocator.new_packet();
		MatrixTransferSchedule schedule = schedule_matrix_transfers((s32) i, packets[i], last, matrix_allocator, liveness[i]);
		MobyPacketLowLevel low = pack_vertices((s32) i, packets[i], matrix_allocator, liveness[i], scale);
		
		// Write the scheduled transfers.
		verify_fatal((last == nullptr && schedule.last_packet_transfers.size() == 0) ||
			(last != nullptr && schedule.last_packet_transfers.size() <= last->main_vertex_count));
		for(size_t i = 0; i < schedule.last_packet_transfers.size(); i++) {
			verify_fatal(last != nullptr);
			MobyVertex& mv = last->vertices.at(last->vertices.size() - i - 1);
			MobyMatrixTransfer& transfer = schedule.last_packet_transfers[i];
			mv.v.regular.low_halfword |= transfer.spr_joint_index << 9;
			mv.v.regular.vu0_transferred_matrix_store_addr = transfer.vu0_dest_addr;
		}
		low.preloop_matrix_transfers = std::move(schedule.preloop_transfers);
		verify_fatal(schedule.two_way_transfers.size() <= low.two_way_blend_vertex_count);
		for(size_t i = 0; i < schedule.two_way_transfers.size(); i++) {
			MobyVertex& mv = low.vertices.at(i);
			MobyMatrixTransfer& transfer = schedule.two_way_transfers[i];
			mv.v.two_way_blend.low_halfword |= transfer.spr_joint_index << 9;
			mv.v.two_way_blend.vu0_transferred_matrix_store_addr = transfer.vu0_dest_addr;
		}
		
		// The vertices are reordered while being packed.
		map_indices(packets[i], low.index_mapping);
		
		low_packets.emplace_back(std::move(low));
		last = &low_packets.back();
	}
	
	for(const MobyPacketLowLevel& low : low_packets) {
		const MobyPacket& packet = low.high_level;
		MobyPacketEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		
		VifPacket st_unpack;
		st_unpack.code.interrupt = 0;
		st_unpack.code.cmd = (VifCmd) 0b1110000; // UNPACK
		st_unpack.code.num = packet.sts.size();
		st_unpack.code.unpack.vnvl = VifVnVl::V2_16;
		st_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		st_unpack.code.unpack.usn = VifUsn::SIGNED;
		st_unpack.code.unpack.addr = ST_UNPACK_ADDR_QUADWORDS;
		st_unpack.data = Buffer((u8*) &packet.sts[0], (u8*) (&packet.sts[0] + packet.sts.size()));
		write_vif_packet(dest, st_unpack);
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, packet, class_header_ofs);
		
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		s64 vertex_header_ofs = dest.tell();
		
		auto vertex_header = write_vertices(dest, packet, low, format);
		
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.transfer_vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.transfer_vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.transfer_vertex_count;
		
		dest.pad(0x10);
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
}

std::vector<MobyMetalSubMesh> read_moby_metal_packets(Buffer src, s64 table_ofs, s64 count) {
	std::vector<MobyMetalSubMesh> packets;
	for(auto& entry : src.read_multiple<MobyPacketEntry>(table_ofs, count, "moby metal packet table")) {
		MobyMetalSubMesh packet;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer index_data(unpacks.at(0).data);
		auto index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		packet.index_header_first_byte = index_header.unknown_0;
		verify(index_header.pad == 0, "Moby has bad index buffer.");
		packet.secret_indices.push_back(index_header.secret_index);
		packet.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 2) {
			Buffer texture_data(unpacks.at(1).data);
			verify(texture_data.size() % 0x40 == 0, "Moby has bad texture unpack.");
			for(size_t i = 0; i < texture_data.size() / 0x40; i++) {
				packet.secret_indices.push_back(texture_data.read<s32>(i * 0x10 + 0xc, "extra index"));
				auto prim = texture_data.read<MobyTexturePrimitive>(i * 0x40, "moby texture primitive");
				verify(prim.d3_tex0_1.data_lo == MOBY_TEX_CHROME || prim.d3_tex0_1.data_lo == MOBY_TEX_GLASS,
					"Metal moby packet has a bad texture index.");
				packet.textures.push_back(prim);
			}
		}
		
		// Read vertex table.
		auto vertex_header = src.read<MobyMetalVertexTableHeader>(entry.vertex_offset, "metal vertex table header");
		packet.vertices = src.read_multiple<MobyMetalVertex>(entry.vertex_offset + 0x10, vertex_header.vertex_count, "metal vertex table").copy();
		packet.unknown_4 = vertex_header.unknown_4;
		packet.unknown_8 = vertex_header.unknown_8;
		packet.unknown_c = vertex_header.unknown_c;
		
		packets.emplace_back(std::move(packet));
	}
	return packets;
}

void write_moby_metal_packets(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& packets, s64 class_header_ofs) {
	for(const MobyMetalSubMesh& packet : packets) {
		MobyPacketEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, packet, class_header_ofs);
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Write vertex table.
		MobyMetalVertexTableHeader vertex_header;
		vertex_header.vertex_count = packet.vertices.size();
		vertex_header.unknown_4 = packet.unknown_4;
		vertex_header.unknown_8 = packet.unknown_8;
		vertex_header.unknown_c = packet.unknown_c;
		s64 vertex_header_ofs = dest.write(vertex_header);
		dest.write_multiple(packet.vertices);
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

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobyPacketBase& packet, s64 class_header_ofs) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	std::vector<u8> indices;
	OutBuffer index_buffer(indices);
	s64 index_header_ofs = index_buffer.alloc<MobyIndexHeader>();
	index_buffer.write_multiple(packet.indices);
	
	MobyIndexHeader index_header = {0};
	index_header.unknown_0 = packet.index_header_first_byte;
	if(packet.textures.size() > 0) {
		index_header.texture_unpack_offset_quadwords = indices.size() / 4;
	}
	if(packet.secret_indices.size() >= 1) {
		index_header.secret_index = packet.secret_indices[0];
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
	index_unpack.data = Buffer(indices);
	write_vif_packet(dest, index_unpack);
	
	s64 rel_texture_unpack_ofs = 0;
	if(packet.textures.size() > 0) {
		while(dest.tell() % 0x10 != 0xc) {
			dest.write<u8>(0);
		}
		
		VifPacket texture_unpack;
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
		texture_unpack.code.num = packet.textures.size() * 4;
		texture_unpack.code.unpack.vnvl = VifVnVl::V4_32;
		texture_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = VifUsn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + index_unpack.code.num;
		
		verify_fatal(packet.secret_indices.size() >= packet.textures.size());
		std::vector<u8> texture_unpack_data;
		for(size_t i = 0; i < packet.textures.size(); i++) {
			MobyTexturePrimitive primitive = packet.textures[i];
			OutBuffer(texture_unpack_data).write(primitive);
		}
		for(size_t i = 1; i < packet.secret_indices.size(); i++) {
			OutBuffer(texture_unpack_data).write((i - 1) * 0x10 + 0xc, packet.secret_indices[i]);
		}
		texture_unpack.data = Buffer(texture_unpack_data);
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsage gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			for(const MobyTexturePrimitive& prim : packet.textures) {
				verify_fatal(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = prim.d3_tex0_1.data_lo;
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

static void sort_moby_vertices_after_reading(MobyPacketLowLevel& low, MobyPacket& packet) {
	verify_fatal(low.vertices.size() == packet.vertices.size());
	
	s32 two_way_end = low.two_way_blend_vertex_count;
	s32 three_way_end = low.two_way_blend_vertex_count + low.three_way_blend_vertex_count;
	
	s32 two_way_index = 0;
	s32 three_way_index = two_way_end;
	s32 next_mapped_index = 0;
	
	std::vector<size_t> mapping(packet.vertices.size(), SIZE_MAX);
	
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
	verify_fatal(next_mapped_index == three_way_end);
	
	for(s32 i = three_way_end; i < (s32) low.vertices.size(); i++) {
		mapping[i] = i;
	}
	
	low.vertices = {};
	
	auto old_vertices = std::move(packet.vertices);
	packet.vertices = std::vector<Vertex>(old_vertices.size());
	for(size_t i = 0; i < old_vertices.size(); i++) {
		packet.vertices[mapping[i]] = old_vertices[i];
	}
	
	map_indices(packet, mapping);
}

void map_indices(MobyPacket& packet, const std::vector<size_t>& mapping) {
	verify_fatal(packet.vertices.size() == mapping.size());
	
	// Find the end of the index buffer.
	s32 next_secret_index_pos = 0;
	size_t buffer_end = 0;
	for(size_t i = 0; i < packet.indices.size(); i++) {
		u8 index = packet.indices[i];
		if(index == 0) {
			if(next_secret_index_pos >= packet.secret_indices.size() || packet.secret_indices[next_secret_index_pos] == 0) {
				verify_fatal(i >= 3);
				buffer_end = i - 3;
			}
			next_secret_index_pos++;
		}
	}
	
	// Map the index buffer and the secret indices.
	next_secret_index_pos = 0;
	for(size_t i = 0; i < buffer_end; i++) {
		u8& index = packet.indices[i];
		if(index == 0) {
			if(next_secret_index_pos < packet.secret_indices.size()) {
				u8& secret_index = packet.secret_indices[next_secret_index_pos];
				if(secret_index != 0 && secret_index <= packet.vertices.size()) {
					secret_index = mapping.at(secret_index - 1) + 1;
				}
			}
			next_secret_index_pos++;
		} else {
			bool restart_bit_set = index >= 0x80;
			if(restart_bit_set) {
				index -= 0x80;
			}
			if(index <= packet.vertices.size()) {;
				index = mapping.at(index - 1) + 1;
			}
			if(restart_bit_set) {
				index += 0x80;
			}
		}
	}
}

}
