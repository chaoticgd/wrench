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

#include <core/vif.h>
#include <core/buffer.h>
#include <core/collada.h>

packed_struct(MobyTexCoord,
	/* 0x0 */ s16 s;
	/* 0x2 */ s16 t;
)

packed_struct(MobyVertex,
	/* 0x0 */ u32 low_word;
	union {
		struct {
			/* 0x4 */ u8 unknown_4;
			/* 0x5 */ u8 unknown_5;
			/* 0x6 */ u8 unknown_6;
			/* 0x7 */ u8 unknown_7;
			/* 0x8 */ u8 unknown_8;
			/* 0x9 */ u8 unknown_9;
			/* 0xa */ s16 x;
			/* 0xc */ s16 y;
			/* 0xe */ s16 z;
		} regular;
		/* 0x4 */ u16 trailing_vertex_indices[6];
	};
)

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
	std::vector<u16> unknowns;
	std::vector<MobyVertex> vertices;
	u16 vertex_count_2 = 0;
	u16 vertex_count_4 = 0;
	std::vector<u16> duplicate_vertices;
	u16 unknown_e = 0;
	std::vector<u8> unknown_e_data;
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

struct MobyFrame {
	struct {
		f32 unknown_0;
		u16 unknown_4;
		u16 unknown_c;
		std::vector<u64> joint_data;
		std::vector<u64> thing_1;
		std::vector<u64> thing_2;
	} regular;
	struct {
		u16 inverse_unknown_0;
		u16 unknown_4;
		std::vector<u8> first_part;
		std::vector<u8> second_part;
		std::vector<u8> third_part;
		std::vector<u8> fourth_part;
		std::vector<u8> fifth_part_1;
		std::vector<u8> fifth_part_2;
	} special;
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
	s32 animation_info = 0;
	u8 sound_count = 0xff;
	u8 unknown_13 = 0;
	bool has_special_data = false;
	struct {
		std::vector<u16> joint_data;
		std::vector<u64> thing_1;
		std::vector<u64> thing_2;
	} special;
};

struct MobyCollision {
	u16 unknown_0;
	u16 unknown_2;
	std::vector<u8> first_part;
	std::vector<Vec3f> second_part;
	std::vector<u8> third_part;
};

packed_struct(MobyTrans,
	Vec3f vector;
	u16 parent_offset;
	u16 seventy;
)

packed_struct(MobyJoint,
	Mat3 matrix;
	MobyTrans trans;
)

struct MobyJointEntry {
	std::vector<u8> thing_one;
	std::vector<u8> thing_two;
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

struct MobyCornKernel {
	Vec4f vec;
	std::vector<MobyVertexPosition> vertices;
};

struct MobyCornCob {
	Opt<MobyCornKernel> kernels[16];
};

struct MobyClassData {
	std::vector<MobySubMesh> submeshes;
	u8 submesh_count = 0;
	std::vector<MobySubMesh> low_lod_submeshes;
	u8 low_lod_submesh_count = 0;
	std::vector<MobyMetalSubMesh> metal_submeshes;
	u8 metal_submesh_count = 0;
	Opt<MobyBangles> bangles;
	Opt<MobyCornCob> corncob;
	std::vector<Opt<MobySequence>> sequences;
	std::vector<u8> mystery_data;
	Opt<MobyCollision> collision;
	std::vector<Mat4> skeleton;
	std::vector<MobyTrans> common_trans;
	std::vector<MobyJointEntry> joints;
	std::vector<MobySoundDef> sound_defs;
	u8 unknown_9;
	u8 lod_trans;
	u8 shadow;
	f32 scale;
	u8 mip_dist;
	glm::vec4 bounding_sphere;
	s32 glow_rgba;
	s16 mode_bits;
	u8 type;
	u8 mode_bits2;
	s32 header_end_offset;
	s32 submesh_table_offset;
	u8 rac1_byte_a;
	u8 rac1_byte_b;
	u16 rac1_short_2e;
	bool force_rac1_format = false; // Used for some mobies in the R&C2 Insomniac Museum.
	bool has_submesh_table = false;
};

packed_struct(MobyClassHeader,
	/* 0x00 */ s32 submesh_table_offset;
	/* 0x04 */ u8 submesh_count;
	/* 0x05 */ u8 low_lod_submesh_count;
	/* 0x06 */ u8 metal_submesh_count;
	/* 0x07 */ u8 metal_submesh_begin;
	/* 0x08 */ u8 joint_count;
	/* 0x09 */ u8 unknown_9;
	/* 0x0a */ u8 rac1_byte_a;
	union {
		/* 0x0b */ u8 rac12_byte_b; // 0x00 => R&C2 format.
		/* 0x0b */ u8 rac3dl_team_textures;
	};
	/* 0x0c */ u8 sequence_count;
	/* 0x0d */ u8 sound_count;
	/* 0x0e */ u8 lod_trans;
	/* 0x0f */ u8 shadow;
	/* 0x10 */ s32 collision;
	/* 0x14 */ s32 skeleton;
	/* 0x18 */ s32 common_trans;
	/* 0x1c */ s32 joints;
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

packed_struct(MobyVertexTableHeaderRac1,
	/* 0x00 */ u32 unknown_count_0;
	/* 0x04 */ u32 vertex_count_2;
	/* 0x08 */ u32 vertex_count_4;
	/* 0x0c */ u32 main_vertex_count;
	/* 0x10 */ u32 duplicate_vertex_count;
	/* 0x14 */ u32 transfer_vertex_count; // transfer_vertex_count == vertex_count_2 + vertex_count_4 + main_vertex_count + duplicate_vertex_count
	/* 0x18 */ u32 vertex_table_offset;
	/* 0x1c */ u32 unknown_e;
)

packed_struct(MobyVertexTableHeaderRac23DL,
	/* 0x0 */ u16 unknown_count_0;
	/* 0x2 */ u16 vertex_count_2;
	/* 0x4 */ u16 vertex_count_4;
	/* 0x6 */ u16 main_vertex_count;
	/* 0x8 */ u16 duplicate_vertex_count;
	/* 0xa */ u16 transfer_vertex_count; // transfer_vertex_count == vertex_count_2 + vertex_count_4 + main_vertex_count + duplicate_vertex_count
	/* 0xc */ u16 vertex_table_offset;
	/* 0xe */ u16 unknown_e;
)

packed_struct(MobyMetalVertexTableHeader,
	/* 0x0 */ s32 vertex_count;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 unknown_8;
	/* 0xc */ s32 unknown_c;
)

packed_struct(MobySequenceHeader,
	/* 0x00 */ Vec4f bounding_sphere;
	/* 0x10 */ u8 frame_count;
	/* 0x11 */ u8 sound_count;
	/* 0x12 */ u8 trigger_count;
	/* 0x13 */ u8 unknown_13;
	/* 0x14 */ u32 triggers;
	/* 0x18 */ u32 animation_info;
)

packed_struct(MobyFrameHeader,
	/* 0x0 */ f32 unknown_0;
	/* 0x4 */ u16 unknown_4;
	/* 0x6 */ u16 data_size_qwords;
	/* 0x8 */ u16 joint_data_size;
	/* 0xa */ u16 thing_1_count;
	/* 0xc */ u16 unknown_c;
	/* 0xe */ u16 thing_2_count;
)

packed_struct(MobyCollisionHeader,
	/* 0x0 */ u16 unknown_0;
	/* 0x2 */ u16 unknown_2;
	/* 0x4 */ s32 first_part_size;
	/* 0x8 */ s32 third_part_size;
	/* 0xc */ s32 second_part_size;
)

packed_struct(MobyGifUsageTableEntry,
	u8 texture_indices[12];
	u32 offset_and_terminator; // High byte is 0x80 => Last entry in the table.
)

packed_struct(MobyCornCobHeader,
	u8 kernels[16];
)

// Note: R&C2 has some R&C1-format mobies.
enum class MobyFormat {
	RAC1, RAC2, RAC3DL
};

MobyClassData read_moby_class(Buffer src, Game game);
void write_moby_class(OutBuffer dest, const MobyClassData& moby, Game game);
MobySequence read_moby_sequence(Buffer src, s64 seq_ofs, s32 joint_count);
s64 write_moby_sequence(OutBuffer dest, const MobySequence& sequence, s64 header_ofs, s32 joint_count);
ColladaScene recover_moby_class(const MobyClassData& moby, s32 o_class, s32 texture_count);
MobyClassData build_moby_class(const ColladaScene& scene);

#endif
