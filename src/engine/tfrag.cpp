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

#include <core/vif.h>

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
		
		tfrag.lod_2_1 = read_unpack<u8>(lod_2[0], VifVnVl::V4_8);
		tfrag.lod_2_2 = read_unpack<s8>(lod_2[1], VifVnVl::V4_8);
		
		// Shared
		Buffer shared_buffer = data.subbuf(header.shared_ofs, header.lod_1_ofs - header.shared_ofs);
		std::vector<VifPacket> shared_command_list = read_vif_command_list(shared_buffer);
		std::vector<VifPacket> shared = filter_vif_unpacks(shared_command_list);
		verify(shared.size() == 4, "Incorrect number of shared VIF unpacks!");
		
		tfrag.shared_1 = read_unpack<u16>(shared[0], VifVnVl::V4_16);
		tfrag.shared_textures = read_unpack<TfragTexturePrimitive>(shared[1], VifVnVl::V4_32);
		tfrag.shared_3 = read_unpack<s16>(shared[2], VifVnVl::V4_16);
		tfrag.shared_4 = read_unpack<s16>(shared[3], VifVnVl::V3_16);
		
		// LOD 1
		Buffer lod_1_buffer = data.subbuf(header.lod_1_ofs, header.lod_0_ofs - header.lod_1_ofs);
		std::vector<VifPacket> lod_1_command_list = read_vif_command_list(lod_1_buffer);
		std::vector<VifPacket> lod_1 = filter_vif_unpacks(lod_1_command_list);
		verify(lod_1.size() == 2, "Incorrect number of LOD 1 VIF unpacks!");
		
		tfrag.lod_1_1 = read_unpack<s8>(lod_1[0], VifVnVl::V4_8);
		tfrag.lod_1_2 = read_unpack<u8>(lod_1[1], VifVnVl::V4_8);
		
		// LOD 01
		Buffer lod_01_buffer = data.subbuf(
			header.lod_0_ofs,
			header.shared_ofs + header.lod_1_size * 0x10 - header.lod_0_ofs);
		std::vector<VifPacket> lod_01_command_list = read_vif_command_list(lod_01_buffer);
		std::vector<VifPacket> lod_01 = filter_vif_unpacks(lod_01_command_list);
		verify(lod_01.size() == 3, "Incorrect number of LOD 01 VIF unpacks!");
		
		tfrag.lod_01_1 = read_unpack<u8>(lod_01[0], VifVnVl::V4_8);
		tfrag.lod_01_2 = read_unpack<s16>(lod_01[1], VifVnVl::V4_16);
		tfrag.lod_01_3 = read_unpack<s16>(lod_01[2], VifVnVl::V3_16);
		
		// LOD 0
		Buffer lod_0_buffer = data.subbuf(
			header.shared_ofs + header.lod_1_size * 0x10,
			header.rgba_ofs - (header.lod_1_size + header.lod_2_size - header.common_size) * 0x10);
		std::vector<VifPacket> lod_0_command_list = read_vif_command_list(lod_0_buffer);
		std::vector<VifPacket> lod_0 = filter_vif_unpacks(lod_0_command_list);
		verify(lod_0.size() >= 4, "Too few LOD 0 VIF unpacks!");
		
		tfrag.lod_0_1 = read_unpack<s16>(lod_0[0], VifVnVl::V3_16);
		tfrag.lod_0_2 = read_unpack<s8>(lod_0[1], VifVnVl::V4_8);
		s64 i;
		for(i = 2; i < lod_0.size() && lod_0[i].code.unpack.vnvl == VifVnVl::V4_8; i++) {
			tfrag.lod_0_3.emplace_back(read_unpack<s8>(lod_0[i], VifVnVl::V4_8));
		}
		verify(i < lod_0.size(), "Bad LOD 0 VIF unpacks!");
		tfrag.lod_0_4 = read_unpack<s16>(lod_0[i], VifVnVl::V4_16);
		
		tfrag.rgbas = data.read_multiple<TfragRgba>(header.rgba_ofs, header.rgba_size * 4, "rgbas").copy();
		tfrag.light = data.read_multiple<u8>(header.light_ofs, header.light_vert_start_ofs - header.light_ofs, "light").copy();
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

static void write_strow(OutBuffer dest, u32 vif_r0, u32 vif_r1, u32 vif_r2, u32 vif_r3) {
	dest.write<u32>(0x30000000);
	dest.write<u32>(vif_r0);
	dest.write<u32>(vif_r1);
	dest.write<u32>(vif_r2);
	dest.write<u32>(vif_r3);
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
		
		// LOD 2
		write_strow(dest, 0x000000a8, 0x000000a8, 0x000000a8, 0x000000a8);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_2_1, VifVnVl::V4_8, VifUsn::UNSIGNED);
		dest.write<u32>(0x05000000); // stmod
		write_unpack(dest, tfrag.lod_2_2, VifVnVl::V4_8, VifUsn::SIGNED);
		
		dest.pad(0x10, 0);
		s64 shared_ofs = dest.tell();
		header.shared_ofs = checked_int_cast<u16>(shared_ofs - tfrag_ofs);
		
		// shared
		write_unpack(dest, tfrag.shared_1, VifVnVl::V4_16, VifUsn::UNSIGNED);
		write_unpack(dest, tfrag.shared_textures, VifVnVl::V4_32, VifUsn::SIGNED);
		write_strow(dest, 0x45000000, 0x45000000, 0x00000000, 0x00000018);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.shared_3, VifVnVl::V4_16, VifUsn::SIGNED);
		write_strow(dest, 0x000252d5, 0x0001791e, 0x000094b5, 0x00000000);
		dest.write<u32>(0x01000102); // stcycl
		write_unpack(dest, tfrag.shared_4, VifVnVl::V3_16, VifUsn::SIGNED);
		dest.write<u32>(0x01000404); // stcycl
		dest.write<u32>(0x05000000); // stmod
		
		dest.pad(0x10, 0);
		s64 lod_1_ofs = dest.tell();
		header.lod_1_ofs = checked_int_cast<u16>(lod_1_ofs - tfrag_ofs);
		
		// LOD 1
		write_unpack(dest, tfrag.lod_1_1, VifVnVl::V4_8, VifUsn::SIGNED);
		write_strow(dest, 0x000000a8, 0x000000a8, 0x000000a8, 0x000000a8);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_1_2, VifVnVl::V4_8, VifUsn::UNSIGNED);
		
		dest.pad(0x10, 0);
		s64 lod_01_ofs = dest.tell();
		header.lod_0_ofs = checked_int_cast<u16>(lod_01_ofs - tfrag_ofs);
		
		// LOD 01
		write_strow(dest, 0x000000a8, 0x000000a8, 0x000000a8, 0x000000a8);
		dest.write<u32>(0x05000001); // stmod
		write_unpack(dest, tfrag.lod_01_1, VifVnVl::V4_8, VifUsn::SIGNED);
		write_strow(dest, 0x45000000, 0x45000000, 0x00000018, 0x00000018);
		write_unpack(dest, tfrag.lod_01_2, VifVnVl::V4_16, VifUsn::SIGNED);
		write_strow(dest, 0x000252d5, 0x0001791e, 0x000094b5, 0x00000000);
		dest.write<u32>(0x01000102); // stcycl
		write_unpack(dest, tfrag.lod_01_3, VifVnVl::V3_16, VifUsn::SIGNED);
		
		dest.pad(0x10);
		s64 lod_0_ofs = dest.tell();
		
		// LOD 0
		write_unpack(dest, tfrag.lod_0_1, VifVnVl::V3_16, VifUsn::SIGNED);
		dest.write<u32>(0x05000000); // stmod
		dest.write<u32>(0x01000404); // stcycl
		write_unpack(dest, tfrag.lod_0_2, VifVnVl::V4_8, VifUsn::SIGNED);
		write_strow(dest, 0x000000a8, 0x000000a8, 0x000000a8, 0x000000a8);
		dest.write<u32>(0x05000001); // stmod
		for(const std::vector<s8>& data : tfrag.lod_0_3) {
			write_unpack(dest, data, VifVnVl::V4_8, VifUsn::UNSIGNED);
		}
		write_strow(dest, 0x45000000, 0x45000000, 0x00000018, 0x00000018);
		write_unpack(dest, tfrag.lod_0_4, VifVnVl::V4_16, VifUsn::SIGNED);
		dest.write<u32>(0x005000000); // stmod
		
		dest.pad(0x10, 0);
		s64 end_ofs = dest.tell();
		header.lod_0_size = checked_int_cast<u8>((end_ofs - lod_0_ofs) / 0x10);
		
		// Fill in VIF command list sizes.
		header.common_size = checked_int_cast<u8>((lod_1_ofs - shared_ofs) / 0x10);
		header.lod_2_size = checked_int_cast<u8>((lod_1_ofs - tfrag_ofs) / 0x10);
		header.lod_1_size = checked_int_cast<u8>((lod_0_ofs - shared_ofs) / 0x10);
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
