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
		packed_nested_struct(epilogue,
			/* 0x0 */ u16 vertex_index;
			/* 0x2 */ u8 unused_2;
			/* 0x3 */ u8 unused_3;
			/* 0x4 */ u16 vertex_indices[6]; // Only populated for the final vertex.
		)
	)
)
static_assert(sizeof(MobyVertex) == 0x10);

packed_struct(MobyGifUsage,
	u8 texture_indices[12];
	u32 offset_and_terminator; // High byte is 0x80 => Last entry in the table.
)

// Note: R&C2 has some R&C1-format mobies.
enum class MobyFormat
{
	RAC1, RAC2, RAC3DL
};

packed_struct(MobyMatrixTransfer,
	u8 spr_joint_index;
	u8 vu0_dest_addr;
)

struct VertexTable
{
	std::vector<MobyMatrixTransfer> preloop_matrix_transfers;
	std::vector<u16> duplicate_vertices;
	s32 two_way_blend_vertex_count = 0;
	s32 three_way_blend_vertex_count = 0;
	s32 main_vertex_count = 0;
	std::vector<MobyVertex> vertices;
	u16 unknown_e = 0;
	std::vector<u8> unknown_e_data;
};

VertexTable read_vertex_table(
	Buffer src,
	s64 header_offset,
	s32 transfer_vertex_count,
	s32 vertex_data_size,
	s32 d,
	s32 e,
	MobyFormat format);
u32 write_vertex_table(OutBuffer& dest, const VertexTable& src, MobyFormat format);

packed_struct(MetalVertex,
	/* 0x0 */ s16 x;
	/* 0x2 */ s16 y;
	/* 0x4 */ s16 z;
	/* 0x6 */ u8 unknown_6;
	/* 0x7 */ u8 unknown_7;
	/* 0x8 */ u8 unknown_8;
	/* 0x9 */ u8 unknown_9;
	/* 0xa */ u8 unknown_a;
	/* 0xb */ u8 unknown_b;
	/* 0xc */ u8 unknown_c;
	/* 0xd */ u8 unknown_d;
	/* 0xe */ u8 unknown_e;
	/* 0xf */ u8 unknown_f;
)

struct MetalVertexTable
{
	std::vector<MetalVertex> vertices;
	u32 unknown_4;
	u32 unknown_8;
	u32 unknown_c;
};

MetalVertexTable read_metal_vertex_table(Buffer src, s64 header_offset);
u32 write_metal_vertex_table(OutBuffer& dest, const MetalVertexTable& src);

struct MobyPacket;
struct MobyPacketLowLevel;
struct VU0MatrixAllocator;
struct MatrixLivenessInfo;

std::vector<Vertex> unpack_vertices(
	const VertexTable& input, Opt<SkinAttributes> blend_cache[64], f32 scale, bool animated);
struct PackVerticesOutput
{
	VertexTable vertex_table;
	std::vector<s32> index_mapping;
};
PackVerticesOutput pack_vertices(
	s32 smi,
	const std::vector<Vertex>& input_vertices,
	VU0MatrixAllocator& mat_alloc,
	const std::vector<MatrixLivenessInfo>& liveness,
	f32 scale);

}

#endif
