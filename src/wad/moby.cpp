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
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const std::vector<s8>& ind, const std::vector<MobyTexturePrimitive>& texs, u8 ihdr0, u8 ihdr2);

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
		moby.collision = src.read_bytes(header.collision, header.skeleton - header.collision, "moby collision");
		mystery_data_ofs = 0xffffffff;
	}
	for(const Mat4& matrix : src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton")) {
		moby.skeleton.push_back(matrix.unpack());
	}
	moby.common_trans = src.read_bytes(header.common_trans, header.joint_count * 0x10, "moby common trans");
	moby.joints = read_moby_joints(src, header.joints);
	moby.sound_defs = src.read_multiple<MobySoundDef>(header.sound_defs, header.sound_count, "moby sound defs").copy();
	if(header.submesh_table_offset != 0) {
		moby.submeshes_1 = read_moby_submeshes(src, header.submesh_table_offset, header.submesh_count_1);
		moby.submeshes_2 = read_moby_submeshes(src, header.submesh_table_offset + header.submesh_count_1 * 0x10, header.submesh_count_2);
		moby.metal_submeshes = read_moby_metal_submeshes(src, header.submesh_table_offset + header.metal_submesh_begin * 0x10, header.metal_submesh_count);
		s64 bangles_submesh_table_ofs = header.submesh_table_offset + (header.metal_submesh_begin + header.metal_submesh_count) * 0x10;
		moby.bangles.submeshes = read_moby_submeshes(src, bangles_submesh_table_ofs, bangles_submesh_count);
		mystery_data_ofs = std::max(mystery_data_ofs, bangles_submesh_table_ofs + bangles_submesh_count * 0x10);
	}
	if(mystery_data_ofs != 0xffffffff) {
		moby.mystery_data = src.read_bytes(mystery_data_ofs, header.skeleton - mystery_data_ofs, "moby mystery data");
	}
	return moby;
}

static s64 class_header_ofs;

void write_moby_class(OutBuffer dest, const MobyClassData& moby) {
	MobyClassHeader header = {0};
	class_header_ofs = dest.alloc<MobyClassHeader>();
	assert(class_header_ofs % 0x40 == 0);
	
	verify(moby.submeshes_1.size() < 256, "Moby class has too many submeshes.");
	header.submesh_count_1 = moby.submeshes_1.size();
	verify(moby.submeshes_2.size() < 256, "Moby class has too many submeshes.");
	header.submesh_count_2 = moby.submeshes_2.size();
	verify(moby.metal_submeshes.size() < 256, "Moby class has too many metal submeshes.");
	header.metal_submesh_count = moby.metal_submeshes.size();
	header.metal_submesh_begin = moby.submeshes_1.size() + moby.submeshes_2.size();
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
	s64 submesh_table_1_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes_1.size());
	s64 submesh_table_2_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes_2.size());
	s64 metal_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.metal_submeshes.size());
	s64 bangles_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.bangles.submeshes.size());
	if(moby.submeshes_1.size() > 0 || moby.submeshes_2.size() > 0 || moby.metal_submeshes.size() > 0) {
		header.submesh_table_offset = submesh_table_1_ofs - class_header_ofs;
	}
	if(moby.collision.size() > 0) {
		header.collision = dest.write_multiple(moby.collision) - class_header_ofs;
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
	if(moby.submeshes_1.size() > 0 || moby.submeshes_2.size() > 0 || moby.metal_submeshes.size() > 0) {
		std::vector<MobyGifUsageTableEntry> gif_usage;
		write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, moby.submeshes_1);
		write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, moby.submeshes_2);
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
	s64 ofs = 0x10;
	if(header.first_part_count != 0) {
		collision.first_part = src.read_bytes(ofs, 0x50, "moby collision data");
		ofs += 0x50;
	}
	collision.second_part = src.read_bytes(ofs, header.second_part_size, "moby collision second part");
	ofs += header.second_part_size;
	collision.third_part = src.read_bytes(ofs, header.third_part_size, "moby collision third part");
	return collision;
}

static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision) {
	MobyCollisionHeader header;
	header.unknown_0 = collision.unknown_0;
	header.unknown_2 = collision.unknown_2;
	header.first_part_count = 0x20;
	header.third_part_size = collision.third_part.size();
	header.second_part_size = collision.second_part.size();
	dest.pad(0x10);
	s64 ofs = dest.write(header);
	dest.write_multiple(collision.first_part);
	dest.write_multiple(collision.second_part);
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
		submesh.indices = index_data.read_multiple<s8>(4, index_data.size() - 4, "moby index unpack data").copy();
		if(unpacks.size() >= 3) {
			Buffer texture_data(unpacks.at(2).data);
			for(s64 i = 0; i < texture_data.size(); i+= 0x40) {
				submesh.textures.push_back(texture_data.read<MobyTexturePrimitive>(i, "moby texture unpack"));
			}
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
		submesh.vertices_8 = src.read_multiple<u16>(array_ofs, vertex_header.vertex_count_8, "vertex table").copy();
		s64 vertices_ofs = entry.vertex_offset + vertex_header.vertex_table_offset;
		submesh.vertices_2 = src.read_multiple<MobyVertex>(vertices_ofs, vertex_header.vertex_count_2, "vertex table").copy();
		vertices_ofs += vertex_header.vertex_count_2 * 0x10;
		submesh.vertices_4 = src.read_multiple<MobyVertex>(vertices_ofs, vertex_header.vertex_count_4, "vertex table").copy();
		vertices_ofs += vertex_header.vertex_count_4 * 0x10;
		submesh.main_vertices = src.read_multiple<MobyVertex>(vertices_ofs, vertex_header.main_vertex_count, "vertex table").copy();
		vertices_ofs += vertex_header.main_vertex_count * 0x10;
		submesh.unknown_e = vertex_header.unknown_e;
		for(;;) {
			MobyVertex vertex = src.read<MobyVertex>(vertices_ofs, "vertex table");
			submesh.trailing_vertices.emplace_back(vertex);
			vertices_ofs += 0x10;
			if(vertex.unknown_4 != 0 || vertex.unknown_6 != 0) {
				break;
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
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh.indices, submesh.textures, submesh.index_header.unknown_0, submesh.index_header.unknown_2);
		
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Write vertex table.
		s64 vertex_header_ofs = dest.alloc<MobyVertexTableHeader>();
		MobyVertexTableHeader vertex_header;
		vertex_header.unknown_count_0 = submesh.unknowns.size();
		vertex_header.vertex_count_2 = submesh.vertices_2.size();
		vertex_header.vertex_count_4 = submesh.vertices_4.size();
		vertex_header.main_vertex_count = submesh.main_vertices.size();
		vertex_header.vertex_count_8 = submesh.vertices_8.size();
		vertex_header.transfer_vertex_count =
			vertex_header.vertex_count_2 +
			vertex_header.vertex_count_4 +
			vertex_header.main_vertex_count +
			vertex_header.vertex_count_8;
		vertex_header.unknown_e = submesh.unknown_e;
		dest.write_multiple(submesh.unknowns);
		dest.pad(0x8);
		dest.write_multiple(submesh.vertices_8);
		dest.pad(0x10);
		vertex_header.vertex_table_offset = dest.tell() - vertex_header_ofs;
		
		dest.write_multiple(submesh.vertices_2);
		dest.write_multiple(submesh.vertices_4);
		dest.write_multiple(submesh.main_vertices);
		dest.write_multiple(submesh.trailing_vertices);
		
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
		submesh.indices = index_data.read_multiple<s8>(4, index_data.size() - 4, "moby index unpack data").copy();
		if(unpacks.size() >= 2) {
			Buffer texture_data(unpacks.at(1).data);
			for(s64 i = 0; i < texture_data.size(); i+= 0x40) {
				submesh.textures.push_back(texture_data.read<MobyTexturePrimitive>(i, "moby texture unpack"));
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

static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes) {
	for(const MobyMetalSubMesh& submesh : submeshes) {
		// Write VIF command list.
		MobySubMeshEntry entry = {0};
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh.indices, submesh.textures, submesh.index_header.unknown_0, submesh.index_header.unknown_2);
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

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const std::vector<s8>& ind, const std::vector<MobyTexturePrimitive>& texs, u8 ihdr0, u8 ihdr2) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	MobyIndexHeader index_header = {0};
	index_header.unknown_0 = ihdr0;
	assert(ind.size() % 4 == 0);
	if(texs.size() > 0) {
		index_header.texture_unpack_offset_quadwords = ind.size() / 4 + 1;
	}
	index_header.unknown_2 = ihdr2;
	
	VifPacket index_unpack;
	index_unpack.code.interrupt = 0;
	index_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
	index_unpack.code.num = 1 + ind.size() / 4;
	index_unpack.code.unpack.vnvl = VifVnVl::V4_8;
	index_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	index_unpack.code.unpack.usn = VifUsn::SIGNED;
	index_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS;
	std::vector<u8> indices;
	OutBuffer index_buffer(indices);
	index_buffer.write(index_header);
	index_buffer.write_multiple(ind);
	index_unpack.data = indices;
	write_vif_packet(dest, index_unpack);
	
	s64 rel_texture_unpack_ofs = 0;
	if(texs.size() > 0) {
		while(dest.tell() % 0x10 != 0xc) {
			dest.write<u8>(0);
		}
		
		VifPacket texture_unpack;
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
		texture_unpack.code.num = texs.size() * 4;
		texture_unpack.code.unpack.vnvl = VifVnVl::V4_32;
		texture_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = VifUsn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + (4 + ind.size()) / 4;
		for(s32 i = 0 ; i < texs.size(); i++) {
			OutBuffer(texture_unpack.data).write(texs[i]);
		}
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsageTableEntry gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			for(const MobyTexturePrimitive& texture : texs) {
				assert(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = texture.d3_tex0.data_lo;
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

ColladaScene lift_moby_model(const MobyClassData& moby) {
	ColladaScene scene;
	
	ColladaNode node_1;
	node_1.name = "mesh_1";
	node_1.mesh = 0;
	scene.nodes.emplace_back(std::move(node_1));
	
	Mesh mesh_1;
	for(const MobySubMesh& submesh : moby.submeshes_1) {
		s32 vertex_base = mesh_1.vertices.size();
		for(const MobyVertex& mv : submesh.vertices_2) {
			Vertex v;
			v.pos = glm::vec3(mv.x / 16384.f, mv.y / 16384.f, mv.z / 16384.f);
			mesh_1.vertices.push_back(v);
		}
		for(const MobyVertex& mv : submesh.vertices_4) {
			Vertex v;
			v.pos = glm::vec3(mv.x / 16384.f, mv.y / 16384.f, mv.z / 16384.f);
			mesh_1.vertices.push_back(v);
		}
		for(const MobyVertex& mv : submesh.main_vertices) {
			Vertex v;
			v.pos = glm::vec3(mv.x / 16384.f, mv.y / 16384.f, mv.z / 16384.f);
			mesh_1.vertices.push_back(v);
		}
		u8 index_queue[3] = {0};
		s32 index_pos = 0;
		for(s8 index : submesh.indices) {
			if(index != 0) {
				if(index > 0) {
					index_queue[index_pos] = vertex_base + index - 1;
					TriFace face;
					face.v0 = index_queue[(index_pos + 1) % 3];
					face.v1 = index_queue[(index_pos + 2) % 3];
					face.v2 = index_queue[(index_pos + 3) % 3];
					mesh_1.tris.push_back(face);
				} else {
					index_queue[index_pos] = vertex_base + index + 127;
				}
				index_pos = (index_pos + 1) % 3;
			}
		}
	}
	scene.meshes.emplace_back(std::move(mesh_1));
	return scene;
}
