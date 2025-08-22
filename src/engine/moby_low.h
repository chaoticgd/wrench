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

#ifndef ENGINE_MOBY_LOW_H
#define ENGINE_MOBY_LOW_H

#include <core/vif.h>
#include <core/buffer.h>
#include <core/collada.h>
#include <core/build_config.h>
#include <engine/basic_types.h>
#include <engine/moby_animation.h>
#include <engine/moby_high.h>
#include <engine/moby_packet.h>

namespace MOBY {

struct MobyMeshSection
{
	std::vector<MobyPacket> high_lod;
	u8 high_lod_count = 0;
	std::vector<MobyPacket> low_lod;
	u8 low_lod_count = 0;
	std::vector<MobyMetalPacket> metal;
	u8 metal_count = 0;
	bool has_packet_table = true;
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

struct MobyJointEntry
{
	std::vector<u8> thing_one;
	std::vector<u8> thing_two;
};

struct MobyAnimationSection
{
	std::vector<Opt<MobySequence>> sequences;
	Opt<std::vector<Mat4>> skeleton;
	Opt<std::vector<MobyTrans>> common_trans;
	u8 joint_count = 0;
	std::vector<MobyJointEntry> joints;
};

struct MobyCollision
{
	u16 unknown_0;
	u16 unknown_2;
	std::vector<u8> first_part;
	std::vector<Vec3f> second_part;
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

struct MobyCornKernel
{
	Vec4f vec;
	std::vector<MobyVec4> vertices;
};

struct MobyCornCob
{
	Opt<MobyCornKernel> kernels[16];
};

struct MobyClassData
{
	MobyMeshSection mesh;
	MobyAnimationSection animation;
	std::vector<MobyBangle> bangles;
	Opt<MobyCornCob> corncob;
	std::vector<u8> shadow;
	Opt<MobyCollision> collision;
	std::vector<MobySoundDef> sound_defs;
	u8 unknown_9 = 0;
	u8 lod_trans = 0;
	f32 scale = 1.f;
	u8 mip_dist = 0;
	glm::vec4 bounding_sphere = {0.f, 0.f, 0.f, 0.f};
	s32 glow_rgba = 0;
	s16 mode_bits = 0;
	u8 type = 0;
	u8 mode_bits2 = 0;
	s32 header_end_offset = 0;
	s32 packet_table_offset = 0;
	u8 rac1_byte_a = 0;
	u8 rac1_byte_b = 0;
	u16 rac1_short_2e = 0;
	std::vector<std::array<u32, 256>> team_palettes;
	s32 palettes_per_texture = 0;
	bool force_rac1_format = false; // Used for some mobies in the R&C2 Insomniac Museum.
};

packed_struct(MobyMeshInfo,
	/* 0x0 */ u8 high_lod_count;
	/* 0x1 */ u8 low_lod_count;
	/* 0x2 */ u8 metal_count;
	/* 0x3 */ u8 metal_begin;
)

packed_struct(MobyClassHeader,
	/* 0x00 */ s32 packet_table_offset;
	/* 0x04 */ MobyMeshInfo mesh_info;
	/* 0x08 */ u8 joint_count;
	/* 0x09 */ u8 unknown_9;
	/* 0x0a */ u8 rac1_byte_a;
	packed_nested_anon_union(
		/* 0x0b */ u8 rac12_byte_b; // 0x00 => R&C2 format.
		/* 0x0b */ u8 rac3dl_team_textures;
	)
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

packed_struct(MobyCollisionHeader,
	/* 0x0 */ u16 unknown_0;
	/* 0x2 */ u16 unknown_2;
	/* 0x4 */ s32 first_part_size;
	/* 0x8 */ s32 third_part_size;
	/* 0xc */ s32 second_part_size;
)

packed_struct(MobyCornCobHeader,
	u8 kernels[16];
)

packed_struct(MobyArmorHeader,
	/* 0x00 */ MobyMeshInfo info;
	/* 0x04 */ s32 packet_table_offset;
	/* 0x08 */ s32 gif_usage;
	/* 0x0c */ s32 pad;
)

MobyClassData read_class(Buffer src, Game game);
void write_class(OutBuffer dest, const MobyClassData& moby, Game game);
MobyMeshSection read_mesh_only_class(Buffer src, Game game);
void write_mesh_only_class(OutBuffer dest, const MobyMeshSection& moby, f32 scale, Game game);

MobyMeshSection read_moby_mesh_section(Buffer src, s64 table_ofs, MobyMeshInfo info, MobyFormat format);
s64 allocate_packet_table(OutBuffer& dest, const MobyMeshSection& mesh, size_t bangle_count);
MobyMeshInfo write_moby_mesh_section(
	OutBuffer& dest,
	std::vector<MobyGifUsage>& gif_usage,
	s64 table_ofs,
	const MobyMeshSection& mesh,
	f32 scale,
	MobyFormat format);

ColladaScene recover_moby_class(const MobyClassData& moby, s32 o_class, s32 texture_count);
MobyClassData build_moby_class(const ColladaScene& scene);

}

#endif
