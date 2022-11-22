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

#include "tfrag.h"

template <typename T>
static std::vector<T> read_unpack(const VifPacket& packet, VifVnVl vnvl) {
	verify(packet.code.is_unpack() && packet.code.unpack.vnvl == vnvl, "Bad VIF command.");
	return packet.data.read_all<T>().copy();
}

std::vector<Tfrag> read_tfrags(Buffer src) {
	TfragsHeader table_header = src.read<TfragsHeader>(0, "tfrags header");
	
	std::vector<Tfrag> tfrags;
	tfrags.reserve(table_header.tfrag_count);
	
	auto table = src.read_multiple<TfragHeader>(table_header.table_offset, table_header.tfrag_count, "tfrag table");
	for(const TfragHeader& header : table) {
		Tfrag& tfrag = tfrags.emplace_back();
		Buffer data = src.subbuf(table_header.table_offset + header.data);
		
		tfrag.bsphere = header.bsphere;
		
		// LOD 2
		Buffer lod_2_buffer = data.subbuf(header.lod_2_ofs, header.shared_ofs - header.lod_2_ofs);
		std::vector<VifPacket> lod_2_command_list = read_vif_command_list(lod_2_buffer);
		std::vector<VifPacket> lod_2 = filter_vif_unpacks(lod_2_command_list);
		verify(lod_2.size() == 2, "Incorrect number of LOD 2 VIF unpacks! %d");
		
		tfrag.lod_2_indices = read_unpack<u8>(lod_2[0], VifVnVl::V4_8);
		tfrag.lod_2_faces = read_unpack<Tface>(lod_2[1], VifVnVl::V4_8);
		
		// Common
		Buffer common_buffer = data.subbuf(header.shared_ofs, header.lod_1_ofs - header.shared_ofs);
		std::vector<VifPacket> common_command_list = read_vif_command_list(common_buffer);
		verify(common_command_list.size() > 5, "Too few shared VIF commands.");
		tfrag.base_position = common_command_list[5].data.read<VifSTROW>(0, "base position");
		std::vector<VifPacket> common = filter_vif_unpacks(common_command_list);
		verify(common.size() == 4, "Incorrect number of shared VIF unpacks!");
		
		tfrag.common_vu_header = common[0].data.read<TfragHeaderUnpack>(0, "VU header");
		tfrag.common_textures = read_unpack<TfragTexturePrimitive>(common[1], VifVnVl::V4_32);
		tfrag.common_vertex_info = read_unpack<TfragVertexInfo>(common[2], VifVnVl::V4_16);
		tfrag.common_positions = read_unpack<TfragVertexPosition>(common[3], VifVnVl::V3_16);
		
		// LOD 1
		Buffer lod_1_buffer = data.subbuf(header.lod_1_ofs, header.lod_0_ofs - header.lod_1_ofs);
		std::vector<VifPacket> lod_1_command_list = read_vif_command_list(lod_1_buffer);
		std::vector<VifPacket> lod_1 = filter_vif_unpacks(lod_1_command_list);
		verify(lod_1.size() == 2, "Incorrect number of LOD 1 VIF unpacks!");
		
		tfrag.lod_1_faces = read_unpack<Tface>(lod_1[0], VifVnVl::V4_8);
		tfrag.lod_1_indices = read_unpack<u8>(lod_1[1], VifVnVl::V4_8);
		
		// LOD 01
		Buffer lod_01_buffer = data.subbuf(
			header.lod_0_ofs,
			header.shared_ofs + header.lod_1_size * 0x10 - header.lod_0_ofs);
		std::vector<VifPacket> lod_01_command_list = read_vif_command_list(lod_01_buffer);
		std::vector<VifPacket> lod_01 = filter_vif_unpacks(lod_01_command_list);
		
		s32 i = 0;
		while(i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V4_8) {
			tfrag.lod_01_unknown_indices.emplace_back(read_unpack<u8>(lod_01[i++], VifVnVl::V4_8));
		}
		if(i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V4_16) {
			tfrag.lod_01_vertex_info = read_unpack<TfragVertexInfo>(lod_01[i++], VifVnVl::V4_16);
		}
		if(i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V3_16) {
			tfrag.lod_01_positions = read_unpack<TfragVertexPosition>(lod_01[i++], VifVnVl::V3_16);
		}
		
		// LOD 0
		Buffer lod_0_buffer = data.subbuf(
			header.shared_ofs + header.lod_1_size * 0x10,
			header.rgba_ofs - (header.lod_1_size + header.lod_2_size - header.common_size) * 0x10);
		std::vector<VifPacket> lod_0_command_list = read_vif_command_list(lod_0_buffer);
		std::vector<VifPacket> lod_0 = filter_vif_unpacks(lod_0_command_list);
		
		i = 0;
		if(i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V3_16) {
			tfrag.lod_0_positions = read_unpack<TfragVertexPosition>(lod_0[i++], VifVnVl::V3_16);
		}
		verify(i < lod_0.size(), "Too few LOD 0 VIF unpacks!");
		tfrag.lod_0_faces = read_unpack<Tface>(lod_0[i++], VifVnVl::V4_8);
		verify(i < lod_0.size(), "Too few LOD 0 VIF unpacks!");
		tfrag.lod_0_indices = read_unpack<u8>(lod_0[i++], VifVnVl::V4_8);
		while(i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V4_8) {
			tfrag.lod_0_unknown_indices.emplace_back(read_unpack<u8>(lod_0[i++], VifVnVl::V4_8));
		}
		if(i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V4_16) {
			tfrag.lod_0_vertex_info = read_unpack<TfragVertexInfo>(lod_0[i++], VifVnVl::V4_16);
		}
		
		//tfrag.rgbas = data.read_multiple<TfragRgba>(header.rgba_ofs, header.rgba_size * 4, "rgbas").copy();
		//tfrag.light = data.read_multiple<u8>(header.light_ofs, header.light_vert_start_ofs - header.light_ofs, "light").copy();
		//tfrag.msphere = data.read_multiple<Vec4f>(header.msphere_ofs, header.msphere_count, "mspheres").copy();
		//tfrag.cube = data.read<TfragCube>(header.cube_ofs, "cube");
	}
	
	return tfrags;
}

template <typename Dest, typename Src>
static Dest checked_int_cast(Src src) {
	assert(src >= std::numeric_limits<Dest>::min()
		&& src <= std::numeric_limits<Dest>::max());
	return static_cast<Dest>(src);
}

static void write_unpack(OutBuffer dest, Buffer data, VifVnVl vnvl, VifUsn usn) {
	VifPacket packet;
	packet.code.interrupt = 0;
	packet.code.cmd = (VifCmd) 0b1100000; // UNPACK
	packet.code.unpack.vnvl = VifVnVl::V4_8;
	packet.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	packet.code.unpack.usn = VifUsn::UNSIGNED;
	packet.code.unpack.addr = 0xcc; // TODO
	packet.data = data;
	packet.code.num = packet.data.size() / packet.code.element_size();
	write_vif_packet(dest, packet);
}

static void write_strow(OutBuffer dest, const VifSTROW& strow) {
	dest.write<u32>(0x30000000);
	dest.write<u32>(strow.vif1_r0);
	dest.write<u32>(strow.vif1_r1);
	dest.write<u32>(strow.vif1_r2);
	dest.write<u32>(strow.vif1_r3);
}

void write_tfrags(OutBuffer dest, const std::vector<Tfrag>& tfrags) {
	s64 table_header_ofs = dest.alloc<TfragsHeader>();
	TfragsHeader table_header = {};
	dest.pad(0x40);
	s64 next_header_ofs = dest.alloc_multiple<TfragHeader>(tfrags.size());
	
	for(const Tfrag& tfrag : tfrags) {
		TfragHeader header = {};
		
		dest.pad(0x10, 0);
		s64 tfrag_ofs = dest.tell();
		header.data = checked_int_cast<s32>(tfrag_ofs - table_header_ofs);
		header.lod_2_ofs = 0;
		
		// Prepare STROW data.
		s32 pos_col_addr = 0x0;
		VifSTROW sth_second_level_indices_strow = {0x45000000, 0x45000000, pos_col_addr, pos_col_addr};
		s32 sth_second_level_indices_addr = 0x0;
		VifSTROW indices_strow = {
			sth_second_level_indices_addr,
			sth_second_level_indices_addr,
			sth_second_level_indices_addr,
			sth_second_level_indices_addr
		};
		
		// LOD 2
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_2_indices, VifVnVl::V4_8, VifUsn::UNSIGNED);
		dest.write<u32>(0x05000000); // stmod
		write_unpack(dest, tfrag.lod_2_faces, VifVnVl::V4_8, VifUsn::SIGNED);
		
		dest.pad(0x10, 0);
		s64 common_ofs = dest.tell();
		header.shared_ofs = checked_int_cast<u16>(common_ofs - tfrag_ofs);
		
		// common
		Buffer vu_header((u8*) &tfrag.common_vu_header, (u8*) (&tfrag.common_vu_header + 1));
		write_unpack(dest, vu_header, VifVnVl::V4_16, VifUsn::UNSIGNED);
		write_unpack(dest, tfrag.common_textures, VifVnVl::V4_32, VifUsn::SIGNED);
		write_strow(dest, sth_second_level_indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.common_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED);
		write_strow(dest, tfrag.base_position);
		dest.write<u32>(0x01000102); // stcycl
		write_unpack(dest, tfrag.common_positions, VifVnVl::V3_16, VifUsn::SIGNED);
		dest.write<u32>(0x01000404); // stcycl
		dest.write<u32>(0x05000000); // stmod
		
		dest.pad(0x10, 0);
		s64 lod_1_ofs = dest.tell();
		header.lod_1_ofs = checked_int_cast<u16>(lod_1_ofs - tfrag_ofs);
		
		// LOD 1
		write_unpack(dest, tfrag.lod_1_faces, VifVnVl::V4_8, VifUsn::SIGNED);
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_1_indices, VifVnVl::V4_8, VifUsn::UNSIGNED);
		
		dest.pad(0x10, 0);
		s64 lod_01_ofs = dest.tell();
		header.lod_0_ofs = checked_int_cast<u16>(lod_01_ofs - tfrag_ofs);
		
		// LOD 01
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		for(const std::vector<u8>& data : tfrag.lod_01_unknown_indices) {
			write_unpack(dest, data, VifVnVl::V4_8, VifUsn::UNSIGNED);
		}
		write_strow(dest, sth_second_level_indices_strow);
		if(!tfrag.lod_01_vertex_info.empty()) {
			write_unpack(dest, tfrag.lod_01_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED);
		}
		write_strow(dest, tfrag.base_position);
		dest.write<u32>(0x01000102); // stcycl
		if(!tfrag.lod_01_positions.empty()) {
			write_unpack(dest, tfrag.lod_01_positions, VifVnVl::V3_16, VifUsn::SIGNED);
		}
		
		dest.pad(0x10);
		s64 lod_0_ofs = dest.tell();
		
		// LOD 0
		if(!tfrag.lod_0_positions.empty()) {
			write_unpack(dest, tfrag.lod_0_positions, VifVnVl::V3_16, VifUsn::SIGNED);
		}
		dest.write<u32>(0x05000000); // stmod
		dest.write<u32>(0x01000404); // stcycl
		write_unpack(dest, tfrag.lod_0_faces, VifVnVl::V4_8, VifUsn::SIGNED);
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_0_indices, VifVnVl::V4_8, VifUsn::UNSIGNED);
		for(const std::vector<u8>& data : tfrag.lod_0_unknown_indices) {
			write_unpack(dest, data, VifVnVl::V4_8, VifUsn::UNSIGNED);
		}
		write_strow(dest, sth_second_level_indices_strow);
		if(!tfrag.lod_0_vertex_info.empty()) {
			write_unpack(dest, tfrag.lod_0_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED);
		}
		dest.write<u32>(0x005000000); // stmod
		
		dest.pad(0x10, 0);
		s64 end_ofs = dest.tell();
		header.lod_0_size = checked_int_cast<u8>((end_ofs - lod_0_ofs) / 0x10);
		
		// Fill in VIF command list sizes.
		header.common_size = checked_int_cast<u8>((lod_1_ofs - common_ofs) / 0x10);
		header.lod_2_size = checked_int_cast<u8>((lod_1_ofs - tfrag_ofs) / 0x10);
		header.lod_1_size = checked_int_cast<u8>((lod_0_ofs - common_ofs) / 0x10);
		header.lod_0_size = checked_int_cast<u8>((end_ofs - lod_01_ofs) / 0x10);
		
		// RGBA
		dest.pad(0x10, 0);
		header.lod_2_rgba_count = tfrag.lod_2_rgba_count;
		header.lod_1_rgba_count = tfrag.lod_1_rgba_count;
		header.lod_0_rgba_count = tfrag.lod_0_rgba_count;
		header.rgba_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		header.rgba_size = checked_int_cast<u8>((tfrag.rgbas.size() + 3) / 4);
		dest.write_multiple(tfrag.rgbas);
		
		dest.pad(0x10, 0);
		header.light_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		dest.write_multiple(tfrag.light);
		
		dest.pad(0x10);
		header.msphere_ofs = dest.tell();
		dest.write_multiple(tfrag.msphere);
		
		dest.pad(0x10);
		header.cube_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		dest.write(tfrag.cube);
		
		dest.write(next_header_ofs, header);
		next_header_ofs += sizeof(TfragHeader);
	}
	
	dest.write(table_header_ofs, table_header);
}

TfragHighestLod extract_highest_tfrag_lod(Tfrag tfrag) {
	TfragHighestLod highest_lod;
	highest_lod.bsphere = tfrag.bsphere;
	highest_lod.base_position = tfrag.base_position;
	highest_lod.common_textures = tfrag.common_textures;
	highest_lod.faces = std::move(tfrag.lod_0_faces);
	highest_lod.indices.insert(highest_lod.indices.end(), BEGIN_END(tfrag.lod_0_indices));
	highest_lod.vertex_info.insert(highest_lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	highest_lod.vertex_info.insert(highest_lod.vertex_info.end(), BEGIN_END(tfrag.lod_01_vertex_info));
	highest_lod.vertex_info.insert(highest_lod.vertex_info.end(), BEGIN_END(tfrag.lod_0_vertex_info));
	highest_lod.positions.insert(highest_lod.positions.end(), BEGIN_END(tfrag.common_positions));
	highest_lod.positions.insert(highest_lod.positions.end(), BEGIN_END(tfrag.lod_01_positions));
	highest_lod.positions.insert(highest_lod.positions.end(), BEGIN_END(tfrag.lod_0_positions));
	highest_lod.rgbas = std::move(tfrag.rgbas);
	highest_lod.light = std::move(tfrag.light);
	highest_lod.msphere = std::move(tfrag.msphere);
	highest_lod.cube = tfrag.cube;
	return highest_lod;
}

ColladaScene recover_tfrags(const std::vector<TfragHighestLod>& tfrags) {
	s32 texture_count = 0;
	for(const TfragHighestLod& tfrag : tfrags) {
		for(const TfragTexturePrimitive& primitive : tfrag.common_textures) {
			texture_count = std::max(texture_count, primitive.d0_tex0_1.data_lo + 1);
		}
	}
	
	ColladaScene scene;
	
	Mesh& mesh = scene.meshes.emplace_back();
	mesh.name = "mesh";
	mesh.flags |= MESH_HAS_TEX_COORDS;
	
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
	
	SubMesh* submesh = nullptr;
	s32 next_texture = 0;
	
	for(const TfragHighestLod& tfrag : tfrags) {
		s32 vertex_base = (s32) mesh.vertices.size();
		for(const TfragVertexInfo& src : tfrag.vertex_info) {
			Vertex& dest = mesh.vertices.emplace_back();
			s16 index = src.vertex_data_offsets[1] / 2;
			assert(index >= 0 && index < tfrag.positions.size());
			const TfragVertexPosition& pos = tfrag.positions[index];
			dest.pos.x = (tfrag.base_position.vif1_r0 + pos.x) / 1024.f;
			dest.pos.y = (tfrag.base_position.vif1_r1 + pos.y) / 1024.f;
			dest.pos.z = (tfrag.base_position.vif1_r2 + pos.z) / 1024.f;
			dest.tex_coord.s = vu_fixed12_to_float(src.s);
			dest.tex_coord.t = vu_fixed12_to_float(src.t);
		}
		s32 index_offset = 0;
		for(const Tface& face : tfrag.faces) {
			s8 vertex_count = face.vertex_count_and_flag;
			if(vertex_count <= 0) {
				if(vertex_count == 0) {
					break;
				} else if(face.end_of_packet_flag >= 0 && texture_count != 0) {
					next_texture = tfrag.common_textures.at(face.ad_gif_offset / 0x5).d0_tex0_1.data_lo;
				}
				vertex_count += 128;
			}
			
			if(submesh == nullptr || next_texture != submesh->material) {
				submesh = &mesh.submeshes.emplace_back();
				submesh->material = next_texture;
			}
			
			s32 queue[2] = {};
			for(s32 i = 0; i < vertex_count; i++) {
				assert(index_offset < tfrag.indices.size());
				s32 index = tfrag.indices[index_offset++];
				assert(index >= 0 && index < tfrag.vertex_info.size());
				if(i >= 2) {
					submesh->faces.emplace_back(queue[0], queue[1], vertex_base + index);
				}
				queue[0] = queue[1];
				queue[1] = vertex_base + index;
			}
		}
	}
	
	return scene;
}
