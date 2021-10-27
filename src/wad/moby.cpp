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

#include "moby.h"

static std::vector<MobySequence> read_moby_sequences(Buffer src, s64 sequence_count);
static void write_moby_sequences(OutBuffer dest, const std::vector<MobySequence>& sequences, s64 list_ofs);
static MobyCollision read_moby_collision(Buffer src);
static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision);
static std::vector<std::vector<u8>> read_moby_joints(Buffer src, s64 joints_ofs);
static s64 write_moby_joints(OutBuffer dest, const std::vector<std::vector<u8>>& joints);
static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count);
using GifUsageTable = std::vector<MobyGifUsageTableEntry>;
static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes);
static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count);
static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes);
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const std::vector<u8>& ind, const std::vector<GsAdData>& texs, u8 ihdr0, u8 secret_index);
static Mesh lift_moby_mesh(const std::vector<MobySubMesh>& submeshes, s32 o_class);

// FIXME: Figure out what points to the mystery data instead of doing this.
static s64 mystery_data_ofs;

MobyClassData read_moby_class(Buffer src) {
	auto header = src.read<MobyClassHeader>(0, "moby class header");
	MobyClassData moby;
	moby.byte_4 = src.read<u32>(4, "moby class header");
	moby.unknown_9 = header.unknown_9;
	moby.lod_trans = header.lod_trans;
	moby.shadow = header.shadow;
	moby.scale = header.scale;
	moby.mip_dist = header.mip_dist;
	moby.corncob = header.corncob;
	moby.bounding_sphere = header.bounding_sphere.unpack();
	moby.glow_rgba = header.glow_rgba;
	moby.mode_bits = header.mode_bits;
	moby.type = header.type;
	moby.mode_bits2 = header.mode_bits2;
	moby.first_sequence_offset = src.read<s32>(0x48, "moby sequences");
	mystery_data_ofs = moby.first_sequence_offset;
	moby.sequences = read_moby_sequences(src, header.sequence_count);
	s32 bangles_submesh_count = 0;
	if(header.bangles != 0) {
		auto bangles_header = src.read<MobyBanglesHeader>(header.bangles * 0x10, "moby bangles header");
		moby.bangles.unknown_0 = bangles_header.unknown_0;
		moby.bangles.unknown_2 = bangles_header.unknown_2;
		moby.bangles.unknown_3 = bangles_header.unknown_3;
		bangles_submesh_count = bangles_header.submesh_count;
		
		moby.bangles.data = src.read_bytes(header.bangles * 0x10, moby.first_sequence_offset - header.bangles * 0x10, "moby bangles data");
		moby.bangles_offset = header.bangles * 0x10;
	}
	verify(header.sequence_count >= 1, "Moby class has no sequences.");
	if(header.collision != 0) {
		moby.collision = read_moby_collision(src.subbuf(header.collision));
		s64 coll_size = 0x10 + moby.collision->first_part.size() + moby.collision->second_part.size() * 8 + moby.collision->third_part.size();
		mystery_data_ofs = std::max(mystery_data_ofs, header.collision + coll_size);
	}
	for(const Mat4& matrix : src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton")) {
		moby.skeleton.push_back(matrix.unpack());
	}
	moby.common_trans = src.read_bytes(header.common_trans, header.joint_count * 0x10, "moby common trans");
	moby.joints = read_moby_joints(src, header.joints);
	moby.sound_defs = src.read_multiple<MobySoundDef>(header.sound_defs, header.sound_count, "moby sound defs").copy();
	if(header.submesh_table_offset != 0) {
		moby.submeshes = read_moby_submeshes(src, header.submesh_table_offset, header.submesh_count);
		moby.low_detail_submeshes = read_moby_submeshes(src, header.submesh_table_offset + header.submesh_count * 0x10, header.low_detail_submesh_count);
		moby.metal_submeshes = read_moby_metal_submeshes(src, header.submesh_table_offset + header.metal_submesh_begin * 0x10, header.metal_submesh_count);
		s64 bangles_submesh_table_ofs = header.submesh_table_offset + (header.metal_submesh_begin + header.metal_submesh_count) * 0x10;
		moby.bangles.submeshes = read_moby_submeshes(src, bangles_submesh_table_ofs, bangles_submesh_count);
		mystery_data_ofs = std::max(mystery_data_ofs, bangles_submesh_table_ofs + bangles_submesh_count * 0x10);
	}
	moby.mystery_data = src.read_bytes(mystery_data_ofs, header.skeleton - mystery_data_ofs, "moby mystery data");
	return moby;
}

static s64 class_header_ofs;

void write_moby_class(OutBuffer dest, const MobyClassData& moby) {
	MobyClassHeader header = {0};
	class_header_ofs = dest.alloc<MobyClassHeader>();
	assert(class_header_ofs % 0x40 == 0);
	
	verify(moby.submeshes.size() < 256, "Moby class has too many submeshes.");
	header.submesh_count = moby.submeshes.size();
	verify(moby.low_detail_submeshes.size() < 256, "Moby class has too many low detail submeshes.");
	header.low_detail_submesh_count = moby.low_detail_submeshes.size();
	verify(moby.metal_submeshes.size() < 256, "Moby class has too many metal submeshes.");
	header.metal_submesh_count = moby.metal_submeshes.size();
	header.metal_submesh_begin = moby.submeshes.size() + moby.low_detail_submeshes.size();
	header.unknown_9 = moby.unknown_9;
	header.lod_trans = moby.lod_trans;
	header.shadow = moby.shadow;
	header.scale = moby.scale;
	verify(moby.sound_defs.size() < 256, "Moby class has too many sounds.");
	header.sound_count = moby.sound_defs.size();
	header.mip_dist = moby.mip_dist;
	header.corncob = moby.corncob;
	header.bounding_sphere = Vec4f::pack(moby.bounding_sphere);
	header.glow_rgba = moby.glow_rgba;
	header.mode_bits = moby.mode_bits;
	header.type = moby.type;
	header.mode_bits2 = moby.mode_bits2;
	
	verify(moby.sequences.size() < 256, "Moby class has too many sequences (max is 255).");
	header.sequence_count = moby.sequences.size();
	s64 sequence_list_ofs = dest.alloc_multiple<s32>(moby.sequences.size());
	dest.pad(0x10);
	if(moby.bangles.submeshes.size() > 0) {
		while(dest.tell() - class_header_ofs < moby.bangles_offset) {
			dest.write<u8>(0);
		}
		header.bangles = (dest.write_multiple(moby.bangles.data) - class_header_ofs) / 0x10;
	}
	while(dest.tell() - class_header_ofs < moby.first_sequence_offset) {
		dest.write<u8>(0);
	}
	write_moby_sequences(dest, moby.sequences, sequence_list_ofs);
	dest.pad(0x10);
	s64 submesh_table_1_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes.size());
	s64 submesh_table_2_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.low_detail_submeshes.size());
	s64 metal_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.metal_submeshes.size());
	s64 bangles_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.bangles.submeshes.size());
	if(moby.submeshes.size() > 0 || moby.low_detail_submeshes.size() > 0 || moby.metal_submeshes.size() > 0) {
		header.submesh_table_offset = submesh_table_1_ofs - class_header_ofs;
	}
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision) - class_header_ofs;
	}
	dest.write_multiple(moby.mystery_data);
	header.skeleton = dest.tell() - class_header_ofs;
	verify(moby.skeleton.size() < 255, "Moby class has too many joints.");
	header.joint_count = moby.skeleton.size();
	for(const glm::mat4& matrix : moby.skeleton) {
		dest.write(Mat4::pack(matrix));
	}
	dest.pad(0x10);
	header.common_trans = dest.write_multiple(moby.common_trans) - class_header_ofs;
	header.joints = write_moby_joints(dest, moby.joints) - class_header_ofs;
	dest.pad(0x10);
	if(moby.sound_defs.size() > 0) {
		header.sound_defs = dest.write_multiple(moby.sound_defs) - class_header_ofs;
	}
	if(moby.submeshes.size() > 0 || moby.low_detail_submeshes.size() > 0 || moby.metal_submeshes.size() > 0) {
		std::vector<MobyGifUsageTableEntry> gif_usage;
		write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, moby.submeshes);
		write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, moby.low_detail_submeshes);
		write_moby_metal_submeshes(dest, metal_submesh_table_ofs, moby.metal_submeshes);
		write_moby_submeshes(dest, gif_usage, bangles_submesh_table_ofs, moby.bangles.submeshes);
		assert(gif_usage.size() > 0);
		gif_usage.back().offset_and_terminator |= 0x80000000;
		header.gif_usage = dest.write_multiple(gif_usage) - class_header_ofs;
	}
	dest.write(class_header_ofs, header);
	dest.write(class_header_ofs + 4, moby.byte_4);
}

static std::vector<MobySequence> read_moby_sequences(Buffer src, s64 sequence_count) {
	std::vector<MobySequence> sequences;
	auto sequence_offsets = src.read_multiple<s32>(sizeof(MobyClassHeader), sequence_count, "moby sequences");
	for(s32 seq_offset : sequence_offsets) {
		auto seq_header = src.read<MobySequenceHeader>(seq_offset, "moby sequence header");
		MobySequence sequence;
		sequence.bounding_sphere = seq_header.bounding_sphere.unpack();
		sequence.animation_info = seq_header.animation_info;
		sequence.sound_count = seq_header.sound_count;
		
		auto frame_table = src.read_multiple<s32>(seq_offset + 0x1c, seq_header.frame_count, "moby sequence table");
		for(s32 frame_offset : frame_table) {
			auto frame_header = src.read<MobyFrameHeader>(frame_offset, "moby frame header");
			MobyFrame frame;
			frame.unknown_0 = frame_header.unknown_0;
			frame.unknown_4 = frame_header.unknown_4;
			frame.unknown_8 = frame_header.unknown_8;
			frame.unknown_c = frame_header.unknown_c;
			frame.unknown_d = frame_header.unknown_d;
			frame.data = src.read_bytes(frame_offset + 0x10, frame_header.count * 0x10, "frame data");
			sequence.frames.emplace_back(std::move(frame));
			
			mystery_data_ofs = std::max(mystery_data_ofs, (s64) frame_offset + 0x10 + frame_header.count * 0x10);
		}
		s64 trigger_list_ofs = seq_offset + 0x1c + seq_header.frame_count * 4;
		sequence.triggers = src.read_multiple<u32>(trigger_list_ofs, seq_header.trigger_count, "moby sequence trigger list").copy();
		if(seq_header.triggers != 0) {
			s64 abs_trigger_ofs = seq_offset + seq_header.triggers;
			sequence.trigger_data = src.read<MobyTriggerData>(abs_trigger_ofs, "moby sequence trigger data");
		}
		
		sequences.emplace_back(std::move(sequence));
	}
	return sequences;
}

static void write_moby_sequences(OutBuffer dest, const std::vector<MobySequence>& sequences, s64 list_ofs) {
	for(const MobySequence& sequence : sequences) {
		dest.pad(0x10);
		s64 seq_header_ofs = dest.alloc<MobySequenceHeader>();
		dest.write<s32>(list_ofs, seq_header_ofs - class_header_ofs);
		list_ofs += 4;
		
		MobySequenceHeader seq_header = {0};
		seq_header.bounding_sphere = Vec4f::pack(sequence.bounding_sphere);
		seq_header.frame_count = sequence.frames.size();
		seq_header.sound_count = sequence.sound_count;
		seq_header.trigger_count = sequence.triggers.size();
		seq_header.pad = -1;
		
		s64 frame_pointer_ofs = dest.alloc_multiple<s32>(sequence.frames.size());
		dest.write_multiple(sequence.triggers);
		if(sequence.trigger_data.has_value()) {
			seq_header.triggers = dest.write(*sequence.trigger_data) - seq_header_ofs;
		}
		seq_header.animation_info = sequence.animation_info;
		for(const MobyFrame& frame : sequence.frames) {
			MobyFrameHeader frame_header = {0};
			frame_header.unknown_0 = frame.unknown_0;
			frame_header.unknown_4 = frame.unknown_4;
			frame_header.count = frame.data.size() / 0x10;
			frame_header.unknown_8 = frame.unknown_8;
			frame_header.unknown_c = frame.unknown_c;
			frame_header.unknown_d = frame.unknown_d;
			dest.pad(0x10);
			dest.write<s32>(frame_pointer_ofs, dest.write(frame_header) - class_header_ofs);
			frame_pointer_ofs += 4;
			dest.write_multiple(frame.data);
		}
		dest.write(seq_header_ofs, seq_header);
	}
}

static MobyCollision read_moby_collision(Buffer src) {
	auto header = src.read<MobyCollisionHeader>(0, "moby collision header");
	MobyCollision collision;
	collision.unknown_0 = header.unknown_0;
	collision.unknown_2 = header.unknown_2;
	s64 ofs = 0x10;
	collision.first_part = src.read_bytes(ofs, header.first_part_size, "moby collision data");
	ofs += header.first_part_size;
	verify(header.second_part_size % 8 == 0, "Bad moby collision.");
	auto second_part = src.read_multiple<s16>(ofs, header.second_part_size / 2, "moby collision second part");
	ofs += header.second_part_size;
	for(s64 i = 0; i < second_part.size() / 4; i++) {
		Vec3f vec;
		vec.x = second_part[i * 4 + 0] / 1024.f;
		vec.y = second_part[i * 4 + 1] / 1024.f;
		vec.z = second_part[i * 4 + 2] / 1024.f;
		collision.second_part.push_back(vec);
	}
	collision.third_part = src.read_bytes(ofs, header.third_part_size, "moby collision third part");
	return collision;
}

static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision) {
	MobyCollisionHeader header;
	header.unknown_0 = collision.unknown_0;
	header.unknown_2 = collision.unknown_2;
	header.first_part_size = collision.first_part.size();
	header.third_part_size = collision.third_part.size();
	header.second_part_size = collision.second_part.size() * 8;
	dest.pad(0x10);
	s64 ofs = dest.write(header);
	dest.write_multiple(collision.first_part);
	for(const Vec3f& vec : collision.second_part) {
		dest.write<s16>(vec.x * 1024.f);
		dest.write<s16>(vec.y * 1024.f);
		dest.write<s16>(vec.z * 1024.f);
		dest.write<s16>(0);
	}
	dest.write_multiple(collision.third_part);
	return ofs;
}

static std::vector<std::vector<u8>> read_moby_joints(Buffer src, s64 joints_ofs) {
	std::vector<std::vector<u8>> lists;
	s32 list_count = src.read<s32>(joints_ofs, "joint list count");
	for(s32 i = 0; i < list_count; i++) {
		std::vector<u8> list;
		s32 list_ofs = src.read<s32>(joints_ofs + (i + 1) * 4, "joint list");
		for(;;) {
			u8 value = src.read<u8>(list_ofs, "joint list data");
			list_ofs++;
			if(value == 0xff) {
				break;
			}
			list.push_back(value);
		}
		lists.emplace_back(std::move(list));
	}
	return lists;
}
static s64 write_moby_joints(OutBuffer dest, const std::vector<std::vector<u8>>& joints) {
	dest.pad(0x10);
	s64 base_ofs = dest.tell();
	dest.write<s32>(joints.size());
	s64 outer_list_ofs = dest.alloc_multiple<s32>(joints.size());
	for(const std::vector<u8>& joint_list : joints) {
		dest.pad(0x4);
		dest.write<s32>(outer_list_ofs, dest.tell() - class_header_ofs);
		outer_list_ofs += 4;
		dest.write_multiple(joint_list);
		dest.write<u8>(0xff);
	}
	return base_ofs;
}

static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count) {
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
		submesh.index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		verify(submesh.index_header.unknown_0 == 0xfe, "Moby has bad index buffer.");
		verify(submesh.index_header.pad == 0, "Moby has bad index buffer.");
		submesh.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 3) {
			Buffer texture_buffer(unpacks.at(2).data);
			submesh.texture_data = texture_buffer.read_multiple<GsAdData>(0, texture_buffer.size() / 0x10, "moby texture data").copy();
		}
		
		// Read vertex table.
		auto vertex_header = src.read<MobyVertexTableHeader>(entry.vertex_offset, "moby vertex header");
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
		s64 array_ofs = entry.vertex_offset + 0x10;
		submesh.unknowns = src.read_multiple<u16>(array_ofs, vertex_header.unknown_count_0, "vertex table").copy();
		array_ofs += vertex_header.unknown_count_0 * 2;
		if(array_ofs % 4 != 0) {
			array_ofs += 2;
		}
		if(array_ofs % 8 != 0) {
			array_ofs += 4;
		}
		submesh.duplicate_vertices = src.read_multiple<u16>(array_ofs, vertex_header.duplicate_vertex_count, "vertex table").copy();
		s64 vertex_ofs = entry.vertex_offset + vertex_header.vertex_table_offset;
		s32 in_file_vertex_count = vertex_header.vertex_count_2 + vertex_header.vertex_count_4 + vertex_header.main_vertex_count;
		submesh.vertices = src.read_multiple<MobyVertex>(vertex_ofs, in_file_vertex_count, "vertex table").copy();
		vertex_ofs += in_file_vertex_count * 0x10;
		submesh.vertex_count_2 = vertex_header.vertex_count_2;
		submesh.vertex_count_4 = vertex_header.vertex_count_4;
		submesh.unknown_e = vertex_header.unknown_e;
		
		// Fix vertex indices (see comment in write_moby_submeshes).
		for(size_t i = 7; i < submesh.vertices.size(); i++) {
			submesh.vertices[i - 7].low_word = (submesh.vertices[i - 7].low_word & ~0x1ff) | (submesh.vertices[i].low_word & 0x1ff);
		}
		s32 trailing_vertex_count = entry.vertex_data_size - vertex_header.vertex_table_offset / 0x10 - in_file_vertex_count;
		verify(trailing_vertex_count < 7, "error: Bad moby vertex table.");
		s64 trailing;
		vertex_ofs += std::max(7 - in_file_vertex_count, 0) * 0x10;
		for(s64 i = std::max(7 - in_file_vertex_count, 0); i < trailing_vertex_count; i++) {
			MobyVertex vertex = src.read<MobyVertex>(vertex_ofs, "vertex table");
			vertex_ofs += 0x10;
			s64 dest_index = in_file_vertex_count + i - 7;
			submesh.vertices.at(dest_index).low_word = (submesh.vertices[dest_index].low_word & ~0x1ff) | (vertex.low_word & 0x1ff);
		}
		MobyVertex last_vertex = src.read<MobyVertex>(vertex_ofs - 0x10, "vertex table");
		for(s32 i = std::max(7 - in_file_vertex_count - trailing_vertex_count, 0); i < 6; i++) {
			s64 dest_index = in_file_vertex_count + trailing_vertex_count + i - 7;
			if(dest_index < submesh.vertices.size()) {
				submesh.vertices[dest_index].low_word = (submesh.vertices[dest_index].low_word & ~0x1ff) | (last_vertex.trailing_vertex_indices[i] & 0x1ff);
			}
		}
		
		submeshes.emplace_back(std::move(submesh));
	}
	return submeshes;
}

static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes) {
	static const s32 ST_UNPACK_ADDR_QUADWORDS = 0xc2;
	
	for(const MobySubMesh& submesh : submeshes) {
		// Write VIF command list.
		MobySubMeshEntry entry = {0};
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
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh.indices, submesh.texture_data, submesh.index_header.unknown_0, submesh.index_header.secret_index);
		
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Umm.. "adjust" vertex indices (see comment below).
		std::vector<MobyVertex> vertices = submesh.vertices;
		std::vector<u16> trailing_vertex_indices(std::max(7 - (s32) vertices.size(), 0), 0);
		for(s32 i = std::max((s32) vertices.size() - 7, 0); i < vertices.size(); i++) {
			trailing_vertex_indices.push_back(vertices[i].low_word & 0x1ff);
		}
		for(s32 i = vertices.size() - 1; i >= 7; i--) {
			vertices[i].low_word = (vertices[i].low_word & ~0x1ff) | (vertices[i - 7].low_word & 0xff);
		}
		for(s32 i = 0; i < std::min(7, (s32) vertices.size()); i++) {
			vertices[i].low_word = vertices[i].low_word & ~0x1ff;
		}
		
		// Write vertex table.
		s64 vertex_header_ofs = dest.alloc<MobyVertexTableHeader>();
		MobyVertexTableHeader vertex_header;
		vertex_header.unknown_count_0 = submesh.unknowns.size();
		vertex_header.vertex_count_2 = submesh.vertex_count_2;
		vertex_header.vertex_count_4 = submesh.vertex_count_4;
		vertex_header.main_vertex_count =
			submesh.vertices.size() -
			submesh.vertex_count_2 -
			submesh.vertex_count_4;
		vertex_header.duplicate_vertex_count = submesh.duplicate_vertices.size();
		vertex_header.transfer_vertex_count =
			vertex_header.vertex_count_2 +
			vertex_header.vertex_count_4 +
			vertex_header.main_vertex_count +
			vertex_header.duplicate_vertex_count;
		vertex_header.unknown_e = submesh.unknown_e;
		dest.write_multiple(submesh.unknowns);
		dest.pad(0x8);
		dest.write_multiple(submesh.duplicate_vertices);
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
				vertex.low_word = trailing_vertex_indices[trailing];
			}
			vertices.push_back(vertex);
		}
		assert(trailing < trailing_vertex_indices.size());
		MobyVertex last_vertex = {0};
		if(submesh.vertices.size() + trailing >= 7) {
			last_vertex.low_word = trailing_vertex_indices[trailing];
		}
		for(s32 i = trailing + 1; i < trailing_vertex_indices.size(); i++) {
			if(submesh.vertices.size() + i >= 7) {
				last_vertex.trailing_vertex_indices[i - trailing - 1] = trailing_vertex_indices[i];
			}
		}
		vertices.push_back(last_vertex);
		dest.write_multiple(vertices);
		
		dest.write(vertex_header_ofs, vertex_header);
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

static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count) {
	std::vector<MobyMetalSubMesh> submeshes;
	for(auto& entry : src.read_multiple<MobySubMeshEntry>(table_ofs, count, "moby metal submesh table")) {
		MobyMetalSubMesh submesh;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer index_data(unpacks.at(0).data);
		submesh.index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		submesh.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 2) {
			Buffer texture_buffer(unpacks.at(1).data);
			submesh.texture_data = texture_buffer.read_multiple<GsAdData>(0, texture_buffer.size() / 0x10, "moby texture data").copy();
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

static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes) {
	for(const MobyMetalSubMesh& submesh : submeshes) {
		// Write VIF command list.
		MobySubMeshEntry entry = {0};
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh.indices, submesh.texture_data, submesh.index_header.unknown_0, submesh.index_header.secret_index);
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

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const std::vector<u8>& ind, const std::vector<GsAdData>& texs, u8 ihdr0, u8 secret_index) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	std::vector<u8> indices;
	OutBuffer index_buffer(indices);
	s64 index_header_ofs = index_buffer.alloc<MobyIndexHeader>();
	index_buffer.write_multiple(ind);
	
	MobyIndexHeader index_header = {0};
	index_header.unknown_0 = ihdr0;
	if(texs.size() > 0) {
		index_header.texture_unpack_offset_quadwords = indices.size() / 4;
	}
	index_header.secret_index = secret_index;
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
	if(texs.size() > 0) {
		while(dest.tell() % 0x10 != 0xc) {
			dest.write<u8>(0);
		}
		
		VifPacket texture_unpack;
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
		texture_unpack.code.num = texs.size();
		texture_unpack.code.unpack.vnvl = VifVnVl::V4_32;
		texture_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = VifUsn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + index_unpack.code.num;
		OutBuffer(texture_unpack.data).write_multiple(texs);
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsageTableEntry gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			assert(texs.size() % 4 == 0);
			for(size_t i = 0; i < texs.size(); i += 4) {
				assert(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = texs[i + 2].data_lo;
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

ColladaScene lift_moby_model(const MobyClassData& moby, s32 o_class) {
	ColladaScene scene;
	
	ColladaNode mesh_node;
	mesh_node.name = "mesh";
	mesh_node.mesh = scene.meshes.size();
	scene.nodes.emplace_back(std::move(mesh_node));
	scene.meshes.emplace_back(lift_moby_mesh(moby.submeshes, o_class));
	
	ColladaNode low_detail_node;
	low_detail_node.name = "low_detail";
	low_detail_node.mesh = scene.meshes.size();
	scene.nodes.emplace_back(std::move(low_detail_node));
	scene.meshes.emplace_back(lift_moby_mesh(moby.low_detail_submeshes, o_class));
	return scene;
}

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, submesh %d has bad " message ".", o_class, i);

static Mesh lift_moby_mesh(const std::vector<MobySubMesh>& submeshes, s32 o_class) {
	Mesh mesh;
	mesh.flags |= MESH_HAS_TEX_COORDS;
	// The game stores this on the end of the VU chain.
	Opt<MobyVertex> intermediate_buffer[512];
	memset(intermediate_buffer, 0, sizeof(intermediate_buffer));
	SubMesh dest;
	for(s32 i = 0; i < submeshes.size(); i++) {
		const MobySubMesh src = submeshes[i];
		s32 vertex_base = mesh.vertices.size();
		for(const MobyVertex& mv : src.vertices) {
			auto& st = src.sts.at(mesh.vertices.size() - vertex_base);
			auto pos = glm::vec3(mv.regular.x / 16384.f, mv.regular.y / 16384.f, mv.regular.z / 16384.f);
			auto tex_coord = glm::vec2(st.s / (INT16_MAX / 8.f), -st.t / (INT16_MAX / 8.f));
			while(tex_coord.s < 0) tex_coord.s += 1;
			while(tex_coord.t < 0) tex_coord.t += 1;
			mesh.vertices.emplace_back(pos, tex_coord);
			
			intermediate_buffer[mv.low_word & 0x1ff] = mv;
		}
		for(u16 v8 : src.duplicate_vertices) {
			VERIFY_SUBMESH((v8 & 0b1111111) == 0, "vertex table");
			auto mv = intermediate_buffer[v8 >> 7];
			VERIFY_SUBMESH(mv.has_value(), "vertex table");
			
			auto& st = src.sts.at(mesh.vertices.size() - vertex_base);
			auto pos = glm::vec3(mv->regular.x / 16384.f, mv->regular.y / 16384.f, mv->regular.z / 16384.f);
			auto tex_coord = glm::vec2(st.s / (INT16_MAX / 8.f), -st.t / (INT16_MAX / 8.f));
			while(tex_coord.s < 0) tex_coord.s += 1;
			while(tex_coord.t < 0) tex_coord.t += 1;
			mesh.vertices.emplace_back(pos, tex_coord);
		}
		s32 index_queue[3] = {0};
		s32 index_pos = 0;
		s32 max_index = 0;
		bool reverse_winding_order = true;
		// There's an extra index stored in the index header, in addition to an
		// index stored in each 0x40 byte texture unpack block. When a texture
		// is applied, the next index from this list is used as the next vertex
		// in the queue, but the triangle with it as its last index is not
		// actually drawn.
		u32 extra_index = src.index_header.secret_index;
		s32 texture_index = 0;
		for(u8 index : src.indices) {
			VERIFY_SUBMESH(index != 0x80, "index buffer");
			if(index == 0) {
				if(extra_index == 0) {
					VERIFY_SUBMESH(dest.faces.size() >= 3, "index buffer");
					// The VU1 microprogram has multiple vertices in flight at
					// a time, so we need to remove the ones that wouldn't have
					// been written to the GS packet.
					dest.faces.pop_back();
					dest.faces.pop_back();
					dest.faces.pop_back();
					break;
				} else {
					index = extra_index + 0x80;
					extra_index = src.texture_data.at(texture_index).super_secret_index;
					
					if(dest.faces.size() > 0) {
						mesh.submeshes.emplace_back(std::move(dest));
					}
					dest = SubMesh();
					dest.texture = src.texture_data.at(texture_index * 4 + 2).data_lo;
					
					texture_index++;
				}
			}
			if(index < 0x80) {
				VERIFY_SUBMESH(vertex_base + index - 1 < mesh.vertices.size(), "index buffer");
				index_queue[index_pos] = vertex_base + index - 1;
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
