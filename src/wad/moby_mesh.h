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

#ifndef WAD_MOBY_MESH_H
#define WAD_MOBY_MESH_H

#include <core/util.h>
#include <core/buffer.h>
#include <core/collada.h>

#define MOBY_EXPORT_SUBMESHES_SEPERATELY false
#define NO_SUBMESH_FILTER -1

#define WRENCH_PI 3.14159265358979323846

packed_struct(MobyTexCoord,
	/* 0x0 */ s16 s;
	/* 0x2 */ s16 t;
)

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

packed_struct(MobyIndexHeader, // Second UNPACK header.
	/* 0x0 */ u8 unknown_0;
	/* 0x1 */ u8 texture_unpack_offset_quadwords; // Offset of texture data relative to decompressed index buffer in VU mem.
	/* 0x2 */ u8 secret_index;
	/* 0x3 */ u8 pad;
	// Indices directly follow.
)

packed_struct(GsAdData,
	/* 0x0 */ s32 data_lo;
	/* 0x4 */ s32 data_hi;
	/* 0x8 */ u8 address;
	/* 0x9 */ u8 pad_9;
	/* 0xa */ u16 pad_a;
	/* 0xc */ u32 super_secret_index; // The VU1 microcode reads extra indices from here.
)

packed_struct(MobyTexturePrimitive,
	/* 0x00 */ GsAdData d1_xyzf2;
	/* 0x10 */ GsAdData d2_clamp;
	/* 0x20 */ GsAdData d3_tex0;
	/* 0x30 */ GsAdData d4_xyzf2;
)

enum MobySpecialTextureIndex {
	MOBY_TEX_NONE = -1,
	MOBY_TEX_CHROME = -2,
	MOBY_TEX_GLASS = -3
};

struct MobySubMeshBase {
	std::vector<u8> indices;
	std::vector<u8> secret_indices;
	std::vector<MobyTexturePrimitive> textures;
	u8 index_header_first_byte = 0xff;
};

struct MobySubMesh : MobySubMeshBase {
	std::vector<MobyTexCoord> sts;
	std::vector<Vertex> vertices;
	std::vector<u16> duplicate_vertices;
	u16 unknown_e = 0;
	std::vector<u8> unknown_e_data;
};

packed_struct(MobyMatrixTransfer,
	u8 spr_joint_index;
	u8 vu0_dest_addr;
)

struct MobySubMeshLowLevel {
	const MobySubMesh& high_level;
	std::vector<MobyMatrixTransfer> preloop_matrix_transfers;
	s32 two_way_blend_vertex_count = 0;
	s32 three_way_blend_vertex_count = 0;
	s32 main_vertex_count = 0;
	std::vector<MobyVertex> vertices;
	std::vector<size_t> index_mapping;
};

packed_struct(MobyMetalVertex,
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

struct MobyMetalSubMesh : MobySubMeshBase {
	std::vector<MobyMetalVertex> vertices;
	u32 unknown_4;
	u32 unknown_8;
	u32 unknown_c;
};

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

packed_struct(MobyGifUsageTableEntry,
	u8 texture_indices[12];
	u32 offset_and_terminator; // High byte is 0x80 => Last entry in the table.
)

// Note: R&C2 has some R&C1-format mobies.
enum class MobyFormat {
	RAC1, RAC2, RAC3DL
};

packed_struct(MobyBangle,
	u8 submesh_begin;
	u8 submesh_count;
	u8 unknown_2;
	u8 unknown_3;
)

packed_struct(MobyVertexPosition,
	s16 x;
	s16 y;
	s16 z;
	s16 w;
)

struct MobyBangles {
	std::vector<MobyBangle> bangles;
	std::vector<MobyVertexPosition> vertices;
	std::vector<MobySubMesh> submeshes;
};

// moby_mesh_importer.cpp
std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, f32 scale, s32 joint_count, MobyFormat format);
std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count);
Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, s32 submesh_filter);
void map_indices(MobySubMesh& submesh, const std::vector<size_t>& index_mapping);

// moby_mesh_exporter.cpp
using GifUsageTable = std::vector<MobyGifUsageTableEntry>;
void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const MobySubMesh* submeshes_in, size_t submesh_count, f32 scale, MobyFormat format, s64 class_header_ofs);
void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes, s64 class_header_ofs);
void write_moby_bangle_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const MobyBangles& bangles, f32 scale, MobyFormat format, s64 class_header_ofs);
std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials);

#endif
