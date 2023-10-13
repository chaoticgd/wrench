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

#ifndef ENGINE_MOBY_PACKET_H
#define ENGINE_MOBY_PACKET_H

#include <core/mesh.h>
#include <core/buffer.h>
#include <core/collada.h>
#include <engine/gif.h>
#include <engine/moby_vertex.h>
#include <engine/moby_skinning.h>

namespace MOBY {

packed_struct(MobyTexCoord,
	/* 0x0 */ s16 s;
	/* 0x2 */ s16 t;
)

packed_struct(MobyIndexHeader, // Second UNPACK header.
	/* 0x0 */ u8 unknown_0;
	/* 0x1 */ u8 texture_unpack_offset_quadwords; // Offset of texture data relative to decompressed index buffer in VU mem.
	/* 0x2 */ u8 secret_index;
	/* 0x3 */ u8 pad;
	// Indices directly follow.
)

packed_struct(MobyTexturePrimitive,
	/* 0x00 */ GifAdData12 d1_tex1_1;
	/* 0x0c */ u32 super_secret_index_1; // The VU1 microcode reads extra indices from these fields.
	/* 0x10 */ GifAdData12 d2_clamp_1;
	/* 0x1c */ u32 super_secret_index_2;
	/* 0x20 */ GifAdData12 d3_tex0_1;
	/* 0x2c */ u32 super_secret_index_3;
	/* 0x30 */ GifAdData12 d4_miptbp1_1;
	/* 0x3c */ u32 super_secret_index_4;
)

enum MobySpecialTextureIndex {
	MOBY_TEX_NONE = -1,
	MOBY_TEX_CHROME = -2,
	MOBY_TEX_GLASS = -3
};

struct MobyPacketBase {
	std::vector<u8> indices;
	std::vector<u8> secret_indices;
	std::vector<MobyTexturePrimitive> textures;
	u8 index_header_first_byte = 0xff;
};

struct MobyPacket : MobyPacketBase {
	std::vector<MobyTexCoord> sts;
	std::vector<Vertex> vertices;
	std::vector<u16> duplicate_vertices;
	u16 unknown_e = 0;
	std::vector<u8> unknown_e_data;
};

struct MobyPacketLowLevel {
	const MobyPacket& high_level;
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

struct MobyMetalSubMesh : MobyPacketBase {
	std::vector<MobyMetalVertex> vertices;
	u32 unknown_4;
	u32 unknown_8;
	u32 unknown_c;
};

packed_struct(MobyPacketEntry,
	/* 0x0 */ u32 vif_list_offset;
	/* 0x4 */ u16 vif_list_size; // In 16 byte units.
	/* 0x6 */ u16 vif_list_texture_unpack_offset; // No third UNPACK if zero.
	/* 0x8 */ u32 vertex_offset;
	/* 0xc */ u8 vertex_data_size; // Includes header, in 16 byte units.
	/* 0xd */ u8 unknown_d; // unknown_d == (0xf + transfer_vertex_count * 6) / 0x10
	/* 0xe */ u8 unknown_e; // unknown_e == (3 + transfer_vertex_count) / 4
	/* 0xf */ u8 transfer_vertex_count; // Number of vertices sent to VU1.
)

packed_struct(MobyBangleHeader,
	u8 packet_begin;
	u8 packet_count;
	u8 unknown_2;
	u8 unknown_3;
)

packed_struct(MobyBangleIndices,
	u8 high_lod_packet_begin;
	u8 high_lod_packet_count;
	u8 low_lod_packet_begin;
	u8 low_lod_packet_count;
)

packed_struct(MobyVec4,
	s16 x;
	s16 y;
	s16 z;
	s16 w;
)

struct MobyBangle {
	std::vector<MobyPacket> high_lod;
	std::vector<MobyPacket> low_lod;
	MobyVec4 vectors[2];
};

using GifUsageTable = std::vector<MobyGifUsage>;

std::vector<MobyPacket> read_moby_packets(Buffer src, s64 table_ofs, s64 count, f32 scale, bool animated, MobyFormat format);
void write_moby_packets(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const MobyPacket* packets_in, size_t packet_count, f32 scale, MobyFormat format, s64 class_header_ofs);

std::vector<MobyMetalSubMesh> read_moby_metal_packets(Buffer src, s64 table_ofs, s64 count);
void write_moby_metal_packets(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& packets, s64 class_header_ofs);

void map_indices(MobyPacket& packet, const std::vector<size_t>& index_mapping);
std::vector<MobyPacket> build_moby_packets(const Mesh& mesh, const std::vector<ColladaMaterial>& materials);

}

#endif
