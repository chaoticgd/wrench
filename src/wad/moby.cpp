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

// Debug settings.
#define MOBY_EXPORT_SUBMESHES_SEPERATELY false

static MobyBangles read_moby_bangles(Buffer src);
static s64 write_moby_bangles(OutBuffer dest, const MobyBangles& bangles);
static MobyCornCob read_moby_corncob(Buffer src);
static s64 write_moby_corncob(OutBuffer dest, const MobyCornCob& corncob);
static std::vector<Opt<MobySequence>> read_moby_sequences(Buffer src, s64 sequence_count);
static void write_moby_sequences(OutBuffer dest, const std::vector<Opt<MobySequence>>& sequences, s64 list_ofs, MobyFormat format);
static MobyCollision read_moby_collision(Buffer src);
static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision);
static std::vector<MobyJointEntry> read_moby_joints(Buffer src, s64 joints_ofs);
static s64 write_moby_joints(OutBuffer dest, const std::vector<MobyJointEntry>& joints);
static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, MobyFormat format);
using GifUsageTable = std::vector<MobyGifUsageTableEntry>;
static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes, MobyFormat format);
static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count);
static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes);
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh);
#define NO_SUBMESH_FILTER -1
static Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, f32 scale, s32 submesh_filter);
static std::vector<Joint> recover_moby_joints(const MobyClassData& moby);
static std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials, f32 scale);
struct IndexMappingRecord {
	s32 submesh = -1;
	s32 index = -1; // The index of the vertex in the vertex table.
	s32 id = -1; // The index of the vertex in the intermediate buffer.
	s32 dedup_out_edge = -1; // If this vertex is a duplicate, this points to the canonical vertex.
};
static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices);

// FIXME: Figure out what points to the mystery data instead of doing this.
static s64 mystery_data_ofs;

MobyClassData read_moby_class(Buffer src, Game game) {
	auto header = src.read<MobyClassHeader>(0, "moby class header");
	MobyClassData moby;
	moby.submesh_count = header.submesh_count;
	moby.low_lod_submesh_count = header.low_lod_submesh_count;
	moby.metal_submesh_count = header.metal_submesh_count;
	moby.unknown_9 = header.unknown_9;
	moby.rac1_byte_a = header.rac1_byte_a;
	moby.rac1_byte_b = header.rac12_byte_b;
	moby.lod_trans = header.lod_trans;
	moby.shadow = header.shadow;
	moby.scale = header.scale;
	moby.mip_dist = header.mip_dist;
	moby.bounding_sphere = header.bounding_sphere.unpack();
	moby.glow_rgba = header.glow_rgba;
	moby.mode_bits = header.mode_bits;
	moby.type = header.type;
	moby.mode_bits2 = header.mode_bits2;
	mystery_data_ofs = src.read<s32>(0x48, "moby sequences");
	
	MobyFormat format;
	switch(game) {
		case Game::RAC1:
			format = MobyFormat::RAC1;
			break;
		case Game::RAC2:
			if(header.rac12_byte_b == 0) {
				format = MobyFormat::RAC2;
			} else {
				format = MobyFormat::RAC1;
				moby.force_rac1_format = true;
			}
			break;
		case Game::RAC3:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	moby.header_end_offset = 0x48;
	for(s32 seq_offset : src.read_multiple<s32>(0x48, header.sequence_count, "sequence offsets")) {
		if(seq_offset != 0) {
			moby.header_end_offset = seq_offset;
			break;
		}
	}
	if(header.bangles != 0) {
		moby.bangles = read_moby_bangles(src.subbuf(header.bangles * 0x10));
		moby.header_end_offset = std::min(moby.header_end_offset, header.bangles * 0x10);
	}
	if(game == Game::RAC1)  {
		moby.rac1_short_2e = header.corncob;
	} else if(header.corncob != 0) {
		moby.corncob = read_moby_corncob(src.subbuf(header.corncob * 0x10));
		moby.header_end_offset = std::min(moby.header_end_offset, header.corncob * 0x10);
	}
	if(game != Game::DL) { // TODO: Get this working.
		moby.sequences = read_moby_sequences(src, header.sequence_count);
	}
	verify(header.sequence_count >= 1, "Moby class has no sequences.");
	if(header.collision != 0) {
		moby.collision = read_moby_collision(src.subbuf(header.collision));
		s64 coll_size = 0x10 + moby.collision->first_part.size() + moby.collision->second_part.size() * 8 + moby.collision->third_part.size();
		mystery_data_ofs = std::max(mystery_data_ofs, header.collision + coll_size);
	}
	moby.skeleton = src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton").copy();
	moby.common_trans = src.read_multiple<MobyTrans>(header.common_trans, header.joint_count, "skeleton trans").copy();
	if(game != Game::DL) { // TODO: Get this working.
		moby.joints = read_moby_joints(src, header.joints);
	}
	moby.sound_defs = src.read_multiple<MobySoundDef>(header.sound_defs, header.sound_count, "moby sound defs").copy();
	if(header.submesh_table_offset != 0) {
		moby.has_submesh_table = true;
		moby.submesh_table_offset = header.submesh_table_offset;
		moby.submeshes = read_moby_submeshes(src, header.submesh_table_offset, header.submesh_count, format);
		moby.low_lod_submeshes = read_moby_submeshes(src, header.submesh_table_offset + header.submesh_count * 0x10, header.low_lod_submesh_count, format);
		s64 metal_table_ofs = header.submesh_table_offset + header.metal_submesh_begin * 0x10;
		moby.metal_submeshes = read_moby_metal_submeshes(src, metal_table_ofs, header.metal_submesh_count);
		if(header.bangles != 0) {
			MobyBangle& first_bangle = moby.bangles->bangles.at(0);
			s64 bangles_submesh_table_ofs = header.submesh_table_offset + first_bangle.submesh_begin * 0x10;
			moby.bangles->submeshes = read_moby_submeshes(src, bangles_submesh_table_ofs, first_bangle.submesh_count, format);
			mystery_data_ofs = std::max(mystery_data_ofs, bangles_submesh_table_ofs + first_bangle.submesh_count * 0x10);
		} else {
			mystery_data_ofs = std::max(mystery_data_ofs, metal_table_ofs + header.metal_submesh_count * 0x10);
		}
	}
	moby.mystery_data = src.read_bytes(mystery_data_ofs, header.skeleton - mystery_data_ofs, "moby mystery data");
	return moby;
}

static s64 class_header_ofs;

void write_moby_class(OutBuffer dest, const MobyClassData& moby, Game game) {
	MobyClassHeader header = {0};
	class_header_ofs = dest.alloc<MobyClassHeader>();
	assert(class_header_ofs % 0x40 == 0);
	
	MobyFormat format;
	switch(game) {
		case Game::RAC1:
			format = MobyFormat::RAC1;
			break;
		case Game::RAC2:
			format = moby.force_rac1_format > 0 ? MobyFormat::RAC1 : MobyFormat::RAC2;
			break;
		case Game::RAC3:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	
	assert(!moby.has_submesh_table | (moby.submeshes.size() == moby.submesh_count));
	verify(moby.submeshes.size() < 256, "Moby class has too many submeshes.");
	header.submesh_count = moby.submesh_count;
	assert(!moby.has_submesh_table | (moby.low_lod_submeshes.size() == moby.low_lod_submesh_count));
	verify(moby.low_lod_submeshes.size() < 256, "Moby class has too many low detail submeshes.");
	header.low_lod_submesh_count = moby.low_lod_submesh_count;
	assert(!moby.has_submesh_table | (moby.metal_submeshes.size() == moby.metal_submesh_count));
	verify(moby.metal_submeshes.size() < 256, "Moby class has too many metal submeshes.");
	header.metal_submesh_count = moby.metal_submesh_count;
	header.metal_submesh_begin = moby.submesh_count + moby.low_lod_submesh_count;
	if(format == MobyFormat::RAC1) {
		header.rac1_byte_a = moby.rac1_byte_a;
		header.rac12_byte_b = moby.rac1_byte_b;
	}
	header.unknown_9 = moby.unknown_9;
	header.lod_trans = moby.lod_trans;
	header.shadow = moby.shadow;
	header.scale = moby.scale;
	verify(moby.sound_defs.size() < 256, "Moby class has too many sounds.");
	header.sound_count = moby.sound_defs.size();
	header.mip_dist = moby.mip_dist;
	header.bounding_sphere = Vec4f::pack(moby.bounding_sphere);
	header.glow_rgba = moby.glow_rgba;
	header.mode_bits = moby.mode_bits;
	header.type = moby.type;
	header.mode_bits2 = moby.mode_bits2;
	
	verify(moby.sequences.size() < 256, "Moby class has too many sequences (max is 255).");
	header.sequence_count = moby.sequences.size();
	s64 sequence_list_ofs = dest.alloc_multiple<s32>(moby.sequences.size());
	while(dest.tell() - class_header_ofs < moby.header_end_offset) {
		dest.write<u8>(0);
	}
	if(moby.bangles.has_value()) {
		dest.pad(0x10);
		header.bangles = (write_moby_bangles(dest, *moby.bangles) - class_header_ofs) / 0x10;
	}
	if(game == Game::RAC1) {
		header.corncob = moby.rac1_short_2e;
	} else if(moby.corncob.has_value()) {
		dest.pad(0x10);
		header.corncob = (write_moby_corncob(dest, *moby.corncob) - class_header_ofs) / 0x10;
	}
	dest.pad(0x10);
	write_moby_sequences(dest, moby.sequences, sequence_list_ofs, format);
	dest.pad(0x10);
	while(dest.tell() < class_header_ofs + moby.submesh_table_offset) {
		dest.write<u8>(0);
	}
	s64 submesh_table_1_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes.size());
	s64 submesh_table_2_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.low_lod_submeshes.size());
	s64 metal_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.metal_submeshes.size());
	s64 bangles_submesh_table_ofs = 0;
	if(moby.bangles.has_value()) {
		bangles_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.bangles->submeshes.size());
	}
	if(moby.has_submesh_table) {
		header.submesh_table_offset = submesh_table_1_ofs - class_header_ofs;
	}
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision) - class_header_ofs;
	}
	dest.write_multiple(moby.mystery_data);
	header.skeleton = dest.tell() - class_header_ofs;
	verify(moby.skeleton.size() < 255, "Moby class has too many joints.");
	header.joint_count = moby.skeleton.size();
	dest.write_multiple(moby.skeleton);
	dest.pad(0x10);
	header.common_trans = dest.write_multiple(moby.common_trans) - class_header_ofs;
	header.joints = write_moby_joints(dest, moby.joints) - class_header_ofs;
	dest.pad(0x10);
	if(moby.sound_defs.size() > 0) {
		header.sound_defs = dest.write_multiple(moby.sound_defs) - class_header_ofs;
	}
	std::vector<MobyGifUsageTableEntry> gif_usage;
	write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, moby.submeshes, format);
	write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, moby.low_lod_submeshes, format);
	write_moby_metal_submeshes(dest, metal_submesh_table_ofs, moby.metal_submeshes);
	if(moby.bangles.has_value()) {
		write_moby_submeshes(dest, gif_usage, bangles_submesh_table_ofs, moby.bangles->submeshes, format);
	}
	if(gif_usage.size() > 0) {
		gif_usage.back().offset_and_terminator |= 0x80000000;
		header.gif_usage = dest.write_multiple(gif_usage) - class_header_ofs;
	}
	dest.write(class_header_ofs, header);
}

static MobyBangles read_moby_bangles(Buffer src) {
	MobyBangles bangles;
	bangles.bangles = src.read_multiple<MobyBangle>(0, 16, "moby bangles").copy();
	s32 bangle_count = 0;
	for(MobyBangle& bangle : bangles.bangles) {
		if(bangle.submesh_begin != 0 || bangle.submesh_begin != 0) {
			bangle_count++;
		}
	}
	bangles.vertices = src.read_multiple<MobyVertexPosition>(0x40, 2 * (bangle_count - 1), "moby bangle vertices").copy();
	return bangles;
}

static s64 write_moby_bangles(OutBuffer dest, const MobyBangles& bangles) {
	s64 ofs = dest.tell();
	dest.write_multiple(bangles.bangles);
	dest.write_multiple(bangles.vertices);
	return ofs;
}

static MobyCornCob read_moby_corncob(Buffer src) {
	MobyCornCob corncob;
	auto header = src.read<MobyCornCobHeader>(0, "moby corncob");
	for(s32 i = 0; i < 16; i++) {
		if(header.kernels[i] != 0xff) {
			MobyCornKernel kernel;
			s64 kernel_ofs = header.kernels[i] * 0x10;
			kernel.vec = src.read<Vec4f>(kernel_ofs, "corn vec4");
			if(src.read<u64>(kernel_ofs, "corn") != 0 || src.read<u64>(kernel_ofs + 8, "corn") != 0) {
				s16 vertex_count = src.read<s16>(kernel_ofs + 0x16, "corn vertex count");
				kernel.vertices = src.read_multiple<MobyVertexPosition>(kernel_ofs + 0x10, vertex_count, "corn vertices").copy();
			}
			corncob.kernels[i] = std::move(kernel);
		}
	}
	return corncob;
}

static s64 write_moby_corncob(OutBuffer dest, const MobyCornCob& corncob) {
	s64 header_ofs = dest.alloc<MobyCornCobHeader>();
	MobyCornCobHeader header;
	for(s32 i = 0; i < 16; i++) {
		if(corncob.kernels[i].has_value()) {
			const MobyCornKernel& kernel = *corncob.kernels[i];
			dest.pad(0x10);
			s64 kernel_ofs = dest.tell();
			dest.write(kernel.vec);
			dest.write_multiple(kernel.vertices);
			if(kernel.vertices.size() > 0) {
				dest.write<s16>(kernel_ofs + 0x16, (s16) kernel.vertices.size());
			}
			header.kernels[i] = (kernel_ofs - header_ofs) / 0x10;
			
		} else {
			header.kernels[i] = 0xff;
		}
	}
	dest.write(header_ofs, header);
	return header_ofs;
}

static std::vector<Opt<MobySequence>> read_moby_sequences(Buffer src, s64 sequence_count) {
	std::vector<Opt<MobySequence>> sequences;
	auto sequence_offsets = src.read_multiple<s32>(sizeof(MobyClassHeader), sequence_count, "moby sequences");
	for(s32 seq_offset : sequence_offsets) {
		if(seq_offset == 0) {
			sequences.emplace_back(std::nullopt);
			continue;
		}
		
		auto seq_header = src.read<MobySequenceHeader>(seq_offset, "moby sequence header");
		MobySequence sequence;
		sequence.bounding_sphere = seq_header.bounding_sphere.unpack();
		sequence.animation_info = seq_header.animation_info;
		sequence.sound_count = seq_header.sound_count;
		
		auto frame_table = src.read_multiple<s32>(seq_offset + 0x1c, seq_header.frame_count, "moby sequence table");
		for(s32 frame_offset : frame_table) {
			frame_offset &= 0xfffffff; // Hack for some mobies on R&C2 Oozla.
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

static void write_moby_sequences(OutBuffer dest, const std::vector<Opt<MobySequence>>& sequences, s64 list_ofs, MobyFormat format) {
	for(const Opt<MobySequence>& sequence_opt : sequences) {
		if(!sequence_opt.has_value()) {
			dest.write<s32>(list_ofs, 0);
			list_ofs += 4;
			continue;
		}
		
		const MobySequence& sequence = *sequence_opt;
		
		dest.pad(0x10);
		s64 seq_header_ofs = dest.alloc<MobySequenceHeader>();
		dest.write<s32>(list_ofs, seq_header_ofs - class_header_ofs);
		list_ofs += 4;
		
		MobySequenceHeader seq_header = {0};
		seq_header.bounding_sphere = Vec4f::pack(sequence.bounding_sphere);
		seq_header.frame_count = sequence.frames.size();
		seq_header.sound_count = sequence.sound_count;
		seq_header.trigger_count = sequence.triggers.size();
		seq_header.pad = format == MobyFormat::RAC1 ? 0 : -1;
		
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

static std::vector<MobyJointEntry> read_moby_joints(Buffer src, s64 joints_ofs) {
	std::vector<MobyJointEntry> joints;
	s32 list_count = src.read<s32>(joints_ofs, "joint list count");
	for(s32 i = 0; i < list_count; i++) {
		MobyJointEntry joint;
		s32 list_ofs = src.read<s32>(joints_ofs + (i + 1) * 4, "joint list");
		s16 thing_one_count = src.read<s16>(list_ofs, "joint count 1");
		list_ofs += 2;
		s16 thing_two_count = src.read<s16>(list_ofs, "joint count 2");
		list_ofs += 2;
		joint.thing_one = src.read_multiple<u8>(list_ofs, thing_one_count, "joint thing ones").copy();
		list_ofs += thing_one_count;
		joint.thing_two = src.read_multiple<u8>(list_ofs, thing_two_count, "joint thing twos").copy();
		list_ofs += thing_two_count;
		verify(src.read<u8>(list_ofs, "joint list terminator") == 0xff, "Bad joint data.");
		joints.emplace_back(std::move(joint));
	}
	return joints;
}

static s64 write_moby_joints(OutBuffer dest, const std::vector<MobyJointEntry>& joints) {
	dest.pad(0x10);
	s64 base_ofs = dest.tell();
	dest.write<s32>(joints.size());
	s64 outer_list_ofs = dest.alloc_multiple<s32>(joints.size());
	for(const MobyJointEntry& joint : joints) {
		dest.pad(0x4);
		dest.write<s32>(outer_list_ofs, dest.tell() - class_header_ofs);
		outer_list_ofs += 4;
		dest.write<s16>(joint.thing_one.size());
		dest.write<s16>(joint.thing_two.size());
		dest.write_multiple(joint.thing_one);
		dest.write_multiple(joint.thing_two);
		dest.write<u8>(0xff);
	}
	return base_ofs;
}

static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, MobyFormat format) {
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
			vertex_header.unknown_count_0 = compact_vertex_header.unknown_count_0;
			vertex_header.vertex_count_2 = compact_vertex_header.vertex_count_2;
			vertex_header.vertex_count_4 = compact_vertex_header.vertex_count_4;
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
		submesh.unknowns = src.read_multiple<u16>(array_ofs, vertex_header.unknown_count_0, "vertex table").copy();
		array_ofs += vertex_header.unknown_count_0 * 2;
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
		s32 in_file_vertex_count = vertex_header.vertex_count_2 + vertex_header.vertex_count_4 + vertex_header.main_vertex_count;
		submesh.vertices = src.read_multiple<MobyVertex>(vertex_ofs, in_file_vertex_count, "vertex table").copy();
		vertex_ofs += in_file_vertex_count * 0x10;
		submesh.vertex_count_2 = vertex_header.vertex_count_2;
		submesh.vertex_count_4 = vertex_header.vertex_count_4;
		submesh.unknown_e = vertex_header.unknown_e;
		if(format == MobyFormat::RAC1) {
			s32 unknown_e_size = entry.vertex_data_size * 0x10 - vertex_header.unknown_e;
			submesh.unknown_e_data = src.read_bytes(entry.vertex_offset + vertex_header.unknown_e, unknown_e_size, "vertex table unknown_e data");
		}
		
		// Fix vertex indices (see comment in write_moby_submeshes).
		for(size_t i = 7; i < submesh.vertices.size(); i++) {
			submesh.vertices[i - 7].low_word = (submesh.vertices[i - 7].low_word & ~0x1ff) | (submesh.vertices[i].low_word & 0x1ff);
		}
		s32 trailing_vertex_count = 0;
		if(format == MobyFormat::RAC1) {
			trailing_vertex_count = (vertex_header.unknown_e - vertex_header.vertex_table_offset) / 0x10 - in_file_vertex_count;
		} else {
			trailing_vertex_count = entry.vertex_data_size - vertex_header.vertex_table_offset / 0x10 - in_file_vertex_count;
		}
		verify(trailing_vertex_count < 7, "Bad moby vertex table.");
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

static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes, MobyFormat format) {
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
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh);
		
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
		s64 vertex_header_ofs;
		if(format == MobyFormat::RAC1) {
			vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac1>();
		} else {
			vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac23DL>();
		}
		MobyVertexTableHeaderRac1 vertex_header;
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
		
		if(format == MobyFormat::RAC1) {
			vertex_header.unknown_e = dest.tell() - vertex_header_ofs;
			dest.write_multiple(submesh.unknown_e_data);
			dest.write(vertex_header_ofs, vertex_header);
		} else {
			MobyVertexTableHeaderRac23DL compact_vertex_header;
			compact_vertex_header.unknown_count_0 = vertex_header.unknown_count_0;
			compact_vertex_header.vertex_count_2 = vertex_header.vertex_count_2;
			compact_vertex_header.vertex_count_4 = vertex_header.vertex_count_4;
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

static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count) {
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

static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes) {
	for(const MobyMetalSubMesh& submesh : submeshes) {
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh);
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

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh) {
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

ColladaScene recover_moby_class(const MobyClassData& moby, s32 o_class, s32 texture_count) {
	ColladaScene scene;
	
	// Used for when the texture index stored in a GS primitive is -1.
	Material& none = scene.materials.emplace_back();
	none.name = "none";
	none.colour = glm::vec4(1, 1, 1, 1);
	// Used for when there're more textures referenced than are listed in the
	// moby class table. This happens for R&C2 ship parts.
	Material& dummy = scene.materials.emplace_back();
	dummy.name = "dummy";
	dummy.colour = glm::vec4(0.5, 0.5, 0.5, 1);
	
	for(s32 texture = 0; texture < texture_count; texture++) {
		Material& mat = scene.materials.emplace_back();
		mat.name = "mat_" + std::to_string(texture);
		mat.texture = texture;
	}
	for(s32 texture = 0; texture < texture_count; texture++) {
		Material& chrome = scene.materials.emplace_back();
		chrome.name = "chrome_" + std::to_string(texture);
		chrome.texture = texture;
	}
	for(s32 texture = 0; texture < texture_count; texture++) {
		Material& glass = scene.materials.emplace_back();
		glass.name = "glass_" + std::to_string(texture);
		glass.texture = texture;
	}
	
	if(MOBY_EXPORT_SUBMESHES_SEPERATELY) {
		for(s32 i = 0; i < (s32) moby.submeshes.size(); i++) {
			std::string name = "high_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, name.c_str(), o_class, texture_count, moby.scale, i));
		}
		for(s32 i = 0; i < (s32) moby.low_lod_submeshes.size(); i++) {
			std::string name = "low_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, name.c_str(), o_class, texture_count, moby.scale, i));
		}
		if(moby.bangles.has_value()) {
			for(s32 i = 0; i < (s32) moby.bangles->submeshes.size(); i++) {
				std::string name = "bangles_" + std::to_string(i);
				scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, name.c_str(), o_class, texture_count, moby.scale, i));
			}
		}
	} else {
		scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, "high_lod", o_class, texture_count, moby.scale, NO_SUBMESH_FILTER));
		scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, "low_lod", o_class, texture_count, moby.scale, NO_SUBMESH_FILTER));
		if(moby.bangles.has_value()) {
			scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, "bangles", o_class, texture_count, moby.scale, NO_SUBMESH_FILTER));
		}
	}
	
	if(moby.joints.size() != 0) {
		scene.joints = recover_moby_joints(moby);
	}
	
	return scene;
}

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, submesh %d has bad " message ".", o_class, i);

static Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, f32 scale, s32 submesh_filter) {
	Mesh mesh;
	mesh.name = name;
	mesh.flags = MESH_HAS_TEX_COORDS;
	// The game stores this on the end of the VU chain.
	Opt<MobyVertex> intermediate_buffer[512];
	SubMesh dest;
	dest.material = 0;
	for(s32 i = 0; i < submeshes.size(); i++) {
		const MobySubMesh src = submeshes[i];
		bool lift_submesh = !MOBY_EXPORT_SUBMESHES_SEPERATELY || submesh_filter == -1 || i == submesh_filter; // This is just for debugging.
		s32 vertex_base = mesh.vertices.size();
		for(const MobyVertex& mv : src.vertices) {
			auto& st = src.sts.at(mesh.vertices.size() - vertex_base);
			auto pos = glm::vec3(mv.regular.x * scale / 1024.f, mv.regular.y * scale / 1024.f, mv.regular.z * scale / 1024.f);
			auto normal = glm::vec3(0, 0, 0);
			auto tex_coord = glm::vec2(st.s / (INT16_MAX / 8.f), -st.t / (INT16_MAX / 8.f));
			while(tex_coord.s < 0) tex_coord.s += 1;
			while(tex_coord.t < 0) tex_coord.t += 1;
			mesh.vertices.emplace_back(pos, normal, tex_coord);
			
			intermediate_buffer[mv.low_word & 0x1ff] = mv;
		}
		for(u16 dupe : src.duplicate_vertices) {
			auto mv = intermediate_buffer[dupe];
			VERIFY_SUBMESH(mv.has_value(), "vertex table");
			
			auto& st = src.sts.at(mesh.vertices.size() - vertex_base);
			auto pos = glm::vec3(mv->regular.x * scale / 1024.f, mv->regular.y * scale / 1024.f, mv->regular.z * scale / 1024.f);
			auto normal = glm::vec3(0, 0, 0);
			auto tex_coord = glm::vec2(st.s / (INT16_MAX / 8.f), -st.t / (INT16_MAX / 8.f));
			while(tex_coord.s < 0) tex_coord.s += 1;
			while(tex_coord.t < 0) tex_coord.t += 1;
			mesh.vertices.emplace_back(pos, normal, tex_coord);
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

static std::vector<Joint> recover_moby_joints(const MobyClassData& moby) {
	assert(moby.skeleton.size() == moby.common_trans.size());
	
	std::vector<Joint> joints;
	joints.reserve(1 + moby.skeleton.size());
	
	for(size_t i = 0; i < moby.skeleton.size(); i++) {
		const Mat4& skeleton = moby.skeleton[i];
		const MobyTrans& trans = moby.common_trans[i];
		Joint j;
		j.matrix = skeleton.unpack();
		j.matrix[0][3] *= moby.scale / 1024.f;
		j.matrix[1][3] *= moby.scale / 1024.f;
		j.matrix[2][3] *= moby.scale / 1024.f;
		j.matrix[3] = glm::vec4(0, 0, 0, 1);
		s32 parent;
		if(i > 0) {
			parent = trans.parent_offset / 0x40;
		} else {
			parent = -1;
		}
		verify(parent < (s32) joints.size(), "Bad moby joints.");
		add_joint(joints, j, parent);
	}
	
	return joints;
}

MobyClassData build_moby_class(const ColladaScene& scene) {
	const Mesh* high_lod_mesh = nullptr;
	const Mesh* low_lod_mesh = nullptr;
	for(const Mesh& mesh : scene.meshes) {
		if(mesh.name == "high_lod") {
			high_lod_mesh = &mesh;
		}
		if(mesh.name == "low_lod") {
			low_lod_mesh = &mesh;
		}
	}
	verify(high_lod_mesh, "Collada file doesn't contain a 'high_lod' node.");
	
	MobyClassData moby;
	moby.submeshes = build_moby_submeshes(*high_lod_mesh, scene.materials, 0.25);
	moby.submesh_count = moby.submeshes.size();
	if(low_lod_mesh) {
		moby.low_lod_submeshes = build_moby_submeshes(*low_lod_mesh, scene.materials, 0.25);
		moby.low_lod_submesh_count = moby.low_lod_submeshes.size();
	}
	moby.unknown_9 = 0;
	moby.lod_trans = 0x20;
	moby.shadow = 0;
	moby.scale = 0.25;
	moby.mip_dist = 0x8;
	moby.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	moby.glow_rgba = 0;
	moby.mode_bits = 0x5000;
	moby.type = 0;
	moby.mode_bits2 = 0;
	moby.header_end_offset = 0;
	moby.submesh_table_offset = 0;
	moby.rac1_byte_a = 0;
	moby.rac1_byte_b = 0;
	moby.rac1_short_2e = 0;
	moby.has_submesh_table = true;
	
	MobySequence dummy_seq;
	dummy_seq.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	dummy_seq.frames.emplace_back();
	moby.sequences.emplace_back(std::move(dummy_seq));
	
	return moby;
}

struct RichIndex {
	s32 index : 30;
	s32 restart : 1;
	s32 is_dupe : 1 = 0;
};

static std::vector<RichIndex> fake_tristripper(const std::vector<Face>& faces) {
	std::vector<RichIndex> indices;
	for(const Face& face : faces) {
		indices.push_back({face.v0, 1});
		indices.push_back({face.v1, 1});
		indices.push_back({face.v2, 0});
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

static std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials, f32 scale) {
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
				
				mid.vertices.push_back({r.index, r.index});
			} else if(mapping.submesh != mid_submeshes.size()) {
				if(canonical.id == -1) {
					canonical.id = next_id++;
					mid.vertices.at(canonical.index).id = canonical.id;
				}
				mid.duplicate_vertices.push_back({canonical.id, r.index});
			}
			
			if(mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT - 4) {
				new_submesh();
				continue;
			}
			
			mid.indices.push_back({canonical.index, r.restart, r.is_dupe});
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
			
			MobyVertex low_vert = {0};
			low_vert.low_word = vertex.id;
			low_vert.regular.x = high_vert.pos.x * inverse_scale;
			low_vert.regular.y = high_vert.pos.y * inverse_scale;
			low_vert.regular.z = high_vert.pos.z * inverse_scale;
			low.vertices.push_back(low_vert);
			
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
