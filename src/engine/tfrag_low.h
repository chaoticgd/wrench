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

#ifndef ENGINE_TFRAG_LOW_H
#define ENGINE_TFRAG_LOW_H

#include <core/vif.h>
#include <core/buffer.h>
#include <core/collada.h>
#include <core/build_config.h>
#include <engine/basic_types.h>
#include <engine/gif.h>

packed_struct(TfragsHeader,
	/* 0x0 */ s32 table_offset;
	/* 0x4 */ s32 tfrag_count;
	/* 0x8 */ f32 thingy;
	/* 0xc */ u32 mysterious_second_thingy;
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
	packed_nested_anon_union(
		/* 0x32 */ u16 light_end_ofs_rac_gc_uya;
		/* 0x32 */ u16 light_vert_start_ofs_dl;
	)
	/* 0x34 */ u8 dir_lights_one;
	/* 0x35 */ u8 dir_lights_upd;
	/* 0x36 */ u16 point_lights;
	/* 0x38 */ u16 cube_ofs;
	/* 0x3a */ u16 occl_index;
	/* 0x3c */ u8 vert_count;
	/* 0x3d */ u8 tri_count;
	/* 0x3e */ u16 mip_dist;
)

packed_struct(TfragTexturePrimitive,
	/* 0x00 */ GifAdData16 d1_tex0_1;
	/* 0x10 */ GifAdData16 d2_tex1_1;
	/* 0x20 */ GifAdData16 d3_clamp_1;
	/* 0x30 */ GifAdData16 d4_miptbp1_1;
	/* 0x40 */ GifAdData16 d5_miptbp2_1;
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

packed_struct(TfragLight,
	/* 0x0 */ s8 unknown_0;
	/* 0x1 */ s8 intensity;
	/* 0x2 */ s8 azimuth;
	/* 0x3 */ s8 elevation;
	/* 0x4 */ s16 color;
	/* 0x6 */ s16 pad;
)

packed_struct(TfragHeaderUnpack,
	/* PACK UNPK */
	/* 0x00 0x00 */ u16 positions_common_count;
	/* 0x02 0x04 */ u16 unknown_2;
	/* 0x04 0x08 */ u16 positions_lod_01_count;
	/* 0x06 0x0c */ u16 unknown_6;
	/* 0x08 0x10 */ u16 positions_lod_0_count;
	/* 0x0a 0x14 */ u16 unknown_a;
	/* 0x0c 0x18 */ u16 positions_common_addr;
	/* 0x0e 0x1c */ u16 vertex_info_common_addr;
	/* 0x10 0x20 */ u16 unknown_10;
	/* 0x12 0x24 */ u16 vertex_info_lod_01_addr; // Only the LOD 01 and LOD 0 entries have vertex_data_offsets[0] populated.
	/* 0x14 0x28 */ u16 unknown_14;
	/* 0x16 0x2c */ u16 vertex_info_lod_0_addr;
	/* 0x18 0x30 */ u16 unknown_18;
	/* 0x1a 0x34 */ u16 indices_addr;
	/* 0x1c 0x38 */ u16 parent_indices_lod_01_addr;
	/* 0x1e 0x3c */ u16 unk_indices_2_lod_01_addr;
	/* 0x20 0x40 */ u16 parent_indices_lod_0_addr;
	/* 0x22 0x44 */ u16 unk_indices_2_lod_0_addr;
	/* 0x24 0x48 */ u16 strips_addr;
	/* 0x26 0x4c */ u16 texture_ad_gifs_addr;
)

packed_struct(TfragVertexPosition,
	/* PACK UNPK */
	/* 0x00 0x00 */ s16 x;
	/* 0x02 0x04 */ s16 y;
	/* 0x04 0x08 */ s16 z;
)

packed_struct(TfragVertexInfo,
	/* PACK UNPK */
	/* 0x00 0x00 */ s16 s;
	/* 0x02 0x04 */ s16 t;
	/* 0x04 0x08 */ s16 parent;
	/* 0x04 0x08 */ s16 vertex;
)

packed_struct(TfragStrip,
	/* PACK UNPK */
	/* 0x00 0x00 */ s8 vertex_count_and_flag;
	/* 0x01 0x04 */ s8 end_of_packet_flag;
	/* 0x02 0x08 */ s8 ad_gif_offset;
	/* 0x03 0x0c */ s8 pad;
)

struct TfragMemoryMap
{
	s32 header_common_addr = -1;
	s32 ad_gifs_common_addr = -1;
	s32 positions_common_addr = -1;
	s32 positions_lod_01_addr = -1;
	s32 positions_lod_0_addr = -1;
	s32 vertex_info_common_addr = -1;
	s32 vertex_info_lod_01_addr = -1;
	s32 vertex_info_lod_0_addr = -1;
	s32 parent_indices_lod_01_addr = -1;
	s32 unk_indices_2_lod_01_addr = -1;
	s32 parent_indices_lod_0_addr = -1;
	s32 unk_indices_2_lod_0_addr = -1;
	s32 indices_addr = -1;
	s32 strips_addr = -1;
};

// A full tfrag, including all LOD levels and migration information.
struct Tfrag
{
	Vec4f bsphere;
	u8 lod_2_rgba_count;
	u8 lod_1_rgba_count;
	u8 lod_0_rgba_count;
	u8 base_only;
	u8 rgba_verts_loc;
	u8 flags;
	u16 occl_index;
	u16 mip_dist;
	VifSTROW base_position;
	std::vector<u8> lod_2_indices;
	std::vector<TfragStrip> lod_2_strips;
	TfragHeaderUnpack common_vu_header;
	std::vector<TfragTexturePrimitive> common_textures;
	std::vector<TfragVertexInfo> common_vertex_info;
	std::vector<TfragVertexPosition> common_positions;
	std::vector<TfragStrip> lod_1_strips;
	std::vector<u8> lod_1_indices;
	std::vector<u8> lod_01_parent_indices;
	std::vector<u8> lod_01_unknown_indices_2;
	std::vector<TfragVertexInfo> lod_01_vertex_info;
	std::vector<TfragVertexPosition> lod_01_positions;
	std::vector<TfragVertexPosition> lod_0_positions;
	std::vector<TfragStrip> lod_0_strips;
	std::vector<u8> lod_0_indices;
	std::vector<u8> lod_0_parent_indices; // lod_0_parent_indices.size() ~= lod_0_positions.size()
	std::vector<u8> lod_0_unknown_indices_2;
	std::vector<TfragVertexInfo> lod_0_vertex_info;
	std::vector<TfragRgba> rgbas;
	std::vector<TfragLight> lights;
	std::vector<Vec4f> msphere;
	TfragCube cube;
	TfragMemoryMap memory_map;
	u16 positions_slack; // Space between the positions and vertex info arrays in VU memory.
};

struct Tfrags
{
	f32 thingy;
	u32 mysterious_second_thingy;
	std::vector<Tfrag> fragments;
};

Tfrags read_tfrags(Buffer src, Game game);
void write_tfrags(OutBuffer dest, const Tfrags& tfrags, Game game);
void allocate_tfrags_vu(Tfrags& tfrags);

#endif
