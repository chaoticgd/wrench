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

#ifndef ENGINE_SHRUB_H
#define ENGINE_SHRUB_H

#include <variant>

#include <core/buffer.h>
#include <core/gltf.h>
#include <core/texture.h>
#include <core/tristrip.h>
#include <core/build_config.h>
#include <engine/basic_types.h>
#include <engine/gif.h>

packed_struct(ShrubPacketEntry,
	/* 0x0 */ s32 offset;
	/* 0x4 */ s32 size;
)

packed_struct(ShrubBillboard,
	/* 0x00 */ f32 fade_distance;
	/* 0x04 */ f32 width;
	/* 0x08 */ f32 height;
	/* 0x0c */ f32 z_ofs;
	/* 0x10 */ GifAdData16 d1_tex1_1;
	/* 0x20 */ GifAdData16 d2_tex0_1;
	/* 0x30 */ GifAdData16 d3_miptbp1_1;
)

struct ShrubBillboardInfo
{
	f32 fade_distance;
	f32 width;
	f32 height;
	f32 z_ofs;
};

packed_struct(ShrubClassHeader,
	/* 0x00 */ Vec4f bounding_sphere;
	/* 0x10 */ f32 mip_distance;
	/* 0x14 */ u16 mode_bits;
	/* 0x16 */ s16 instance_count;
	/* 0x18 */ s32 instances_pointer;
	/* 0x1c */ s32 billboard_offset;
	/* 0x20 */ f32 scale;
	/* 0x24 */ s16 o_class;
	/* 0x26 */ s16 s_class;
	/* 0x28 */ s16 packet_count;
	/* 0x2a */ s16 pad_2a;
	/* 0x2c */ s32 normals_offset;
	/* 0x30 */ s32 pad_30;
	/* 0x34 */ s16 drawn_count;
	/* 0x36 */ s16 scis_count;
	/* 0x38 */ s16 billboard_count;
	/* 0x3a */ s16 pad_3a[3];
)

packed_struct(ShrubVertexPart1,
	/* 0x00 */ s16 x;
	/* 0x02 */ s16 y;
	/* 0x04 */ s16 z;
	/* 0x06 */ s16 gs_packet_offset;
)

packed_struct(ShrubVertexPart2,
	/* 0x00 */ s16 s;
	/* 0x02 */ s16 t;
	/* 0x04 */ s16 h;
	/* 0x06 */ s16 n_and_stop_cond; // If this is negative the strip ends.
)

packed_struct(ShrubPacketHeader,
	/* 0x0 */ s32 texture_count;
	/* 0x4 */ s32 gif_tag_count;
	/* 0x8 */ s32 vertex_count;
	/* 0xc */ s32 vertex_offset;
)

packed_struct(ShrubVertexGifTag,
	/* 0x0 */ GifTag12 tag;
	/* 0xc */ s32 gs_packet_offset;
)

packed_struct(ShrubTexturePrimitive,
	/* 0x00 */ GifAdData12 d1_tex1_1;
	/* 0x0c */ s32 gs_packet_offset;
	/* 0x10 */ GifAdData16 d2_clamp_1;
	/* 0x20 */ GifAdData16 d3_miptbp1_1;
	/* 0x30 */ GifAdData16 d4_tex0_1;
)

struct ShrubVertex
{
	s16 x;
	s16 y;
	s16 z;
	s16 s;
	s16 t;
	s16 h;
	s16 n;
};

struct ShrubVertexPrimitive
{
	GeometryType type;
	std::vector<ShrubVertex> vertices;
};

using ShrubPrimitive = std::variant<ShrubTexturePrimitive, ShrubVertexPrimitive>;

struct ShrubPacket
{
	std::vector<ShrubPrimitive> primitives;
};

packed_struct(ShrubNormal,
	s16 x;
	s16 y;
	s16 z;
	s16 pad = 0;
)

struct ShrubClass
{
	Vec4f bounding_sphere;
	f32 mip_distance;
	u16 mode_bits;
	f32 scale;
	s16 o_class;
	std::vector<ShrubPacket> packets;
	Opt<ShrubBillboard> billboard;
	std::vector<ShrubNormal> normals;
};

ShrubClass read_shrub_class(Buffer src);
void write_shrub_class(OutBuffer dest, const ShrubClass& shrub);

GLTF::Mesh recover_shrub_class(const ShrubClass& shrub);
ShrubClass build_shrub_class(
	const GLTF::Mesh& mesh,
	const std::vector<Material>& materials,
	f32 mip_distance,
	u16 mode_bits,
	s16 o_class,
	Opt<ShrubBillboardInfo> billboard);

#endif
