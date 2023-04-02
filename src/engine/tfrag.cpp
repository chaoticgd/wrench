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

#include "tfrag.h"

#define TFRAG_DEBUG_RECOVER_ALL_LODS
#define TFRAG_DEBUG_RAINBOW_STRIPS

static s32 count_triangles(const Tfrag& tfrag);
static TfragLod extract_highest_tfrag_lod(const Tfrag& tfrag);
static TfragLod extract_medium_tfrag_lod(const Tfrag& tfrag);
static TfragLod extract_low_tfrag_lod(const Tfrag& tfrag);
void recover_tfrag_lod(Mesh& mesh, const TfragLod& lod, s32 texture_count);
static s32 recover_tfrag_vertices(Mesh& mesh, const TfragLod& lod, s32 strip_index);

template <typename T>
static std::vector<T> read_unpack(const VifPacket& packet, VifVnVl vnvl) {
	verify(packet.code.is_unpack() && packet.code.unpack.vnvl == vnvl, "Bad VIF command.");
	return packet.data.read_all<T>().copy();
}

Tfrags read_tfrags(Buffer src, Game game) {
	Tfrags tfrags;
	
	TfragsHeader table_header = src.read<TfragsHeader>(0, "tfrags header");
	tfrags.thingy = table_header.thingy;
	tfrags.mysterious_second_thingy = table_header.mysterious_second_thingy;
	tfrags.fragments.reserve(table_header.tfrag_count);
	
	auto table = src.read_multiple<TfragHeader>(table_header.table_offset, table_header.tfrag_count, "tfrag table");
	for(size_t i = 0; i < table.size(); i++) {
		const TfragHeader& header = table[i];
		
		ERROR_CONTEXT("tfrag %d", i);
		
		Tfrag& tfrag = tfrags.fragments.emplace_back();
		Buffer data = src.subbuf(table_header.table_offset + header.data);
		
		tfrag.bsphere = header.bsphere;
		tfrag.lod_2_rgba_count = header.lod_2_rgba_count;
		tfrag.lod_1_rgba_count = header.lod_1_rgba_count;
		tfrag.lod_0_rgba_count = header.lod_0_rgba_count;
		tfrag.base_only = header.base_only;
		tfrag.rgba_verts_loc = header.rgba_verts_loc;
		tfrag.flags = header.flags;
		tfrag.occl_index = header.occl_index;
		tfrag.mip_dist = header.mip_dist;
		
		// LOD 2
		Buffer lod_2_buffer = data.subbuf(header.lod_2_ofs, header.shared_ofs - header.lod_2_ofs);
		std::vector<VifPacket> lod_2_command_list = read_vif_command_list(lod_2_buffer);
		std::vector<VifPacket> lod_2 = filter_vif_unpacks(lod_2_command_list);
		verify(lod_2.size() == 2, "Incorrect number of LOD 2 VIF unpacks! %d");
		
		tfrag.lod_2_indices = read_unpack<u8>(lod_2[0], VifVnVl::V4_8);
		tfrag.memory_map.indices_addr = lod_2[0].code.unpack.addr;
		tfrag.lod_2_strips = read_unpack<TfragStrip>(lod_2[1], VifVnVl::V4_8);
		tfrag.memory_map.strips_addr = lod_2[1].code.unpack.addr;
		
		// Common
		Buffer common_buffer = data.subbuf(header.shared_ofs, header.lod_1_ofs - header.shared_ofs);
		std::vector<VifPacket> common_command_list = read_vif_command_list(common_buffer);
		verify(common_command_list.size() > 5, "Too few shared VIF commands.");
		tfrag.base_position = common_command_list[5].data.read<VifSTROW>(0, "base position");
		std::vector<VifPacket> common = filter_vif_unpacks(common_command_list);
		verify(common.size() == 4, "Incorrect number of shared VIF unpacks!");
		
		tfrag.common_vu_header = common[0].data.read<TfragHeaderUnpack>(0, "VU header");
		tfrag.memory_map.header_common_addr = common[0].code.unpack.addr;
		tfrag.common_textures = read_unpack<TfragTexturePrimitive>(common[1], VifVnVl::V4_32);
		tfrag.memory_map.ad_gifs_common_addr = common[1].code.unpack.addr;
		tfrag.common_vertex_info = read_unpack<TfragVertexInfo>(common[2], VifVnVl::V4_16);
		tfrag.memory_map.vertex_info_common_addr = common[2].code.unpack.addr;
		tfrag.common_positions = read_unpack<TfragVertexPosition>(common[3], VifVnVl::V3_16);
		tfrag.memory_map.positions_common_addr = common[3].code.unpack.addr;
		
		// LOD 1
		Buffer lod_1_buffer = data.subbuf(header.lod_1_ofs, header.lod_0_ofs - header.lod_1_ofs);
		std::vector<VifPacket> lod_1_command_list = read_vif_command_list(lod_1_buffer);
		std::vector<VifPacket> lod_1 = filter_vif_unpacks(lod_1_command_list);
		verify(lod_1.size() == 2, "Incorrect number of LOD 1 VIF unpacks!");
		
		tfrag.lod_1_strips = read_unpack<TfragStrip>(lod_1[0], VifVnVl::V4_8);
		verify(tfrag.memory_map.strips_addr == lod_1[0].code.unpack.addr, "Weird tfrag.");
		tfrag.lod_1_indices = read_unpack<u8>(lod_1[1], VifVnVl::V4_8);
		verify(tfrag.memory_map.indices_addr == lod_1[1].code.unpack.addr, "Weird tfrag.");
		
		// LOD 01
		Buffer lod_01_buffer = data.subbuf(
			header.lod_0_ofs,
			header.shared_ofs + header.lod_1_size * 0x10 - header.lod_0_ofs);
		std::vector<VifPacket> lod_01_command_list = read_vif_command_list(lod_01_buffer);
		std::vector<VifPacket> lod_01 = filter_vif_unpacks(lod_01_command_list);
		
		s32 j = 0;
		if(j < lod_01.size() && lod_01[j].code.unpack.vnvl == VifVnVl::V4_8) {
			tfrag.lod_01_unknown_indices = read_unpack<u8>(lod_01[j], VifVnVl::V4_8);
			tfrag.memory_map.unk_indices_lod_01_addr = lod_01[j].code.unpack.addr;
			j++;
		}
		if(j < lod_01.size() && lod_01[j].code.unpack.vnvl == VifVnVl::V4_8) {
			tfrag.lod_01_unknown_indices_2 = read_unpack<u8>(lod_01[j], VifVnVl::V4_8);
			tfrag.memory_map.unk_indices_2_lod_01_addr = lod_01[j].code.unpack.addr;
			j++;
		}
		if(j < lod_01.size() && lod_01[j].code.unpack.vnvl == VifVnVl::V4_16) {
			tfrag.lod_01_vertex_info = read_unpack<TfragVertexInfo>(lod_01[j], VifVnVl::V4_16);
			tfrag.memory_map.vertex_info_lod_01_addr = lod_01[j].code.unpack.addr;
			j++;
		}
		if(j < lod_01.size() && lod_01[j].code.unpack.vnvl == VifVnVl::V3_16) {
			tfrag.lod_01_positions = read_unpack<TfragVertexPosition>(lod_01[j], VifVnVl::V3_16);
			tfrag.memory_map.positions_lod_01_addr = lod_01[j].code.unpack.addr;
			j++;
		}
		
		// LOD 0
		Buffer lod_0_buffer = data.subbuf(
			header.shared_ofs + header.lod_1_size * 0x10,
			header.rgba_ofs - (header.lod_1_size + header.lod_2_size - header.common_size) * 0x10);
		std::vector<VifPacket> lod_0_command_list = read_vif_command_list(lod_0_buffer);
		std::vector<VifPacket> lod_0 = filter_vif_unpacks(lod_0_command_list);
		
		j = 0;
		if(j < lod_0.size() && lod_0[j].code.unpack.vnvl == VifVnVl::V3_16) {
			tfrag.lod_0_positions = read_unpack<TfragVertexPosition>(lod_0[j], VifVnVl::V3_16);
			tfrag.memory_map.positions_lod_0_addr = lod_0[j].code.unpack.addr;
			j++;
		}
		verify(j < lod_0.size(), "Too few LOD 0 VIF unpacks!");
		tfrag.lod_0_strips = read_unpack<TfragStrip>(lod_0[j], VifVnVl::V4_8);
		verify(tfrag.memory_map.strips_addr == lod_0[j].code.unpack.addr, "Weird tfrag.");
		j++;
		verify(j < lod_0.size(), "Too few LOD 0 VIF unpacks!");
		tfrag.lod_0_indices = read_unpack<u8>(lod_0[j], VifVnVl::V4_8);
		verify(tfrag.memory_map.indices_addr == lod_0[j].code.unpack.addr, "Weird tfrag.");
		j++;
		if(j < lod_0.size() && lod_0[j].code.unpack.vnvl == VifVnVl::V4_8) {
			tfrag.lod_0_unknown_indices = read_unpack<u8>(lod_0[j], VifVnVl::V4_8);
			tfrag.memory_map.unk_indices_lod_0_addr = lod_0[j].code.unpack.addr;
			j++;
		}
		if(j < lod_0.size() && lod_0[j].code.unpack.vnvl == VifVnVl::V4_8) {
			tfrag.lod_0_unknown_indices_2 = read_unpack<u8>(lod_0[j], VifVnVl::V4_8);
			tfrag.memory_map.unk_indices_2_lod_0_addr = lod_0[j].code.unpack.addr;
			j++;
		}
		if(j < lod_0.size() && lod_0[j].code.unpack.vnvl == VifVnVl::V4_16) {
			tfrag.lod_0_vertex_info = read_unpack<TfragVertexInfo>(lod_0[j], VifVnVl::V4_16);
			tfrag.memory_map.vertex_info_lod_0_addr = lod_0[j].code.unpack.addr;
			j++;
		}
		
		tfrag.rgbas = data.read_multiple<TfragRgba>(header.rgba_ofs, header.rgba_size * 4, "rgbas").copy();
		tfrag.light = data.read_multiple<u8>(header.light_ofs, header.msphere_ofs - header.light_ofs, "light").copy();
		tfrag.msphere = data.read_multiple<Vec4f>(header.msphere_ofs, header.msphere_count, "mspheres").copy();
		tfrag.cube = data.read<TfragCube>(header.cube_ofs, "cube");
	}
	
	return tfrags;
}

template <typename Dest, typename Src>
static Dest checked_int_cast(Src src) {
	assert(src >= std::numeric_limits<Dest>::min()
		&& src <= std::numeric_limits<Dest>::max());
	return static_cast<Dest>(src);
}

static void write_unpack(OutBuffer dest, Buffer data, VifVnVl vnvl, VifUsn usn, s32 addr) {
	VifPacket packet;
	packet.code.interrupt = 0;
	packet.code.cmd = (VifCmd) 0b1100000; // UNPACK
	packet.code.unpack.vnvl = vnvl;
	packet.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	packet.code.unpack.usn = usn;
	packet.code.unpack.addr = addr;
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

void write_tfrags(OutBuffer dest, const Tfrags& tfrags, Game game) {
	s64 table_header_ofs = dest.alloc<TfragsHeader>();
	TfragsHeader table_header = {};
	dest.pad(0x40);
	s64 table_ofs = dest.alloc_multiple<TfragHeader>(tfrags.fragments.size());
	s64 next_header_ofs = table_ofs;
	table_header.table_offset = (s32) next_header_ofs;
	table_header.tfrag_count = (s32) tfrags.fragments.size();
	table_header.thingy = tfrags.thingy;
	table_header.mysterious_second_thingy = tfrags.mysterious_second_thingy;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragHeader header = {};
		TfragHeaderUnpack unpack_header = tfrag.common_vu_header;
		
		header.bsphere = tfrag.bsphere;
		header.lod_2_rgba_count = tfrag.lod_2_rgba_count;
		header.lod_1_rgba_count = tfrag.lod_1_rgba_count;
		header.lod_0_rgba_count = tfrag.lod_0_rgba_count;
		header.base_only = tfrag.base_only;
		header.rgba_verts_loc = tfrag.rgba_verts_loc;
		header.flags = tfrag.flags;
		header.dir_lights_one = 0xff;
		header.point_lights = 0xffff;
		header.occl_index = tfrag.occl_index;
		header.vert_count = checked_int_cast<u8>(
			tfrag.common_positions.size() +
			tfrag.lod_01_positions.size() +
			tfrag.lod_0_positions.size());
		header.tri_count = checked_int_cast<u8>(count_triangles(tfrag));
		header.mip_dist = tfrag.mip_dist;
		
		dest.pad(0x10, 0);
		s64 tfrag_ofs = dest.tell();
		header.data = checked_int_cast<s32>(tfrag_ofs - table_ofs);
		header.lod_2_ofs = 0;
		
		// Prepare STROW data.
		VifSTROW single_vertex_info_strow = {
			0x45000000,
			0x45000000,
			0,
			tfrag.memory_map.positions_common_addr
		};
		VifSTROW double_vertex_info_strow = {
			0x45000000,
			0x45000000,
			tfrag.memory_map.positions_common_addr,
			tfrag.memory_map.positions_common_addr
		};
		VifSTROW indices_strow = {
			tfrag.memory_map.vertex_info_common_addr,
			tfrag.memory_map.vertex_info_common_addr,
			tfrag.memory_map.vertex_info_common_addr,
			tfrag.memory_map.vertex_info_common_addr
		};
		
		// LOD 2
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_2_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.indices_addr);
		dest.write<u32>(0x05000000); // stmod
		write_unpack(dest, tfrag.lod_2_strips, VifVnVl::V4_8, VifUsn::SIGNED, tfrag.memory_map.strips_addr);
		
		dest.pad(0x10, 0);
		s64 common_ofs = dest.tell();
		header.shared_ofs = checked_int_cast<u16>(common_ofs - tfrag_ofs);
		
		// common
		Buffer vu_header((u8*) &unpack_header, (u8*) (&unpack_header + 1));
		write_unpack(dest, vu_header, VifVnVl::V4_16, VifUsn::UNSIGNED, tfrag.memory_map.header_common_addr);
		header.tex_ofs = checked_int_cast<u16>(dest.tell() + 4 - tfrag_ofs);
		write_unpack(dest, tfrag.common_textures, VifVnVl::V4_32, VifUsn::SIGNED, tfrag.memory_map.ad_gifs_common_addr);
		write_strow(dest, single_vertex_info_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.common_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED, tfrag.memory_map.vertex_info_common_addr);
		write_strow(dest, tfrag.base_position);
		dest.write<u32>(0x01000102); // stcycl
		if(game != Game::RAC) {
			header.light_vert_start_ofs_gc_uya_dl = checked_int_cast<u16>(dest.tell() + 4 - tfrag_ofs);
		}
		write_unpack(dest, tfrag.common_positions, VifVnVl::V3_16, VifUsn::SIGNED, tfrag.memory_map.positions_common_addr);
		dest.write<u32>(0x01000404); // stcycl
		dest.write<u32>(0x05000000); // stmod
		
		dest.pad(0x10, 0);
		s64 lod_1_ofs = dest.tell();
		header.lod_1_ofs = checked_int_cast<u16>(lod_1_ofs - tfrag_ofs);
		
		// LOD 1
		write_unpack(dest, tfrag.lod_1_strips, VifVnVl::V4_8, VifUsn::SIGNED, tfrag.memory_map.strips_addr);
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_1_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.indices_addr);
		
		dest.pad(0x10, 0);
		s64 lod_01_ofs = dest.tell();
		header.lod_0_ofs = checked_int_cast<u16>(lod_01_ofs - tfrag_ofs);
		
		// LOD 01
		if(game == Game::RAC || !tfrag.lod_01_unknown_indices.empty() || !tfrag.lod_01_unknown_indices_2.empty()) {
			write_strow(dest, indices_strow);
		}
		// R&C1 didn't optimise the VIF command lists so much.
		bool lod_01_needs_stmod = game == Game::RAC
			|| !tfrag.lod_01_unknown_indices.empty()
			|| !tfrag.lod_01_unknown_indices_2.empty()
			|| !tfrag.lod_01_vertex_info.empty()
			|| !tfrag.lod_01_positions.empty()
			|| !tfrag.lod_0_positions.empty();
		if(lod_01_needs_stmod) {
			dest.write<u32>(0x05000001); // stmod
		}
		if(!tfrag.lod_01_unknown_indices.empty()) {
			write_unpack(dest, tfrag.lod_01_unknown_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.unk_indices_lod_01_addr);
		}
		if(!tfrag.lod_01_unknown_indices_2.empty()) {
			write_unpack(dest, tfrag.lod_01_unknown_indices_2, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.unk_indices_2_lod_01_addr);
		}
		if(game == Game::RAC || !tfrag.lod_01_vertex_info.empty()) {
			write_strow(dest, double_vertex_info_strow);
		}
		if(!tfrag.lod_01_vertex_info.empty()) {
			write_unpack(dest, tfrag.lod_01_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED, tfrag.memory_map.vertex_info_lod_01_addr);
		}
		write_strow(dest, tfrag.base_position);
		dest.write<u32>(0x01000102); // stcycl
		if(!tfrag.lod_01_positions.empty()) {
			write_unpack(dest, tfrag.lod_01_positions, VifVnVl::V3_16, VifUsn::SIGNED, tfrag.memory_map.positions_lod_01_addr);
		}
		
		dest.pad(0x10);
		s64 lod_0_ofs = dest.tell();
		
		// LOD 0
		if(!tfrag.lod_0_positions.empty()) {
			write_unpack(dest, tfrag.lod_0_positions, VifVnVl::V3_16, VifUsn::SIGNED, tfrag.memory_map.positions_lod_0_addr);
		}
		dest.write<u32>(0x05000000); // stmod
		dest.write<u32>(0x01000404); // stcycl
		write_unpack(dest, tfrag.lod_0_strips, VifVnVl::V4_8, VifUsn::SIGNED, tfrag.memory_map.strips_addr);
		write_strow(dest, indices_strow);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_0_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.indices_addr);
		if(!tfrag.lod_0_unknown_indices.empty()) {
			write_unpack(dest, tfrag.lod_0_unknown_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.unk_indices_lod_0_addr);
		}
		if(!tfrag.lod_0_unknown_indices_2.empty()) {
			write_unpack(dest, tfrag.lod_0_unknown_indices_2, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.unk_indices_2_lod_0_addr);
		}
		if(game == Game::RAC || !tfrag.lod_0_vertex_info.empty()) {
			write_strow(dest, double_vertex_info_strow);
		}
		if(!tfrag.lod_0_vertex_info.empty()) {
			write_unpack(dest, tfrag.lod_0_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED, tfrag.memory_map.vertex_info_lod_0_addr);
		}
		dest.write<u32>(0x005000000); // stmod
		
		dest.pad(0x10, 0);
		s64 end_ofs = dest.tell();
		
		// Fill in VIF command list sizes.
		header.common_size = checked_int_cast<u8>((lod_1_ofs - common_ofs) / 0x10);
		header.lod_2_size = checked_int_cast<u8>((lod_1_ofs - tfrag_ofs) / 0x10);
		header.lod_1_size = checked_int_cast<u8>((lod_0_ofs - common_ofs) / 0x10);
		header.lod_0_size = checked_int_cast<u8>((end_ofs - lod_01_ofs) / 0x10);
		
		header.texture_count = checked_int_cast<u8>(tfrag.common_textures.size());
		
		// RGBA
		dest.pad(0x10, 0);
		header.rgba_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		header.rgba_size = checked_int_cast<u8>((tfrag.rgbas.size() + 3) / 4);
		dest.write_multiple(tfrag.rgbas);
		
		dest.pad(0x10, 0);
		header.light_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		dest.write_multiple(tfrag.light);
		
		dest.pad(0x10);
		header.msphere_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		if(game == Game::RAC) {
			header.msphere_ofs_2_rac = header.msphere_ofs;
		}
		header.msphere_count = checked_int_cast<u8>(tfrag.msphere.size());
		dest.write_multiple(tfrag.msphere);
		
		dest.pad(0x10);
		header.cube_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		dest.write(tfrag.cube);
		
		dest.write(next_header_ofs, header);
		next_header_ofs += sizeof(TfragHeader);
	}
	
	dest.write(table_header_ofs, table_header);
}

static s32 count_triangles(const Tfrag& tfrag) {
	s32 triangles = 0;
	for(const TfragStrip& strip : tfrag.lod_0_strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if(vertex_count <= 0) {
			if(vertex_count == 0) {
				break;
			}
			vertex_count += 128;
		}
		triangles += vertex_count - 2;
	}
	return triangles;
}

void allocate_tfrags_vu(Tfrags& tfrags) {
	static const s32 VU1_BUFFER_SIZE = 0x118;
	
	for(Tfrag& tfrag : tfrags.fragments) {
		// Calculate sizes in VU memory.
		s32 header_common_size = 5;
		s32 matrix_size = 4;
		s32 ad_gifs_common_size = tfrag.common_textures.size() * (sizeof(TfragTexturePrimitive) / 16);
		s32 positions_common_size = tfrag.common_positions.size() * 2;
		s32 positions_lod_01_size = tfrag.lod_01_positions.size() * 2;
		s32 positions_lod_0_size = tfrag.lod_0_positions.size() * 2;
		s32 vertex_info_common_size = tfrag.common_vertex_info.size();
		s32 vertex_info_lod_01_size = tfrag.lod_01_vertex_info.size();
		s32 vertex_info_lod_0_size = tfrag.lod_0_vertex_info.size();
		s32 unk_indices_lod_01_size = align32(tfrag.lod_01_unknown_indices.size(), 4) / 4;
		s32 unk_indices_2_lod_01_size = align32(tfrag.lod_01_unknown_indices_2.size(), 4) / 4;
		s32 unk_indices_lod_0_size = align32(tfrag.lod_0_unknown_indices.size(), 4) / 4;
		s32 unk_indices_2_lod_0_size = align32(tfrag.lod_0_unknown_indices_2.size(), 4) / 4;
		s32 indices_size = align32(std::max({tfrag.lod_0_indices.size(), tfrag.lod_1_indices.size(), tfrag.lod_2_indices.size()}), 4) / 4;
		s32 strips_size = std::max({tfrag.lod_0_strips.size(), tfrag.lod_1_strips.size(), tfrag.lod_2_strips.size()});
		
		// Calculate addresses in VU memory.
		tfrag.memory_map.header_common_addr = 0;
		tfrag.memory_map.ad_gifs_common_addr = tfrag.memory_map.header_common_addr + header_common_size + matrix_size;
		tfrag.memory_map.positions_common_addr = tfrag.memory_map.ad_gifs_common_addr + ad_gifs_common_size;
		tfrag.memory_map.positions_lod_01_addr = tfrag.memory_map.positions_common_addr + positions_common_size;
		tfrag.memory_map.positions_lod_0_addr = tfrag.memory_map.positions_lod_01_addr + positions_lod_01_size;
		//tfrag.memory_map.vertex_info_common_addr = tfrag.memory_map.positions_lod_0_addr + positions_lod_0_size + 2;
		tfrag.memory_map.vertex_info_lod_01_addr = tfrag.memory_map.vertex_info_common_addr + vertex_info_common_size;
		tfrag.memory_map.vertex_info_lod_0_addr = tfrag.memory_map.vertex_info_lod_01_addr + vertex_info_lod_01_size;
		tfrag.memory_map.unk_indices_lod_01_addr = tfrag.memory_map.vertex_info_lod_0_addr + vertex_info_lod_0_size;
		tfrag.memory_map.unk_indices_2_lod_01_addr = tfrag.memory_map.unk_indices_lod_01_addr + unk_indices_lod_01_size;
		tfrag.memory_map.unk_indices_lod_0_addr = tfrag.memory_map.unk_indices_2_lod_01_addr + unk_indices_2_lod_01_size;
		tfrag.memory_map.unk_indices_2_lod_0_addr = tfrag.memory_map.unk_indices_lod_0_addr + unk_indices_lod_0_size;
		tfrag.memory_map.indices_addr = tfrag.memory_map.unk_indices_2_lod_0_addr + unk_indices_2_lod_0_size;
		tfrag.memory_map.strips_addr = tfrag.memory_map.indices_addr + indices_size;
		
		if(positions_lod_01_size == 0) tfrag.memory_map.positions_lod_01_addr = -1;
		if(positions_lod_0_size == 0) tfrag.memory_map.positions_lod_0_addr = -1;
		if(vertex_info_lod_01_size == 0) tfrag.memory_map.vertex_info_lod_01_addr = -1;
		if(vertex_info_lod_0_size == 0) tfrag.memory_map.vertex_info_lod_0_addr = -1;
		if(unk_indices_lod_01_size == 0) tfrag.memory_map.unk_indices_lod_01_addr = -1;
		if(unk_indices_lod_0_size == 0) tfrag.memory_map.unk_indices_lod_0_addr = -1;
		
		s32 end_addr = tfrag.memory_map.strips_addr + strips_size;
		//assert(end_addr <= VU1_BUFFER_SIZE);
	}
}

ColladaScene recover_tfrags(const Tfrags& tfrags) {
	s32 texture_count = 0;
	for(const Tfrag& tfrag : tfrags.fragments) {
		for(const TfragTexturePrimitive& primitive : tfrag.common_textures) {
			texture_count = std::max(texture_count, primitive.d1_tex0_1.data_lo + 1);
		}
	}
	
	ColladaScene scene;
	
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

#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
	u32 mesh_flags = MESH_HAS_TEX_COORDS | MESH_HAS_VERTEX_COLOURS;
#else
	u32 mesh_flags = MESH_HAS_TEX_COORDS;
#endif

	Mesh& high_mesh = scene.meshes.emplace_back();
	high_mesh.name = "mesh";
	high_mesh.flags |= mesh_flags;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragLod lod = extract_highest_tfrag_lod(tfrag);
		recover_tfrag_lod(high_mesh, lod, texture_count);
	}

#ifdef TFRAG_DEBUG_RECOVER_ALL_LODS
	Mesh& medium_mesh = scene.meshes.emplace_back();
	medium_mesh.name = "medium_lod";
	medium_mesh.flags |= mesh_flags;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragLod lod = extract_medium_tfrag_lod(tfrag);
		recover_tfrag_lod(medium_mesh, lod, texture_count);
	}
	
	Mesh& low_mesh = scene.meshes.emplace_back();
	low_mesh.name = "low_lod";
	low_mesh.flags |= mesh_flags;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		TfragLod lod = extract_low_tfrag_lod(tfrag);
		recover_tfrag_lod(low_mesh, lod, texture_count);
	}
#endif
	
	return scene;
}

static TfragLod extract_highest_tfrag_lod(const Tfrag& tfrag) {
	TfragLod lod;
	lod.bsphere = tfrag.bsphere;
	lod.base_position = tfrag.base_position;
	lod.common_textures = tfrag.common_textures;
	lod.strips = tfrag.lod_0_strips;
	lod.indices.insert(lod.indices.end(), BEGIN_END(tfrag.lod_0_indices));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.lod_01_vertex_info));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.lod_0_vertex_info));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.common_positions));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.lod_01_positions));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.lod_0_positions));
	lod.rgbas = tfrag.rgbas;
	lod.light = tfrag.light;
	lod.msphere = tfrag.msphere;
	lod.cube = tfrag.cube;
	return lod;
}

static TfragLod extract_medium_tfrag_lod(const Tfrag& tfrag) {
	TfragLod lod;
	lod.bsphere = tfrag.bsphere;
	lod.base_position = tfrag.base_position;
	lod.common_textures = tfrag.common_textures;
	lod.strips = tfrag.lod_1_strips;
	lod.indices.insert(lod.indices.end(), BEGIN_END(tfrag.lod_1_indices));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.lod_01_vertex_info));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.common_positions));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.lod_01_positions));
	lod.rgbas = tfrag.rgbas;
	lod.light = tfrag.light;
	lod.msphere = tfrag.msphere;
	lod.cube = tfrag.cube;
	return lod;
}

static TfragLod extract_low_tfrag_lod(const Tfrag& tfrag) {
	TfragLod lod;
	lod.bsphere = tfrag.bsphere;
	lod.base_position = tfrag.base_position;
	lod.common_textures = tfrag.common_textures;
	lod.strips = tfrag.lod_2_strips;
	lod.indices.insert(lod.indices.end(), BEGIN_END(tfrag.lod_2_indices));
	lod.vertex_info.insert(lod.vertex_info.end(), BEGIN_END(tfrag.common_vertex_info));
	lod.positions.insert(lod.positions.end(), BEGIN_END(tfrag.common_positions));
	lod.rgbas = tfrag.rgbas;
	lod.light = tfrag.light;
	lod.msphere = tfrag.msphere;
	lod.cube = tfrag.cube;
	return lod;
}

void recover_tfrag_lod(Mesh& mesh, const TfragLod& lod, s32 texture_count) {
	SubMesh* submesh = nullptr;
	s32 next_texture = 0;
	
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
	static s32 strip_index = 0;
#else
	s32 vertex_base = recover_tfrag_vertices(mesh, lod, 0);
#endif
	
	s32 index_offset = 0;
	for(const TfragStrip& strip : lod.strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if(vertex_count <= 0) {
			if(vertex_count == 0) {
				break;
			} else if(strip.end_of_packet_flag >= 0 && texture_count != 0) {
				next_texture = lod.common_textures.at(strip.ad_gif_offset / 0x5).d1_tex0_1.data_lo;
			}
			vertex_count += 128;
		}
		
		if(submesh == nullptr || next_texture != submesh->material) {
			submesh = &mesh.submeshes.emplace_back();
			submesh->material = next_texture;
		}
		
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
		s32 vertex_base = recover_tfrag_vertices(mesh, lod, strip_index++);
#endif
		
		s32 queue[2] = {};
		for(s32 i = 0; i < vertex_count; i++) {
			assert(index_offset < lod.indices.size());
			s32 index = lod.indices[index_offset++];
			assert(index >= 0 && index < lod.vertex_info.size());
			if(i >= 2) {
				submesh->faces.emplace_back(queue[0], queue[1], vertex_base + index);
			}
			queue[0] = queue[1];
			queue[1] = vertex_base + index;
		}
	}
}

static s32 recover_tfrag_vertices(Mesh& mesh, const TfragLod& lod, s32 strip_index) {
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
	static const u8 colours[12][4] = {
		{255, 0,   0,   255},
		{255, 255, 0,   255},
		{0,   255, 0,   255},
		{0,   255, 255, 255},
		{0,   0,   255, 255},
		{255, 0,   255, 255},
		{128, 0,   0,   255},
		{128, 128, 0,   255},
		{0,   128, 0,   255},
		{0,   128, 128, 255},
		{0,   0,   128, 255},
		{128, 0,   128, 255}
	};
#endif
	
	s32 vertex_base = (s32) mesh.vertices.size();
	for(const TfragVertexInfo& src : lod.vertex_info) {
		Vertex& dest = mesh.vertices.emplace_back();
		s16 index = src.vertex_data_offsets[1] / 2;
		assert(index >= 0 && index < lod.positions.size());
		const TfragVertexPosition& pos = lod.positions[index];
		dest.pos.x = (lod.base_position.vif1_r0 + pos.x) / 1024.f;
		dest.pos.y = (lod.base_position.vif1_r1 + pos.y) / 1024.f;
		dest.pos.z = (lod.base_position.vif1_r2 + pos.z) / 1024.f;
		dest.tex_coord.s = vu_fixed12_to_float(src.s);
		dest.tex_coord.t = vu_fixed12_to_float(src.t);
#ifdef TFRAG_DEBUG_RAINBOW_STRIPS
		dest.colour.r = colours[strip_index % 12][0];
		dest.colour.g = colours[strip_index % 12][1];
		dest.colour.b = colours[strip_index % 12][2];
		dest.colour.a = colours[strip_index % 12][3];
#endif
	}
	return vertex_base;
}
