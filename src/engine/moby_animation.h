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

#ifndef ENGINE_MOBY_ANIMATION_H
#define ENGINE_MOBY_ANIMATION_H

#include <core/buffer.h>
#include <core/build_config.h>
#include <engine/basic_types.h>

struct MobyFrame
{
	struct {
		f32 unknown_0;
		u16 unknown_4;
		u16 unknown_c;
		std::vector<u64> joint_data;
		std::vector<u64> thing_1;
		std::vector<u64> thing_2;
	} regular;
	struct {
		u16 inverse_unknown_0;
		u16 unknown_4;
		std::vector<u8> first_part;
		std::vector<u8> second_part;
		std::vector<u8> third_part;
		std::vector<u8> fourth_part;
		std::vector<u8> fifth_part_1;
		std::vector<u8> fifth_part_2;
	} special;
};

packed_struct(MobyTriggerData,
	/* 0x00 */ u32 unknown_0;
	/* 0x04 */ u32 unknown_4;
	/* 0x08 */ u32 unknown_8;
	/* 0x0c */ u32 unknown_c;
	/* 0x10 */ u32 unknown_10;
	/* 0x14 */ u32 unknown_14;
	/* 0x18 */ u32 unknown_18;
	/* 0x1c */ u32 unknown_1c;
)

struct MobySequence
{
	glm::vec4 bounding_sphere;
	std::vector<MobyFrame> frames;
	std::vector<u32> triggers;
	Opt<MobyTriggerData> trigger_data;
	s32 animation_info = 0;
	u8 sound_count = 0xff;
	u8 unknown_13 = 0;
	bool has_special_data = false;
	struct {
		std::vector<u16> joint_data;
		std::vector<u64> thing_1;
		std::vector<u64> thing_2;
	} special;
	std::vector<u8> deadlocked_data;
};

packed_struct(MobySequenceHeader,
	/* 0x00 */ Vec4f bounding_sphere;
	/* 0x10 */ u8 frame_count;
	/* 0x11 */ u8 sound_count;
	/* 0x12 */ u8 trigger_count;
	/* 0x13 */ u8 unknown_13;
	/* 0x14 */ u32 triggers;
	/* 0x18 */ u32 animation_info;
)

packed_struct(Rac123MobyFrameHeader,
	/* 0x0 */ f32 unknown_0;
	/* 0x4 */ u16 unknown_4;
	/* 0x6 */ u16 data_size_qwords;
	/* 0x8 */ u16 joint_data_size;
	/* 0xa */ u16 thing_1_count;
	/* 0xc */ u16 unknown_c;
	/* 0xe */ u16 thing_2_count;
)

packed_struct(DeadlockedMobySequenceDataHeader,
	/* 0x0 */ u8 unknown_0;
	/* 0x1 */ u8 spr_dma_qwc;
	/* 0x2 */ u8 unknown_2;
	/* 0x3 */ u8 unknown_3;
	/* 0x4 */ u32 unknown_4;
	/* 0x8 */ u32 unknown_8;
	/* 0xc */ u32 unknown_c;
)

std::vector<Opt<MobySequence>> read_moby_sequences(
	Buffer src, s64 sequence_count, s32 joint_count, Game game);
void write_moby_sequences(
	OutBuffer dest,
	const std::vector<Opt<MobySequence>>& sequences,
	s64 class_header_ofs,
	s64 list_ofs,
	s32 joint_count,
	Game game);

#endif
