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

#include "tfrag_low.h"

#include <algorithm>

static s32 count_triangles(const Tfrag& tfrag);
static void read_tfrag_command_lists(Tfrag& tfrag, const TfragHeader& header, Buffer data);
static void write_tfrag_command_lists(
	OutBuffer dest, TfragHeader& header, const Tfrag& tfrag, s64 tfrag_ofs, Game game);

Tfrags read_tfrags(Buffer src, Game game)
{
	Tfrags tfrags;
	
	TfragsHeader table_header = src.read<TfragsHeader>(0, "tfrags header");
	tfrags.thingy = table_header.thingy;
	tfrags.mysterious_second_thingy = table_header.mysterious_second_thingy;
	tfrags.fragments.reserve(table_header.tfrag_count);
	
	auto table = src.read_multiple<TfragHeader>(table_header.table_offset, table_header.tfrag_count, "tfrag table");
	for (size_t i = 0; i < table.size(); i++) {
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
		
		// All the LODs.
		read_tfrag_command_lists(tfrag, header, data);
		
		tfrag.rgbas = data.read_multiple<TfragRgba>(header.rgba_ofs, header.rgba_size * 4, "rgbas").copy();
		tfrag.lights = data.read_multiple<TfragLight>(header.light_ofs + 0x10, header.vert_count, "light").copy();
		tfrag.msphere = data.read_multiple<Vec4f>(header.msphere_ofs, header.msphere_count, "mspheres").copy();
		tfrag.cube = data.read<TfragCube>(header.cube_ofs, "cube");
		
		s32 positions_end = 0;
		if (tfrag.memory_map.positions_lod_0_addr != -1) {
			positions_end = tfrag.memory_map.positions_lod_0_addr + tfrag.lod_0_positions.size() * 2;
		} else if (tfrag.memory_map.positions_lod_01_addr != -1) {
			positions_end = tfrag.memory_map.positions_lod_01_addr + tfrag.lod_01_positions.size() * 2;
		} else if (tfrag.memory_map.positions_common_addr != -1) {
			positions_end = tfrag.memory_map.positions_common_addr + tfrag.common_positions.size() * 2;
		} else {
			verify_not_reached("Bad tfrag positions.");
		}
		
		tfrag.positions_slack = tfrag.memory_map.vertex_info_common_addr - positions_end;
	}
	
	return tfrags;
}

void write_tfrags(OutBuffer dest, const Tfrags& tfrags, Game game)
{
	s64 table_header_ofs = dest.alloc<TfragsHeader>();
	TfragsHeader table_header = {};
	dest.pad(0x40);
	s64 table_ofs = dest.alloc_multiple<TfragHeader>(tfrags.fragments.size());
	s64 next_header_ofs = table_ofs;
	table_header.table_offset = (s32) next_header_ofs;
	table_header.tfrag_count = (s32) tfrags.fragments.size();
	table_header.thingy = tfrags.thingy;
	table_header.mysterious_second_thingy = tfrags.mysterious_second_thingy;
	
	for (const Tfrag& tfrag : tfrags.fragments) {
		TfragHeader header = {};
		
		header.bsphere = tfrag.bsphere;
		header.lod_2_rgba_count = tfrag.lod_2_rgba_count;
		header.lod_1_rgba_count = tfrag.lod_1_rgba_count;
		header.lod_0_rgba_count = tfrag.lod_0_rgba_count;
		header.base_only = tfrag.base_only;
		header.texture_count = checked_int_cast<u8>(tfrag.common_textures.size());
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
		
		// All the LODs.
		write_tfrag_command_lists(dest, header, tfrag, tfrag_ofs, game);
		
		// RGBA
		dest.pad(0x10, 0);
		header.rgba_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		header.rgba_size = checked_int_cast<u8>((tfrag.rgbas.size() + 3) / 4);
		dest.write_multiple(tfrag.rgbas);
		
		dest.pad(0x10, 0);
		header.light_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		dest.write(tfrag.base_position);
		dest.write_multiple(tfrag.lights);
		
		dest.pad(0x10);
		header.msphere_ofs = checked_int_cast<u16>(dest.tell() - tfrag_ofs);
		if (game != Game::DL) {
			header.light_end_ofs_rac_gc_uya = header.msphere_ofs;
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

static s32 count_triangles(const Tfrag& tfrag)
{
	s32 triangles = 0;
	for (const TfragStrip& strip : tfrag.lod_0_strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if (vertex_count <= 0) {
			if (vertex_count == 0) {
				break;
			}
			vertex_count += 128;
		}
		triangles += vertex_count - 2;
	}
	return triangles;
}

template <typename T>
static std::vector<T> read_unpack(const VifPacket& packet, VifVnVl vnvl)
{
	verify(packet.code.is_unpack() && packet.code.unpack.vnvl == vnvl, "Bad VIF command.");
	return packet.data.read_all<T>().copy();
}

static void read_tfrag_command_lists(Tfrag& tfrag, const TfragHeader& header, Buffer data)
{
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
	
	s32 i = 0;
	if (i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V4_8 && tfrag.common_vu_header.positions_lod_01_count > 0) {
		tfrag.lod_01_parent_indices = read_unpack<u8>(lod_01[i], VifVnVl::V4_8);
		tfrag.memory_map.parent_indices_lod_01_addr = lod_01[i].code.unpack.addr;
		s32 size_difference = (s32) tfrag.lod_01_parent_indices.size() - (s32) tfrag.common_vu_header.positions_lod_01_count;
		verify(size_difference >= 0 && size_difference < 4, "Parent indices array has bad size.");
		tfrag.lod_01_parent_indices.resize(tfrag.common_vu_header.positions_lod_01_count);
		i++;
	}
	if (i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V4_8 && lod_01[i].code.unpack.addr) {
		tfrag.lod_01_unknown_indices_2 = read_unpack<u8>(lod_01[i], VifVnVl::V4_8);
		tfrag.memory_map.unk_indices_2_lod_01_addr = lod_01[i].code.unpack.addr;
		i++;
	}
	if (i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V4_16) {
		tfrag.lod_01_vertex_info = read_unpack<TfragVertexInfo>(lod_01[i], VifVnVl::V4_16);
		tfrag.memory_map.vertex_info_lod_01_addr = lod_01[i].code.unpack.addr;
		i++;
	}
	if (i < lod_01.size() && lod_01[i].code.unpack.vnvl == VifVnVl::V3_16) {
		tfrag.lod_01_positions = read_unpack<TfragVertexPosition>(lod_01[i], VifVnVl::V3_16);
		tfrag.memory_map.positions_lod_01_addr = lod_01[i].code.unpack.addr;
		i++;
	}
	
	// LOD 0
	Buffer lod_0_buffer = data.subbuf(
		header.shared_ofs + header.lod_1_size * 0x10,
		header.rgba_ofs - (header.lod_1_size + header.lod_2_size - header.common_size) * 0x10);
	std::vector<VifPacket> lod_0_command_list = read_vif_command_list(lod_0_buffer);
	std::vector<VifPacket> lod_0 = filter_vif_unpacks(lod_0_command_list);
	
	i = 0;
	if (i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V3_16) {
		tfrag.lod_0_positions = read_unpack<TfragVertexPosition>(lod_0[i], VifVnVl::V3_16);
		tfrag.memory_map.positions_lod_0_addr = lod_0[i].code.unpack.addr;
		i++;
	}
	verify(i < lod_0.size(), "Too few LOD 0 VIF unpacks!");
	tfrag.lod_0_strips = read_unpack<TfragStrip>(lod_0[i], VifVnVl::V4_8);
	verify(tfrag.memory_map.strips_addr == lod_0[i].code.unpack.addr, "Weird tfrag.");
	i++;
	verify(i < lod_0.size(), "Too few LOD 0 VIF unpacks!");
	tfrag.lod_0_indices = read_unpack<u8>(lod_0[i], VifVnVl::V4_8);
	verify(tfrag.memory_map.indices_addr == lod_0[i].code.unpack.addr, "Weird tfrag.");
	i++;
	if (i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V4_8 && tfrag.common_vu_header.positions_lod_0_count > 0) {
		tfrag.lod_0_parent_indices = read_unpack<u8>(lod_0[i], VifVnVl::V4_8);
		tfrag.memory_map.parent_indices_lod_0_addr = lod_0[i].code.unpack.addr;
		s32 size_difference = (s32) tfrag.lod_0_parent_indices.size() - (s32) tfrag.common_vu_header.positions_lod_0_count;
		verify(size_difference >= 0 && size_difference < 4, "Parent indices array has bad size.");
		tfrag.lod_0_parent_indices.resize(tfrag.common_vu_header.positions_lod_0_count);
		i++;
	}
	if (i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V4_8) {
		tfrag.lod_0_unknown_indices_2 = read_unpack<u8>(lod_0[i], VifVnVl::V4_8);
		tfrag.memory_map.unk_indices_2_lod_0_addr = lod_0[i].code.unpack.addr;
		i++;
	}
	if (i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V4_16) {
		tfrag.lod_0_vertex_info = read_unpack<TfragVertexInfo>(lod_0[i], VifVnVl::V4_16);
		tfrag.memory_map.vertex_info_lod_0_addr = lod_0[i].code.unpack.addr;
		i++;
	}
}

static void write_unpack(OutBuffer dest, Buffer data, VifVnVl vnvl, VifUsn usn, s32 addr)
{
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

static void write_tfrag_command_lists(
	OutBuffer dest, TfragHeader& header, const Tfrag& tfrag, s64 tfrag_ofs, Game game)
{
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
	Buffer vu_header((u8*) &tfrag.common_vu_header, (u8*) (&tfrag.common_vu_header + 1));
	write_unpack(dest, vu_header, VifVnVl::V4_16, VifUsn::UNSIGNED, tfrag.memory_map.header_common_addr);
	header.tex_ofs = checked_int_cast<u16>(dest.tell() + 4 - tfrag_ofs);
	write_unpack(dest, tfrag.common_textures, VifVnVl::V4_32, VifUsn::SIGNED, tfrag.memory_map.ad_gifs_common_addr);
	write_strow(dest, single_vertex_info_strow);
	dest.write<u32>(0x05000001); // stmod
	write_unpack(dest, tfrag.common_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED, tfrag.memory_map.vertex_info_common_addr);
	write_strow(dest, tfrag.base_position);
	dest.write<u32>(0x01000102); // stcycl
	if (game == Game::DL) {
		header.light_vert_start_ofs_dl = checked_int_cast<u16>(dest.tell() + 4 - tfrag_ofs);
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
	if (!tfrag.lod_01_parent_indices.empty() || !tfrag.lod_01_unknown_indices_2.empty()) {
		write_strow(dest, indices_strow);
	}
	bool lod_01_needs_stmod =
		!tfrag.lod_01_parent_indices.empty() ||
		!tfrag.lod_01_unknown_indices_2.empty() ||
		!tfrag.lod_01_vertex_info.empty() ||
		!tfrag.lod_01_positions.empty() ||
		!tfrag.lod_0_positions.empty();
	if (lod_01_needs_stmod) {
		dest.write<u32>(0x05000001); // stmod
	}
	if (!tfrag.lod_01_parent_indices.empty()) {
		write_unpack(dest, tfrag.lod_01_parent_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.parent_indices_lod_01_addr);
	}
	if (!tfrag.lod_01_unknown_indices_2.empty()) {
		write_unpack(dest, tfrag.lod_01_unknown_indices_2, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.unk_indices_2_lod_01_addr);
	}
	if (!tfrag.lod_01_vertex_info.empty()) {
		write_strow(dest, double_vertex_info_strow);
	}
	if (!tfrag.lod_01_vertex_info.empty()) {
		write_unpack(dest, tfrag.lod_01_vertex_info, VifVnVl::V4_16, VifUsn::SIGNED, tfrag.memory_map.vertex_info_lod_01_addr);
	}
	write_strow(dest, tfrag.base_position);
	dest.write<u32>(0x01000102); // stcycl
	if (!tfrag.lod_01_positions.empty()) {
		write_unpack(dest, tfrag.lod_01_positions, VifVnVl::V3_16, VifUsn::SIGNED, tfrag.memory_map.positions_lod_01_addr);
	}
	
	dest.pad(0x10);
	s64 lod_0_ofs = dest.tell();
	
	// LOD 0
	if (!tfrag.lod_0_positions.empty()) {
		write_unpack(dest, tfrag.lod_0_positions, VifVnVl::V3_16, VifUsn::SIGNED, tfrag.memory_map.positions_lod_0_addr);
	}
	dest.write<u32>(0x05000000); // stmod
	dest.write<u32>(0x01000404); // stcycl
	write_unpack(dest, tfrag.lod_0_strips, VifVnVl::V4_8, VifUsn::SIGNED, tfrag.memory_map.strips_addr);
	write_strow(dest, indices_strow);
	dest.write<u32>(0x05000001); // stmod
	write_unpack(dest, tfrag.lod_0_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.indices_addr);
	if (!tfrag.lod_0_parent_indices.empty()) {
		write_unpack(dest, tfrag.lod_0_parent_indices, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.parent_indices_lod_0_addr);
	}
	if (!tfrag.lod_0_unknown_indices_2.empty()) {
		write_unpack(dest, tfrag.lod_0_unknown_indices_2, VifVnVl::V4_8, VifUsn::UNSIGNED, tfrag.memory_map.unk_indices_2_lod_0_addr);
	}
	if (!tfrag.lod_0_vertex_info.empty()) {
		write_strow(dest, double_vertex_info_strow);
	}
	if (!tfrag.lod_0_vertex_info.empty()) {
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
}

static s32 pad_index_array(std::vector<u8>& indices)
{
	s32 old_size = (s32) indices.size();
	if (indices.size() % 2 != 0) {
		indices.emplace_back(0);
	}
	if (indices.size() % 4 != 0) {
		indices.emplace_back(0);
		indices.emplace_back(0);
	}
	return old_size;
}

void allocate_tfrags_vu(Tfrags& tfrags)
{
	static const s32 VU1_BUFFER_SIZE = 0x148;
	
	for (Tfrag& tfrag : tfrags.fragments) {
		// Clear old data (for testing).
		tfrag.memory_map = {};
		
		// Write counts into the VU header.
		tfrag.common_vu_header.positions_common_count = checked_int_cast<u16>(tfrag.common_positions.size());
		tfrag.common_vu_header.positions_lod_01_count = checked_int_cast<u16>(tfrag.lod_01_positions.size());
		tfrag.common_vu_header.positions_lod_0_count = checked_int_cast<u16>(tfrag.lod_0_positions.size());
		
		// Pad index arrays.
		pad_index_array(tfrag.lod_2_indices);
		pad_index_array(tfrag.lod_1_indices);
		pad_index_array(tfrag.lod_01_parent_indices);
		pad_index_array(tfrag.lod_01_unknown_indices_2);
		pad_index_array(tfrag.lod_0_indices);
		pad_index_array(tfrag.lod_0_parent_indices);
		pad_index_array(tfrag.lod_0_unknown_indices_2);
		
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
		s32 parent_indices_lod_01_size = align32(tfrag.lod_01_parent_indices.size(), 4) / 4;
		s32 unk_indices_2_lod_01_size = align32(tfrag.lod_01_unknown_indices_2.size(), 4) / 4;
		s32 parent_indices_lod_0_size = align32(tfrag.lod_0_parent_indices.size(), 4) / 4;
		s32 unk_indices_2_lod_0_size = align32(tfrag.lod_0_unknown_indices_2.size(), 4) / 4;
		s32 indices_size = align32(std::max({tfrag.lod_0_indices.size(), tfrag.lod_1_indices.size(), tfrag.lod_2_indices.size()}), 4) / 4;
		s32 strips_size = std::max({tfrag.lod_0_strips.size(), tfrag.lod_1_strips.size(), tfrag.lod_2_strips.size()});
		
		// Calculate addresses in VU memory.
		tfrag.memory_map.header_common_addr = 0;
		tfrag.memory_map.ad_gifs_common_addr = tfrag.memory_map.header_common_addr + header_common_size + matrix_size;
		tfrag.memory_map.positions_common_addr = tfrag.memory_map.ad_gifs_common_addr + ad_gifs_common_size;
		tfrag.memory_map.positions_lod_01_addr = tfrag.memory_map.positions_common_addr + positions_common_size;
		tfrag.memory_map.positions_lod_0_addr = tfrag.memory_map.positions_lod_01_addr + positions_lod_01_size;
		tfrag.memory_map.vertex_info_common_addr = tfrag.memory_map.positions_lod_0_addr + positions_lod_0_size + tfrag.positions_slack;
		tfrag.memory_map.vertex_info_lod_01_addr = tfrag.memory_map.vertex_info_common_addr + vertex_info_common_size;
		tfrag.memory_map.vertex_info_lod_0_addr = tfrag.memory_map.vertex_info_lod_01_addr + vertex_info_lod_01_size;
		tfrag.memory_map.parent_indices_lod_01_addr = tfrag.memory_map.vertex_info_lod_0_addr + vertex_info_lod_0_size;
		tfrag.memory_map.unk_indices_2_lod_01_addr = tfrag.memory_map.parent_indices_lod_01_addr + parent_indices_lod_01_size;
		tfrag.memory_map.parent_indices_lod_0_addr = tfrag.memory_map.unk_indices_2_lod_01_addr + unk_indices_2_lod_01_size;
		tfrag.memory_map.unk_indices_2_lod_0_addr = tfrag.memory_map.parent_indices_lod_0_addr + parent_indices_lod_0_size;
		tfrag.memory_map.indices_addr = tfrag.memory_map.unk_indices_2_lod_0_addr + unk_indices_2_lod_0_size;
		tfrag.memory_map.strips_addr = tfrag.memory_map.indices_addr + indices_size;
		
		// Write addresses into the VU header.
		tfrag.common_vu_header.positions_common_addr = checked_int_cast<u16>(tfrag.memory_map.positions_common_addr);
		tfrag.common_vu_header.vertex_info_common_addr = checked_int_cast<u16>(tfrag.memory_map.vertex_info_common_addr);
		tfrag.common_vu_header.vertex_info_lod_01_addr = checked_int_cast<u16>(tfrag.memory_map.vertex_info_lod_01_addr);
		tfrag.common_vu_header.vertex_info_lod_0_addr = checked_int_cast<u16>(tfrag.memory_map.vertex_info_lod_0_addr);
		tfrag.common_vu_header.indices_addr = checked_int_cast<u16>(tfrag.memory_map.indices_addr);
		tfrag.common_vu_header.parent_indices_lod_01_addr = checked_int_cast<u16>(tfrag.memory_map.parent_indices_lod_01_addr);
		tfrag.common_vu_header.unk_indices_2_lod_01_addr = checked_int_cast<u16>(tfrag.memory_map.unk_indices_2_lod_01_addr);
		tfrag.common_vu_header.parent_indices_lod_0_addr = checked_int_cast<u16>(tfrag.memory_map.parent_indices_lod_0_addr);
		tfrag.common_vu_header.unk_indices_2_lod_0_addr = checked_int_cast<u16>(tfrag.memory_map.unk_indices_2_lod_0_addr);
		tfrag.common_vu_header.strips_addr = checked_int_cast<u16>(tfrag.memory_map.strips_addr);
		tfrag.common_vu_header.texture_ad_gifs_addr = checked_int_cast<u16>(tfrag.memory_map.ad_gifs_common_addr);
		
		// For testing purposes.
		if (positions_lod_01_size == 0) tfrag.memory_map.positions_lod_01_addr = -1;
		if (positions_lod_0_size == 0) tfrag.memory_map.positions_lod_0_addr = -1;
		if (vertex_info_lod_01_size == 0) tfrag.memory_map.vertex_info_lod_01_addr = -1;
		if (vertex_info_lod_0_size == 0) tfrag.memory_map.vertex_info_lod_0_addr = -1;
		if (parent_indices_lod_01_size == 0) tfrag.memory_map.parent_indices_lod_01_addr = -1;
		if (parent_indices_lod_0_size == 0) tfrag.memory_map.parent_indices_lod_0_addr = -1;
		
		s32 end_addr = tfrag.memory_map.strips_addr + strips_size;
		verify_fatal(end_addr <= VU1_BUFFER_SIZE);
	}
}
