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

#ifndef ENGINE_TIE_H
#define ENGINE_TIE_H

#include <variant>
#include <core/vif.h>
#include <core/buffer.h>
#include <core/collada.h>
#include <core/build_config.h>
#include <engine/basic_types.h>
#include <engine/gif.h>

packed_struct(TieLodHeader,
	/* 0x0 */ s16 vert_count;
	/* 0x2 */ s16 tri_count;
	/* 0x4 */ s16 strip_count;
	/* 0x6 */ s16 pad;
)

packed_struct(RacTieClassHeader,
	/* 0x00 */ s32 packets[3];
	/* 0x0c */ u32 vert_normals;
	/* 0x10 */ f32 near_dist;
	/* 0x14 */ f32 mid_dist;
	/* 0x18 */ f32 far_dist;
	/* 0x1c */ u32 unknown_1c;
	/* 0x20 */ u8 packet_count[3];
	/* 0x23 */ u8 texture_count;
	/* 0x24 */ u32 unknown_24;
	/* 0x28 */ u32 unknown_28;
	/* 0x2c */ u32 ad_gif_ofs;
	/* 0x30 */ Vec4f bsphere;
	/* 0x40 */ f32 scale;
	/* 0x44 */ u32 unknown_44;
	/* 0x48 */ u32 unknown_48;
	/* 0x4c */ u32 unknown_4c;
	/* 0x50 */ u32 unknown_50;
	/* 0x54 */ u32 unknown_54;
	/* 0x58 */ u32 unknown_58;
	/* 0x5c */ u32 unknown_5c;
	/* 0x60 */ u32 unknown_60;
	/* 0x64 */ u32 unknown_64;
	/* 0x68 */ u32 unknown_68;
	/* 0x6c */ u32 unknown_6c;
)

packed_struct(GcUyaDlTieClassHeader,
	/* 0x00 */ s32 packets[3];
	/* 0x0c */ u8 packet_count[3];
	/* 0x0f */ u8 texture_count;
	/* 0x10 */ f32 near_dist;
	/* 0x14 */ f32 mid_dist;
	/* 0x18 */ f32 far_dist;
	/* 0x1c */ s32 ad_gif_ofs;
	/* 0x20 */ s32 instance_index;
	/* 0x24 */ s16 cache_sizes[3];
	/* 0x2a */ s16 rgba_remap_ofs[3];
	/* 0x30 */ s32 ambient_rgbas;
	/* 0x34 */ s32 vert_normals;
	/* 0x38 */ s16 vert_normal_count;
	/* 0x3a */ s16 ambient_size;
	/* 0x3c */ s16 mode_bits;
	/* 0x3e */ s16 instance_count;
	/* 0x40 */ f32 scale;
	/* 0x44 */ s16 o_class;
	/* 0x46 */ s16 t_class;
	/* 0x48 */ f32 mip_dist;
	/* 0x4c */ s32 glow_rgba;
	/* 0x50 */ Vec4f bsphere;
	/* 0x60 */ TieLodHeader lods[3];
	/* 0x78 */ s16 glow_remap_ofs[3];
	/* 0x7e */ s16 pad;
)

packed_struct(TiePacketHeader,
	/* 0x0 */ s32 data;
	/* 0x4 */ u8 shader_count;
	/* 0x5 */ u8 bfc_distance;
	/* 0x6 */ u8 control_count;
	/* 0x7 */ u8 control_size;
	/* 0x8 */ u8 vert_ofs;
	/* 0x9 */ u8 vert_size;
	/* 0xa */ u8 rgba_count;
	/* 0xb */ u8 multipass_ofs;
	/* 0xc */ u8 scissor_ofs;
	/* 0xd */ u8 scissor_size;
	/* 0xe */ u8 nultipass_type;
	/* 0xf */ u8 multipass_uv_size;
)

packed_struct(TieUnpackHeader,
	/* PACK UNPK */
	/* 0x00 0x00 */ u8 unknown_0;
	/* 0x01 0x04 */ u8 unknown_2;
	/* 0x02 0x08 */ u8 unknown_4;
	/* 0x03 0x0c */ u8 strip_count;
	/* 0x04 0x10 */ u8 unknown_8;
	/* 0x05 0x14 */ u8 unknown_a;
	/* 0x06 0x18 */ u8 unknown_c;
	/* 0x07 0x1c */ u8 unknown_e;
	/* 0x08 0x20 */ u8 dinky_vertices_size_plus_four;
	/* 0x09 0x24 */ u8 fat_vertices_size;
	/* 0x0a 0x28 */ u8 unknown_14;
	/* 0x0b 0x2c */ u8 unknown_16;
)

packed_struct(TieStrip,
	/* PACK UNPK */
	/* 0x00 0x00 */ u8 vertex_count;
	/* 0x01 0x04 */ u8 pad_1;
	/* 0x02 0x08 */ u8 gif_tag_offset;
	/* 0x03 0x0c */ u8 rc34_winding_order;
)

packed_struct(TieDinkyVertex,
	/* PACK UNPK */
	/* 0x00 0x00 */ s16 x;
	/* 0x02 0x04 */ s16 y;
	/* 0x04 0x08 */ s16 z;
	/* 0x06 0x0c */ u16 gs_packet_write_ofs;
	/* 0x08 0x10 */ u16 s;
	/* 0x0a 0x14 */ u16 t;
	/* 0x0c 0x18 */ u16 q;
	/* 0x0e 0x20 */ u16 gs_packet_write_ofs_2;
)

packed_struct(TieFatVertex,
	/* PACK UNPK */
	/* 0x00 0x00 */ u16 unknown_0;
	/* 0x02 0x04 */ u16 unknown_2;
	/* 0x04 0x08 */ u16 unknown_4;
	/* 0x06 0x0c */ u16 gs_packet_write_ofs;
	/* 0x08 0x10 */ s16 x;
	/* 0x0a 0x14 */ s16 y;
	/* 0x0c 0x18 */ s16 z;
	/* 0x0e 0x1c */ u16 pad_e;
	/* 0x10 0x20 */ u16 s;
	/* 0x12 0x24 */ u16 t;
	/* 0x14 0x28 */ u16 q;
	/* 0x16 0x2c */ u16 gs_packet_write_ofs_2;
)

struct TiePrimitive
{
	s32 material_index;
	std::vector<TieDinkyVertex> vertices;
	s32 winding_order;
};

struct TiePacket
{
	std::vector<TiePrimitive> primitives;
	std::vector<u8> multipass;
	std::vector<u8> scissor;
};

struct TieLod
{
	std::vector<TiePacket> packets;
};

packed_struct(TieAdGifs,
	/* 0x00 */ GifAdData16 d1_tex0_1;
	/* 0x10 */ GifAdData16 d2_tex1_1;
	/* 0x20 */ GifAdData16 d3_miptbp1_1;
	/* 0x30 */ GifAdData16 d4_clamp_1;
	/* 0x40 */ GifAdData16 d5_miptbp2_1;
)

struct TieClass
{
	TieLod lods[3];
	f32 scale;
	std::vector<TieAdGifs> ad_gifs;
};

TieClass read_tie_class(Buffer src, Game game);
void write_tie_class(OutBuffer dest, const TieClass& tie);
ColladaScene recover_tie_class(const TieClass& tie);

#endif
