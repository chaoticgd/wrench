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

#include <core/buffer.h>
#include <engine/basic_types.h>
#include <engine/gif.h>

packed_struct(TfragTexturePrimitive,
	/* 0x00 */ GifAdData16 d0_tex0_1;
	/* 0x10 */ GifAdData16 d1_tex1_1;
	/* 0x20 */ GifAdData16 d2_clamp_1;
	/* 0x30 */ GifAdData16 d3_miptbp1_1;
	/* 0x40 */ GifAdData16 d4_miptbp2_1;
)

packed_struct(TfragRgba,
	/* 0x0 */ u8 r;
	/* 0x1 */ u8 g;
	/* 0x2 */ u8 b;
	/* 0x3 */ u8 a;
)

packed_struct(TfragVec4i,
	/* 0x0 */ s16 x;
	/* 0x2 */ s16 y;
	/* 0x4 */ s16 z;
	/* 0x6 */ s16 w;
)

packed_struct(TfragCube,
	TfragVec4i vectors[8];
)

struct Tfrag {
	Vec4f bsphere;
	std::vector<u8> lod_2_1;
	std::vector<s8> lod_2_2;
	std::vector<u16> shared_1;
	std::vector<TfragTexturePrimitive> shared_textures;
	std::vector<s16> shared_3;
	std::vector<s16> shared_4;
	std::vector<s8> lod_1_1;
	std::vector<u8> lod_1_2;
	std::vector<u8> lod_01_1;
	std::vector<s16> lod_01_2;
	std::vector<s16> lod_01_3;
	std::vector<s16> lod_0_1;
	std::vector<s8> lod_0_2;
	std::vector<std::vector<s8>> lod_0_3;
	std::vector<s16> lod_0_4;
	std::vector<TfragRgba> rgbas;
	u8 lod_2_rgba_count;
	u8 lod_1_rgba_count;
	u8 lod_0_rgba_count;
	std::vector<u8> light;
	std::vector<Vec4f> msphere;
	TfragCube cube;
};

std::vector<Tfrag> read_tfrags(Buffer src);
void write_tfrags(OutBuffer dest, const std::vector<Tfrag>& tfrags);

packed_struct(TfragsHeader,
	/* 0x0 */ s32 table_offset;
	/* 0x4 */ s32 tfrag_count;
	/* 0x8 */ s32 unknown;
)

packed_struct(TfragHeader,
	/* 0x00 */ Vec4f bsphere;
	/* 0x10 */ s32 data;
	/* 0x14 */ u16 lod_2_ofs;
	/* 0x16 */ u16 shared_ofs;
	/* 0x18 */ u16 lod_1_ofs;
	/* 0x1a */ u16 lod_0_ofs;
	/* 0x1c */ u16 tex_ofs;
	/* 0x1e */ u16 rgba_ofs;
	/* 0x20 */ u8 common_size;
	/* 0x21 */ u8 lod_2_size;
	/* 0x22 */ u8 lod_1_size;
	/* 0x23 */ u8 lod_0_size;
	/* 0x24 */ u8 lod_2_rgba_count;
	/* 0x25 */ u8 lod_1_rgba_count;
	/* 0x26 */ u8 lod_0_rgba_count;
	/* 0x27 */ u8 base_only;
	/* 0x28 */ u8 texture_count;
	/* 0x29 */ u8 rgba_size;
	/* 0x2a */ u8 rgba_verts_loc;
	/* 0x2b */ u8 occl_index_stash;
	/* 0x2c */ u8 msphere_count;
	/* 0x2d */ u8 flags;
	/* 0x2e */ u16 msphere_ofs;
	/* 0x30 */ u16 light_ofs;
	/* 0x32 */ u16 light_vert_start_ofs;
	/* 0x34 */ u8 dir_lights_one;
	/* 0x35 */ u8 dir_lights_upd;
	/* 0x36 */ u16 point_lights;
	/* 0x38 */ u16 cube_ofs;
	/* 0x3a */ u16 occl_index;
	/* 0x3c */ u8 vert_count;
	/* 0x3d */ u8 tri_count;
	/* 0x3e */ u16 mip_dist;
)
