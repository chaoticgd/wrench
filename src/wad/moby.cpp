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

MobyClassData read_moby_class(Buffer src) {
	auto header = src.read<MobyClassHeader>(0, "moby class header");
	MobyClassData moby;
	moby.bounding_sphere = header.bounding_sphere.unpack();
	moby.sequences = read_moby_sequences(src, header.sequence_count);
	s32 submesh_count = header.submesh_count_0 + header.submesh_count_1;
	moby.submesh_entries = src.read_multiple<MobySubMeshEntry>(header.submesh_table_offset, submesh_count, "moby submesh table").copy();
	if(header.collision != 0) {
 		moby.collision = read_moby_collision(src.subbuf(header.collision));
	}
	for(const Mat4& matrix : src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton")) {
		moby.skeleton.push_back(matrix.unpack());
	}
	return moby;
}

void write_moby_class(OutBuffer dest, const MobyClassData& moby) {
	MobyClassHeader header = {0};
	s64 header_ofs = dest.alloc<MobyClassHeader>();
	verify(moby.sequences.size() < 256, "Moby class has too many sequences (max is 255).");
	header.sequence_count = moby.sequences.size();
	write_moby_sequences(dest, moby.sequences);
	dest.pad(0x10);
	dest.write_multiple(moby.submesh_entries);
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision);
	}
	dest.pad(0x10);
	header.skeleton = dest.tell();
	verify(moby.skeleton.size() < 255, "Moby class has too many joints.");
	header.joint_count = moby.skeleton.size();
	for(const glm::mat4& matrix : moby.skeleton) {
		dest.write(Mat4::pack(matrix));
	}
	dest.write(header_ofs, header);
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
		dest.write(sequence_pointer_ofs, seq_header_ofs);
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
			dest.write<s32>(frame_pointer_ofs, dest.write(frame_header));
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
