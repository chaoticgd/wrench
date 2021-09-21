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
static void write_moby_sequences(OutBuffer dest, const std::vector<MobySequence>& sequences);
static MobyCollision read_moby_collision(Buffer src);
static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision);
static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count);
using GifUsageTable = std::vector<MobyGifUsageTableEntry>;
static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes);
static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count);
static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes);
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const std::vector<u8>& ind, const std::vector<s32>& texs);

MobyClassData read_moby_class(Buffer src) {
	auto header = src.read<MobyClassHeader>(0, "moby class header");
	MobyClassData moby;
	moby.unknown_9 = header.unknown_9;
	moby.lod_trans = header.lod_trans;
	moby.shadow = header.shadow;
	moby.scale = header.scale;
	moby.bangles = header.bangles;
	moby.mip_dist = header.mip_dist;
	moby.corncob = header.corncob;
	moby.bounding_sphere = header.bounding_sphere.unpack();
	moby.glow_rgba = header.glow_rgba;
	moby.mode_bits = header.mode_bits;
	moby.type = header.type;
	moby.mode_bits2 = header.mode_bits2;
	moby.sequences = read_moby_sequences(src, header.sequence_count);
	if(header.collision != 0) {
 		moby.collision = read_moby_collision(src.subbuf(header.collision));
	}
	for(const Mat4& matrix : src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton")) {
		moby.skeleton.push_back(matrix.unpack());
	}
	moby.common_trans = src.read_bytes(header.common_trans, header.joint_count * 0x10, "moby common trans");
	if(header.joint_count > 0) {
		moby.anim_joints = src.read_bytes(header.anim_joints, 0x80, "moby anim joints");
	} else {
		moby.anim_joints = src.read_bytes(header.anim_joints, 0x10, "moby anim joints");
	}
	moby.sound_defs = src.read_multiple<MobySoundDef>(header.sound_defs, header.sound_count, "moby sound defs").copy();
	moby.submeshes_1 = read_moby_submeshes(src, header.submesh_table_offset, header.submesh_count_1);
	moby.submeshes_2 = read_moby_submeshes(src, header.submesh_table_offset + header.submesh_count_1 * 0x10, header.submesh_count_2);
	moby.metal_submeshes = read_moby_metal_submeshes(src, header.submesh_table_offset + header.metal_submesh_begin * 0x10, header.metal_submesh_count);
	return moby;
}

static s64 class_header_ofs;

void write_moby_class(OutBuffer dest, const MobyClassData& moby) {
	MobyClassHeader header = {0};
	class_header_ofs = dest.alloc<MobyClassHeader>();
	
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
	header.bangles = moby.bangles;
	header.mip_dist = moby.mip_dist;
	header.corncob = moby.corncob;
	header.bounding_sphere = Vec4f::pack(moby.bounding_sphere);
	header.glow_rgba = moby.glow_rgba;
	header.mode_bits = moby.mode_bits;
	header.type = moby.type;
	header.mode_bits2 = moby.mode_bits2;
	
	verify(moby.sequences.size() < 256, "Moby class has too many sequences (max is 255).");
	header.sequence_count = moby.sequences.size();
	write_moby_sequences(dest, moby.sequences);
	dest.pad(0x10);
	s64 submesh_table_1_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes_1.size());
	s64 submesh_table_2_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes_2.size());
	s64 metal_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.metal_submeshes.size());
	header.submesh_table_offset = submesh_table_1_ofs - class_header_ofs;
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision) - class_header_ofs;
	}
	dest.pad(0x40);
	header.skeleton = dest.tell() - class_header_ofs;
	verify(moby.skeleton.size() < 255, "Moby class has too many joints.");
	header.joint_count = moby.skeleton.size();
	for(const glm::mat4& matrix : moby.skeleton) {
		dest.write(Mat4::pack(matrix));
	}
	dest.pad(0x10);
	header.common_trans = dest.write_multiple(moby.common_trans) - class_header_ofs;
	dest.pad(0x10);
	header.anim_joints = dest.write_multiple(moby.anim_joints) - class_header_ofs;
	dest.pad(0x10);
	header.sound_defs = dest.write_multiple(moby.sound_defs) - class_header_ofs;
	std::vector<MobyGifUsageTableEntry> gif_usage;
	write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, moby.submeshes_1);
	write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, moby.submeshes_2);
	write_moby_metal_submeshes(dest, metal_submesh_table_ofs, moby.metal_submeshes);
	assert(gif_usage.size() > 0);
	gif_usage.back().offset_and_terminator |= 0x80000000;
	header.gif_usage = dest.write_multiple(gif_usage) - class_header_ofs;
	dest.write(class_header_ofs, header);
}

static std::vector<MobySequence> read_moby_sequences(Buffer src, s64 sequence_count) {
	std::vector<MobySequence> sequences;
	auto sequence_offsets = src.read_multiple<s32>(sizeof(MobyClassHeader), sequence_count, "moby sequences");
	for(s32 seq_offset : sequence_offsets) {
		auto seq_header = src.read<MobySequenceHeader>(seq_offset, "moby sequence header");
		MobySequence sequence;
		sequence.bounding_sphere = seq_header.bounding_sphere.unpack();
		
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

static void write_moby_sequences(OutBuffer dest, const std::vector<MobySequence>& sequences) {
	s64 sequence_pointer_ofs = dest.alloc_multiple<s32>(sequences.size());
	for(const MobySequence& sequence : sequences) {
		dest.pad(0x10);
		s64 seq_header_ofs = dest.alloc<MobySequenceHeader>();
		dest.write(sequence_pointer_ofs, seq_header_ofs - class_header_ofs);
		sequence_pointer_ofs += 4;
		
		MobySequenceHeader seq_header = {0};
		seq_header.bounding_sphere = Vec4f::pack(sequence.bounding_sphere);
		seq_header.frame_count = sequence.frames.size();
		seq_header.sound_count = -1;
		seq_header.trigger_count = sequence.triggers.size();
		seq_header.pad = -1;
		
		s64 frame_pointer_ofs = dest.alloc_multiple<s32>(sequence.frames.size());
		dest.write_multiple(sequence.triggers);
		if(sequence.trigger_data.has_value()) {
			seq_header.triggers = dest.write(*sequence.trigger_data) - seq_header_ofs;
		}
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
		submesh.indices = unpacks.at(1).data;
		if(unpacks.size() >= 3) {
			Buffer texture_data(unpacks.at(2).data);
			for(s64 i = 0; i < texture_data.size(); i+= 0x40) {
				submesh.textures.push_back(texture_data.read<s32>(i + 0x20, "moby a+d unpack"));
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
			continue;
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
			submesh.trailing_vertices.emplace_back(src.read<MobyVertex>(vertices_ofs, "vertex table"));
			vertices_ofs += 0x10;
			if(submesh.trailing_vertices.back().unknown_4 != 0) {
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
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh.indices, submesh.textures);
		
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
		submesh.indices = unpacks.at(0).data;
		if(unpacks.size() >= 2) {
			Buffer texture_data(unpacks.at(1).data);
			for(s64 i = 0; i < texture_data.size(); i+= 0x40) {
				submesh.textures.push_back(texture_data.read<s32>(i + 0x20, "moby a+d unpack"));
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
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh.indices, submesh.textures);
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

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const std::vector<u8>& ind, const std::vector<s32>& texs) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	VifPacket index_unpack;
	index_unpack.code.interrupt = 0;
	index_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
	index_unpack.code.num = ind.size() / 4;
	index_unpack.code.unpack.vnvl = VifVnVl::V4_8;
	index_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	index_unpack.code.unpack.usn = VifUsn::SIGNED;
	index_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS;
	index_unpack.data = ind;
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
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + ind.size() / 4;
		u8 ad_data[0x40] = {
			0x92, 0xff, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0xa0, 0x41, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};
		for(s32 index : texs) {
			*(s32*) &ad_data[0x20] = index;
			texture_unpack.data.insert(texture_unpack.data.end(), ad_data, ad_data + sizeof(ad_data));
		}
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsageTableEntry gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			for(s32 index : texs) {
				assert(index < 16);
				assert(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = index;
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
