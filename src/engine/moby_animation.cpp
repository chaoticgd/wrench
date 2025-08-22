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

#include "moby_animation.h"

static MobySequence read_moby_sequence(Buffer src, s64 seq_ofs, s32 joint_count, Game game);
static s64 write_moby_sequence(
	OutBuffer dest, const MobySequence& sequence, s64 header_ofs, s32 joint_count, Game game);
static MobySequence read_dl_moby_sequence(Buffer src, s64 seq_ofs, s32 joint_count);
static s64 write_dl_moby_sequence(
	OutBuffer dest, const MobySequence& sequence, s64 header_ofs, s32 joint_count);

std::vector<Opt<MobySequence>> read_moby_sequences(
	Buffer src, s64 sequence_count, s32 joint_count, Game game)
{
	std::vector<Opt<MobySequence>> sequences;
	auto sequence_offsets = src.read_multiple<s32>(0x48, sequence_count, "moby sequences");
	for (s32 seq_offset : sequence_offsets) {
		if (seq_offset == 0) {
			sequences.emplace_back(std::nullopt);
			continue;
		}
		
		if (game == Game::DL) {
			sequences.emplace_back(read_dl_moby_sequence(src, seq_offset, joint_count));
		} else {
			sequences.emplace_back(read_moby_sequence(src, seq_offset, joint_count, game));
		}
	}
	return sequences;
}

void write_moby_sequences(
	OutBuffer dest,
	const std::vector<Opt<MobySequence>>& sequences,
	s64 class_header_ofs,
	s64 list_ofs,
	s32 joint_count,
	Game game)
{
	for (const Opt<MobySequence>& sequence_opt : sequences) {
		if (!sequence_opt.has_value()) {
			dest.write<s32>(list_ofs, 0);
			list_ofs += 4;
			continue;
		}
		
		const MobySequence& sequence = *sequence_opt;
		s64 seq_ofs;
		if (game == Game::DL) {
			seq_ofs = write_dl_moby_sequence(dest, sequence, class_header_ofs, joint_count);
		} else {
			seq_ofs = write_moby_sequence(dest, sequence, class_header_ofs, joint_count, game);
		}
		dest.write<u32>(list_ofs, seq_ofs - class_header_ofs);
		list_ofs += 4;
	}
}

static MobySequence read_moby_sequence(Buffer src, s64 seq_ofs, s32 joint_count, Game game)
{
	auto seq_header = src.read<MobySequenceHeader>(seq_ofs, "moby sequence header");
	MobySequence sequence;
	sequence.bounding_sphere = seq_header.bounding_sphere.unpack();
	sequence.animation_info = seq_header.animation_info;
	sequence.sound_count = seq_header.sound_count;
	sequence.unknown_13 = seq_header.unknown_13;
	
	auto frame_table = src.read_multiple<s32>(seq_ofs + 0x1c, seq_header.frame_count, "moby sequence table");
	for (s32 frame_ofs_and_flag : frame_table) {
		if ((frame_ofs_and_flag & 0xf0000000) != 0) {
			sequence.has_special_data = true;
		}
	}
	
	s64 after_frame_list = seq_ofs + 0x1c + seq_header.frame_count * 4;
	sequence.triggers = src.read_multiple<u32>(after_frame_list, seq_header.trigger_count, "moby sequence trigger list").copy();
	s64 after_trigger_list = after_frame_list + seq_header.trigger_count * 4;
	
	if (!sequence.has_special_data) { // Normal case.
		for (s32 frame_ofs_and_flag : frame_table) {
			MobyFrame frame;
			s32 flag = frame_ofs_and_flag & 0xf0000000;
			s32 frame_ofs = frame_ofs_and_flag & 0x0fffffff;
			
			auto frame_header = src.read<Rac123MobyFrameHeader>(frame_ofs, "moby frame header");
			frame.regular.unknown_0 = frame_header.unknown_0;
			frame.regular.unknown_4 = frame_header.unknown_4;
			frame.regular.unknown_c = frame_header.unknown_c;
			s32 data_ofs = frame_ofs + 0x10;
			frame.regular.joint_data = src.read_multiple<u64>(data_ofs, joint_count, "frame thing 1").copy();
			data_ofs += joint_count * 8;
			frame.regular.thing_1 = src.read_multiple<u64>(data_ofs, frame_header.thing_1_count, "frame thing 1").copy();
			data_ofs += frame_header.thing_1_count * 8;
			frame.regular.thing_2 = src.read_multiple<u64>(data_ofs, frame_header.thing_2_count, "frame thing 2").copy();
			
			s64 end_of_frame = frame_ofs + 0x10 + frame_header.data_size_qwords * 0x10;
			sequence.frames.emplace_back(std::move(frame));
		}
		
		sequence.triggers = src.read_multiple<u32>(after_frame_list, seq_header.trigger_count, "moby sequence trigger list").copy();
	} else { // For Ratchet and a handful of other mobies.
		u32 packed_vals = src.read<u32>(after_trigger_list, "special anim data offsets");
		u32 second_part_ofs = 4 + ((packed_vals & 0b00000000000000000000001111111111) >> 0);
		u32 third_part_ofs = 4 + ((packed_vals & 0b00000000000111111111110000000000) >> 10);
		u32 fourth_part_ofs = 4 + (((packed_vals & 0b11111111111000000000000000000000) >> 21));
		
		sequence.special.joint_data = src.read_multiple<u16>(after_trigger_list + 4, joint_count * 3, "").copy();
		s64 thing_ofs = after_trigger_list + 4 + joint_count * 6;
		
		u8 thing_1_count = src.read<u8>(thing_ofs + 0, "special anim data thing 1 count");
		u8 thing_2_count = src.read<u8>(thing_ofs + 1, "special anim data thing 2 count");
		sequence.special.thing_1 = src.read_multiple<u64>(thing_ofs + 2, thing_1_count, "special anim data thing 1").copy();
		s64 thing_2_ofs = thing_ofs + 2 + thing_1_count * 8;
		sequence.special.thing_2 = src.read_multiple<u64>(thing_2_ofs, thing_2_count, "special anim data thing 2").copy();
		
		for (s32 frame_ofs_and_flag : frame_table) {
			s32 frame_ofs = frame_ofs_and_flag & 0xfffffff;
			
			MobyFrame frame;
			
			frame.special.inverse_unknown_0 = src.read<u16>(frame_ofs, "special anim data unknown 0");
			frame.special.unknown_4 = src.read<u16>(frame_ofs + 2, "special anim data unknown 1");
			frame.special.first_part = src.read_multiple<u8>(frame_ofs + 4, second_part_ofs - 4, "special anim data first part").copy();
			s64 second_part_size = third_part_ofs - second_part_ofs;
			frame.special.second_part = src.read_multiple<u8>(frame_ofs + second_part_ofs, second_part_size, "special anim data second part").copy();
			s64 third_part_size = fourth_part_ofs - third_part_ofs;
			frame.special.third_part = src.read_multiple<u8>(frame_ofs + third_part_ofs, third_part_size, "special anim data third part").copy();
			
			s32 fourth_part_size = joint_count;
			while (fourth_part_size % 8 != 0) fourth_part_size++;
			fourth_part_size /= 8;
			frame.special.fourth_part = src.read_multiple<u8>(frame_ofs + fourth_part_ofs, fourth_part_size, "special anim data fourth part").copy();
			s64 ofs = frame_ofs + fourth_part_ofs + fourth_part_size;
			
			auto read_fifth_part = [&](s32 count) {
				std::vector<u8> part;
				for (s32 i = 0; i < count; i++) {
					u8 packed_flag = src.read<u8>(ofs++, "special anim data flag");
					part.push_back(packed_flag);
					s32 flag_1 = ((packed_flag & 0b00000011) >> 0);
					if (flag_1 == 3) flag_1 = 0;
					for (s32 j = 0; j < flag_1; j++) {
						part.push_back(src.read<u8>(ofs++, "special anim data fifth part"));
					}
					s32 flag_2 = ((packed_flag & 0b00001100) >> 2);
					if (flag_2 == 3) flag_2 = 0;
					for (s32 j = 0; j < flag_2; j++) {
						part.push_back(src.read<u8>(ofs++, "special anim data fifth part"));
					}
					s32 flag_3 = ((packed_flag & 0b00110000) >> 4);
					if (flag_3 == 3) flag_3 = 0;
					for (s32 j = 0; j <flag_3; j++) {
						part.push_back(src.read<u8>(ofs++, "special anim data fifth part"));
					}
				}
				return part;
			};
			
			frame.special.fifth_part_1 = read_fifth_part(thing_1_count);
			frame.special.fifth_part_2 = read_fifth_part(thing_2_count);
			
			sequence.frames.emplace_back(std::move(frame));
		}
	}
	
	if (seq_header.triggers != 0) {
		s64 trigger_data_ofs = seq_ofs + seq_header.triggers;
		if (game == Game::RAC) {
			trigger_data_ofs = seq_header.triggers;
		} else {
			trigger_data_ofs = seq_ofs + seq_header.triggers;
		}
		sequence.trigger_data = src.read<MobyTriggerData>(trigger_data_ofs, "moby sequence trigger data");
	}
	
	return sequence;
}

static s64 write_moby_sequence(
	OutBuffer dest, const MobySequence& sequence, s64 header_ofs, s32 joint_count, Game game)
{
	dest.pad(0x10);
	s64 seq_header_ofs = dest.alloc<MobySequenceHeader>();
	
	MobySequenceHeader seq_header = {0};
	seq_header.bounding_sphere = Vec4f::pack(sequence.bounding_sphere);
	seq_header.frame_count = sequence.frames.size();
	seq_header.sound_count = sequence.sound_count;
	seq_header.trigger_count = sequence.triggers.size();
	seq_header.unknown_13 = sequence.unknown_13;
	
	s64 frame_pointer_ofs = dest.alloc_multiple<s32>(sequence.frames.size());
	dest.write_multiple(sequence.triggers);
	
	if (sequence.has_special_data) {
		s32 first_part_size = 0;
		s32 second_part_size = 0;
		s32 third_part_size = 0;
		
		if (sequence.frames.size() >= 1) {
			const MobyFrame& frame = sequence.frames[0];
			first_part_size = (s32) frame.special.first_part.size();
			second_part_size = (s32) frame.special.second_part.size();
			third_part_size = (s32) frame.special.third_part.size();
		}
		
		u32 second_part_ofs = first_part_size;
		u32 third_part_ofs = second_part_ofs + second_part_size;
		u32 fourth_part_ofs = third_part_ofs + third_part_size;
		verify(second_part_ofs <= 0b1111111111, "Animation frame too big.");
		verify(third_part_ofs <= 0b11111111111, "Animation frame too big.");
		verify(fourth_part_ofs <= 0b11111111111, "Animation frame too big.");
		dest.write<u32>(second_part_ofs | (third_part_ofs << 10) | (fourth_part_ofs << 21));
		
		dest.pad(0x2);
		dest.write_multiple(sequence.special.joint_data);
		
		verify(sequence.special.thing_1.size() < 256, "Animation frame too big.");
		verify(sequence.special.thing_2.size() < 256, "Animation frame too big.");
		dest.write<u8>(sequence.special.thing_1.size());
		dest.write<u8>(sequence.special.thing_2.size());
		dest.write_multiple(sequence.special.thing_1);
		dest.write_multiple(sequence.special.thing_2);
	}
	
	if (sequence.trigger_data.has_value()) {
		if (game == Game::RAC) {
			dest.pad(0x10);
		}
		s64 trigger_data_ofs = dest.write(*sequence.trigger_data);
		if (game == Game::RAC) {
			seq_header.triggers = trigger_data_ofs - header_ofs;
		} else {
			seq_header.triggers = trigger_data_ofs - seq_header_ofs;
		}
	}
	seq_header.animation_info = sequence.animation_info;
	
	for (const MobyFrame& frame : sequence.frames) {
		if (!sequence.has_special_data) {
			s32 data_size_bytes = (joint_count + frame.regular.thing_1.size() + frame.regular.thing_2.size()) * 8;
			while (data_size_bytes % 0x10 != 0) data_size_bytes++;
			
			Rac123MobyFrameHeader frame_header = {0};
			frame_header.unknown_0 = frame.regular.unknown_0;
			frame_header.unknown_4 = frame.regular.unknown_4;
			verify(data_size_bytes / 0x10 < 65536, "Frame data too big.");
			frame_header.data_size_qwords = data_size_bytes / 0x10;
			frame_header.joint_data_size = joint_count * 8;
			verify(frame.regular.thing_1.size() < 65536, "Frame data too big.");
			frame_header.thing_1_count = (u16) frame.regular.thing_1.size();
			frame_header.unknown_c = frame.regular.unknown_c;
			verify(frame.regular.thing_2.size() < 65536, "Frame data too big.");
			frame_header.thing_2_count = (u16) frame.regular.thing_2.size();
			dest.pad(0x10);
			dest.write<u32>(frame_pointer_ofs, (dest.write(frame_header) - header_ofs));
			dest.write_multiple(frame.regular.joint_data);
			dest.write_multiple(frame.regular.thing_1);
			dest.write_multiple(frame.regular.thing_2);
		} else {
			dest.pad(0x4);
			dest.write<u32>(frame_pointer_ofs, (dest.tell() - header_ofs) | 0xf0000000);
			
			dest.write<u16>(frame.special.inverse_unknown_0);
			dest.write<u16>(frame.special.unknown_4);
			dest.write_multiple(frame.special.first_part);
			dest.write_multiple(frame.special.second_part);
			dest.write_multiple(frame.special.third_part);
			dest.write_multiple(frame.special.fourth_part);
			dest.write_multiple(frame.special.fifth_part_1);
			dest.write_multiple(frame.special.fifth_part_2);
		}
		frame_pointer_ofs += 4;
	}
	dest.write(seq_header_ofs, seq_header);
	
	return seq_header_ofs;
}

static MobySequence read_dl_moby_sequence(Buffer src, s64 seq_ofs, s32 joint_count)
{
	MobySequenceHeader header = src.read<MobySequenceHeader>(seq_ofs, "moby sequence header");
	s32 data_ofs = src.read<s32>(seq_ofs + 0x1c, "moby sequence data offset");
	auto data_header = src.read<DeadlockedMobySequenceDataHeader>(seq_ofs + data_ofs, "moby sequence data header");
	MobySequence seq;
	seq.deadlocked_data = src.read_bytes(seq_ofs, data_ofs + data_header.spr_dma_qwc * 16, "moby sequence");
	return seq;
}

static s64 write_dl_moby_sequence(
	OutBuffer dest, const MobySequence& sequence, s64 header_ofs, s32 joint_count)
{
	s64 seq_ofs = dest.tell();
	dest.write_multiple(sequence.deadlocked_data);
	return seq_ofs;
}
