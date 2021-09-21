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
#include "vif.h"

packed_struct(MobyTexCoord,
	s16 s;
	s16 t;
)

packed_struct(MobyVertex,
	/* 0x0 */ u8 unknown_0;
	/* 0x1 */ u8 unknown_1;
	/* 0x2 */ u8 unknown_2;
	/* 0x3 */ u8 unknown_3;
	/* 0x4 */ u8 unknown_4;
	/* 0x5 */ u8 unknown_5;
	/* 0x6 */ u8 unknown_6;
	/* 0x7 */ u8 unknown_7;
	/* 0x8 */ u8 unknown_8;
	/* 0x9 */ u8 unknown_9;
	/* 0xa */ s16 x;
	/* 0xc */ s16 y;
	/* 0xe */ s16 z;
)

struct MobySubMesh {
	std::vector<MobyTexCoord> sts;
	std::vector<u8> indices;
	std::vector<s32> texture_indices;
	std::vector<u16> unknowns;
	std::vector<u16> vertices_8;
	std::vector<MobyVertex> vertices_2;
	std::vector<MobyVertex> vertices_4;
	std::vector<MobyVertex> main_vertices;
	std::vector<MobyVertex> trailing_vertices;
	u16 unknown_e;
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

struct MobyCollision {
	u16 unknown_0;
	u16 unknown_2;
	std::vector<u8> first_part;
	std::vector<u8> second_part;
	std::vector<u8> third_part;
};

packed_struct(MobySoundDef,
	/* 0x00 */ f32 min_range;
	/* 0x04 */ f32 max_range;
	/* 0x08 */ s32 min_volume;
	/* 0x0c */ s32 max_volume;
	/* 0x10 */ s32 min_pitch;
	/* 0x14 */ s32 max_pitch;
	/* 0x18 */ u8 loop;
	/* 0x19 */ u8 flags;
	/* 0x1a */ s16 index;
	/* 0x1c */ s32 bank_index;
)

struct MobyClassData {
	std::vector<MobySubMesh> submeshes_1;
	std::vector<MobySubMesh> submeshes_2;
	std::vector<MobySequence> sequences;
	Opt<MobyCollision> collision;
	std::vector<glm::mat4> skeleton;
	std::vector<u8> common_trans;
	std::vector<u8> anim_joints;
	std::vector<MobySoundDef> sound_defs;
	u8 unknown_6;
	u8 unknown_7;
	u8 unknown_9;
	u8 lod_trans;
	u8 shadow;
	f32 scale;
	u8 bangles;
	u8 mip_dist;
	s16 corncob;
	glm::vec4 bounding_sphere;
	s32 glow_rgba;
	s16 mode_bits;
	u8 type;
	u8 mode_bits2;
};

packed_struct(MobyClassHeader,
	/* 0x00 */ s32 submesh_table_offset;
	/* 0x04 */ u8 submesh_count_1;
	/* 0x05 */ u8 submesh_count_2;
	/* 0x06 */ u8 unknown_6;
	/* 0x07 */ u8 unknown_7;
	/* 0x08 */ u8 joint_count;
	/* 0x09 */ u8 unknown_9;
	/* 0x0a */ u8 unknown_a;
	/* 0x0b */ u8 unknown_b;
	/* 0x0c */ u8 sequence_count;
	/* 0x0d */ u8 sound_count;
	/* 0x0e */ u8 lod_trans;
	/* 0x0f */ u8 shadow;
	/* 0x10 */ s32 collision;
	/* 0x14 */ s32 skeleton;
	/* 0x18 */ s32 common_trans;
	/* 0x1c */ s32 anim_joints;
	/* 0x20 */ s32 gif_usage;
	/* 0x24 */ f32 scale;
	/* 0x28 */ s32 sound_defs;
	/* 0x2c */ u8 bangles;
	/* 0x2d */ u8 mip_dist;
	/* 0x2e */ s16 corncob;
	/* 0x30 */ Vec4f bounding_sphere;
	/* 0x40 */ s32 glow_rgba;
	/* 0x44 */ s16 mode_bits;
	/* 0x46 */ u8 type;
	/* 0x47 */ u8 mode_bits2;
)
static_assert(sizeof(MobyClassHeader) == 0x48);

packed_struct(MobySubMeshEntry,
	/* 0x0 */ u32 vif_list_offset;
	/* 0x4 */ u16 vif_list_size; // In 16 byte units.
	/* 0x6 */ u16 vif_list_texture_unpack_offset; // No third UNPACK if zero.
	/* 0x8 */ u32 vertex_offset;
	/* 0xc */ u8 vertex_data_size; // Includes header, in 16 byte units.
	/* 0xd */ u8 unknown_d; // unknown_d == (0xf + transfer_vertex_count * 6) / 0x10
	/* 0xe */ u8 unknown_e; // unknown_e == (3 + transfer_vertex_count) / 4
	/* 0xf */ u8 transfer_vertex_count; // Number of vertices sent to VU1.
)

packed_struct(MobyVertexTableHeader,
	/* 0x0 */ u16 unknown_count_0;
	/* 0x2 */ u16 vertex_count_2;
	/* 0x4 */ u16 vertex_count_4;
	/* 0x6 */ u16 main_vertex_count;
	/* 0x8 */ u16 vertex_count_8;
	/* 0xa */ u16 transfer_vertex_count; // transfer_vertex_count == vertex_count_2 + vertex_count_4 + main_vertex_count + vertex_count_8
	/* 0xc */ u16 vertex_table_offset;
	/* 0xe */ u16 unknown_e;
)

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

packed_struct(MobyGifUsageTableEntry,
	u8 texture_indices[12];
	u32 offset_and_terminator; // High byte is 0x80 => Last entry in the table.
)

MobyClassData read_moby_class(Buffer src);
void write_moby_class(OutBuffer dest, const MobyClassData& moby);

#endif
