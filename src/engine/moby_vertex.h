/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef ENGINE_MOBY_VERTEX_H
#define ENGINE_MOBY_VERTEX_H

#include <core/mesh.h>
#include <core/buffer.h>

namespace MOBY {

packed_struct(MobyVertex,
	packed_nested_anon_union(
		packed_nested_struct(v,
			packed_nested_anon_union(
				packed_nested_struct(two_way_blend,
					/* 0x0 */ u16 low_halfword; // [0..8] = vertex_index, [9..15] = spr_joint_index
					/* 0x2 */ u8 vu0_matrix_load_addr_1;
					/* 0x3 */ u8 vu0_matrix_load_addr_2;
					/* 0x4 */ u8 weight_1;
					/* 0x5 */ u8 weight_2;
					/* 0x6 */ u8 vu0_transferred_matrix_store_addr;
					/* 0x7 */ u8 vu0_blended_matrix_store_addr;
				)
				packed_nested_struct(three_way_blend,
					/* 0x0 */ u16 low_halfword; // [0..8] = vertex_index, [9..15] = vu0_matrix_load_addr_3/2
					/* 0x2 */ u8 vu0_matrix_load_addr_1;
					/* 0x3 */ u8 vu0_matrix_load_addr_2;
					/* 0x4 */ u8 weight_1;
					/* 0x5 */ u8 weight_2;
					/* 0x6 */ u8 weight_3;
					/* 0x7 */ u8 vu0_blended_matrix_store_addr;
				)
				packed_nested_struct(regular,
					/* 0x0 */ u16 low_halfword; // [0..8] = vertex_index, [9..15] = spr_joint_index
					/* 0x2 */ u8 vu0_matrix_load_addr; // The load is done after the store.
					/* 0x3 */ u8 vu0_transferred_matrix_store_addr;
					/* 0x4 */ u8 unused_4;
					/* 0x5 */ u8 unused_5;
					/* 0x6 */ u8 unused_6;
					/* 0x7 */ u8 unused_7;
				)
				packed_nested_struct(i, // This is just to fish out the vertex index.
					/* 0x0 */ u16 low_halfword; // [0..8] = vertex_index
				)
				packed_nested_struct(register_names, // This is just for cross-referencing with the disassembly.
					/* 0x0 */ u8 vf3x;
					/* 0x1 */ u8 vf3y;
					/* 0x2 */ u8 vf3z;
					/* 0x3 */ u8 vf3w;
					/* 0x4 */ u8 vf4x;
					/* 0x5 */ u8 vf4y;
					/* 0x6 */ u8 vf4z;
					/* 0x7 */ u8 vf4w;
				)
			)
			/* 0x8 */ u8 normal_angle_azimuth;
			/* 0x9 */ u8 normal_angle_elevation;
			/* 0xa */ s16 x;
			/* 0xc */ s16 y;
			/* 0xe */ s16 z;
		)
		packed_nested_struct(trailing,
			/* 0x0 */ u16 vertex_index;
			/* 0x2 */ u8 unused_2;
			/* 0x3 */ u8 unused_3;
			/* 0x4 */ u16 vertex_indices[6]; // Only populated for the final vertex.
		)
	)
)
static_assert(sizeof(MobyVertex) == 0x10);

packed_struct(MobyVertexTableHeaderRac1,
	/* 0x00 */ u32 matrix_transfer_count;
	/* 0x04 */ u32 two_way_blend_vertex_count;
	/* 0x08 */ u32 three_way_blend_vertex_count;
	/* 0x0c */ u32 main_vertex_count;
	/* 0x10 */ u32 duplicate_vertex_count;
	/* 0x14 */ u32 transfer_vertex_count; // transfer_vertex_count == two_way_blend_vertex_count + three_way_blend_vertex_count + main_vertex_count + duplicate_vertex_count
	/* 0x18 */ u32 vertex_table_offset;
	/* 0x1c */ u32 unknown_e;
)

packed_struct(MobyVertexTableHeaderRac23DL,
	/* 0x0 */ u16 matrix_transfer_count;
	/* 0x2 */ u16 two_way_blend_vertex_count;
	/* 0x4 */ u16 three_way_blend_vertex_count;
	/* 0x6 */ u16 main_vertex_count;
	/* 0x8 */ u16 duplicate_vertex_count;
	/* 0xa */ u16 transfer_vertex_count; // transfer_vertex_count == two_way_blend_vertex_count + three_way_blend_vertex_count + main_vertex_count + duplicate_vertex_count
	/* 0xc */ u16 vertex_table_offset;
	/* 0xe */ u16 unknown_e;
)

packed_struct(MobyMetalVertexTableHeader,
	/* 0x0 */ s32 vertex_count;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 unknown_8;
	/* 0xc */ s32 unknown_c;
)

packed_struct(MobyGifUsage,
	u8 texture_indices[12];
	u32 offset_and_terminator; // High byte is 0x80 => Last entry in the table.
)

// Note: R&C2 has some R&C1-format mobies.
enum class MobyFormat {
	RAC1, RAC2, RAC3DL
};

struct MobyPacket;
struct MobyPacketEntry;
struct MobyPacketLowLevel;

std::vector<MobyVertex> read_vertices(Buffer src, const MobyPacketEntry& entry, const MobyVertexTableHeaderRac1& header, MobyFormat format);
MobyVertexTableHeaderRac1 write_vertices(OutBuffer& dest, const MobyPacket& packet, const MobyPacketLowLevel& low, MobyFormat format);

struct VU0MatrixAllocator;
struct MatrixLivenessInfo;

std::vector<Vertex> unpack_vertices(const MobyPacketLowLevel& src, Opt<SkinAttributes> blend_buffer[64], f32 scale);
MobyPacketLowLevel pack_vertices(s32 smi, const MobyPacket& packet, VU0MatrixAllocator& mat_alloc, const std::vector<MatrixLivenessInfo>& liveness, f32 scale);

}

#endif
