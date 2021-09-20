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

#ifndef WAD_MOBY_H
#define WAD_MOBY_H

#include "../core/buffer.h"

struct MobySubMesh {
	
};

struct MobyFrame {
	f32 unknown_0;
	u16 unknown_4;
	u32 unknown_8;
	u16 unknown_c;
	u16 unknown_d;
	std::vector<u8> data;
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

struct MobySequence {
	glm::vec4 bounding_sphere;
	std::vector<MobyFrame> frames;
	std::vector<u32> triggers;
	Opt<MobyTriggerData> trigger_data;
};

packed_struct(MobySubMeshEntry,
	u32 vif_list_offset;
	u16 vif_list_size; // In 16 byte units.
	u16 vif_list_texture_unpack_offset; // No third UNPACK if zero.
	u32 vertex_offset;
	u8 vertex_data_size; // Includes header, in 16 byte units.
	u8 unknown_d; // unknown_d == (0xf + transfer_vertex_count * 6) / 0x10
	u8 unknown_e; // unknown_e == (3 + transfer_vertex_count) / 4
	u8 transfer_vertex_count; // Number of vertices sent to VU1.
)

struct MobyCollision {
	u16 unknown_0;
	u16 unknown_2;
	std::vector<u8> first_part;
	std::vector<u8> second_part;
	std::vector<u8> third_part;
};

struct MobyClassData {
	glm::vec4 bounding_sphere;
	std::vector<MobySequence> sequences;
	std::vector<MobySubMeshEntry> submesh_entries; // Temporary.
	Opt<MobyCollision> collision;
	std::vector<glm::mat4> skeleton;
};

packed_struct(MobyClassHeader,
	/* 0x00 */ s32 submesh_table_offset;
	/* 0x04 */ u8 submesh_count_0;
	/* 0x05 */ u8 submesh_count_1;
	/* 0x06 */ u8 unknown_6;
	/* 0x07 */ u8 unknown_7;
	/* 0x08 */ u8 joint_count;
	/* 0x09 */ u8 unknown_9;
	/* 0x0a */ u8 unknown_a;
	/* 0x0b */ u8 unknown_b;
	/* 0x0c */ u8 sequence_count;
	/* 0x0d */ u8 unknown_d;
	/* 0x0e */ u8 unknown_e;
	/* 0x0f */ u8 unknown_f;
	/* 0x00 */ s32 collision;
	/* 0x04 */ s32 skeleton;
	/* 0x08 */ s32 unknown_18;
	/* 0x0c */ s32 unknown_1c;
	/* 0x00 */ s32 unknown_20;
	/* 0x24 */ s32 unknown_24;
	/* 0x28 */ s32 unknown_28;
	/* 0x2c */ s32 unknown_2c;
	/* 0x30 */ Vec4f bounding_sphere;
	/* 0x40 */ s32 unknown_40;
	/* 0x44 */ s32 unknown_44;
)
static_assert(sizeof(MobyClassHeader) == 0x48);

packed_struct(MobySequenceHeader,
	/* 0x00 */ Vec4f bounding_sphere;
	/* 0x10 */ u8 frame_count;
	/* 0x11 */ s8 sound_count;
	/* 0x12 */ u8 trigger_count;
	/* 0x13 */ s8 pad;
	/* 0x14 */ u32 triggers;
	/* 0x18 */ u32 animation_info;
)

packed_struct(MobyFrameHeader,
	/* 0x0 */ f32 unknown_0;
	/* 0x4 */ u16 unknown_4;
	/* 0x4 */ u16 count;
	/* 0x8 */ u32 unknown_8;
	/* 0xc */ u16 unknown_c;
	/* 0xd */ u16 unknown_d;
)

packed_struct(MobyCollisionHeader,
	/* 0x0 */ s16 unknown_0;
	/* 0x2 */ s16 unknown_2;
	/* 0x4 */ s32 first_part_count;
	/* 0x8 */ s32 third_part_size;
	/* 0xc */ s32 second_part_size;
)

MobyClassData read_moby_class(Buffer src);
void write_moby_class(OutBuffer dest, const MobyClassData& moby);

#endif
