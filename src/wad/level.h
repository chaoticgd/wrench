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

#ifndef WAD_LEVEL_H
#define WAD_LEVEL_H

#include "util.h"
#include "buffer.h"

packed_struct(Gp_GC_8c_DL_70,
	u8 data[0x20];
)

packed_struct(GpPropertiesFirstPart,
	s32 background_r;       // 0x0
	s32 background_g;       // 0x4
	s32 background_b;       // 0x8
	s32 fog_r;              // 0xc
	s32 fog_g;              // 0x10
	s32 fog_b;              // 0x14
	f32 fog_near_dist;      // 0x18
	f32 fog_far_dist;       // 0x1c
	f32 fog_near_intensity; // 0x20
	f32 fog_far_intensity;  // 0x24
	f32 death_height;       // 0x28
	s32 is_spherical_world; // 0x2c
	Vec3f sphere_centre;    // 0x30
	s32 unknown_3c;         // 0x3c
	s32 unknown_40;         // 0x40
	s32 unknown_44;         // 0x44
	s32 unknown_48;         // 0x48
	s32 unknown_4c;         // 0x4c
	u8 unknown_50[0x340 - 0x50]; // 0x50
)

struct GpProperties {
	GpPropertiesFirstPart first_part;
};

enum class Language {
	ENGLISH_US, ENGLISH_UK
};

struct GpString {
	std::optional<std::string> string;
	s16 id;
	s16 unknown_6;
	s32 unknown_8;
	s16 unknown_c;
	s16 unknown_e;
};

packed_struct(GpImportCamera,
	u8 data[0x20];
)

packed_struct(Gp_GC_c_DL_8,
	u8 unknown_0[0x10];
	Mat4 mat_1;
	Mat4 mat_2;
)

struct MobyInstance {
	s32 size;
	s8 mission;
	s32 uid;
	s32 bolts;
	s32 o_class;
	f32 scale;
	s16 draw_dist;
	s8 update_dist;
	Vec3f position;
	Vec3f rotation;
	s8 group;
	s32 is_rooted;
	f32 rooted_dist;
	s32 pvar_index;
	s32 lights_1;
	s32 lights_2;
	s32 lights_3;
	struct {
		s32 unknown_20;
		s32 unknown_24;
		s32 unknown_4c;
		s32 unknown_54;
		s32 unknown_58;
		s32 unknown_68;
		s32 unknown_6c;
	} dl;
	std::vector<u8> pvar;
};

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

struct MobyStore {
	std::vector<s32> classes;
	s32 dynamic_count;
	std::vector<MobyInstance> instances;
	std::optional<std::vector<PvarTableEntry>> pvars_temp; // Only used during reading.
	std::vector<u8> pvar_data;
};

packed_struct(Gp_DL_3c,
	int32_t unknown_0;
	int32_t unknown_4;
)

packed_struct(Gp_GC_64_DL_48,
	uint8_t unknown[0x8];
)

struct GpMobyGroups {
	std::vector<s32> first_part;
	std::vector<s8> second_part;
};

struct Gp_GC_54_DL_38 {
	std::vector<s8> first_part;
	std::vector<s64> second_part;
};

packed_struct(GpSphere,
	Mat3 matrix;
	Vec4f pos;
	Mat3 imatrix;
	Vec4f rot;
)

packed_struct(GpCylinder,
	Mat3 matrix;
	Vec4f pos;
	Mat3 imatrix;
	Vec4f rot;
)

struct Grindrail {
	std::vector<Vec4f> vertices;
};

packed_struct(GpCuboid,
	Mat3 matrix;
	Vec4f pos;
	Mat3 imatrix;
	Vec4f rot;
)

struct Gp_GC_80_DL_64 {
	std::vector<u8> first_part;
	std::vector<u8> second_part;
};

packed_struct(GrindRailData,
	Vec4f point;
	uint8_t unknown_10[0x10];
)

struct GrindRails {
	std::vector<GrindRailData> grindrails;
	std::vector<std::vector<Vec4f>> splines;
};

packed_struct(GpGameplayAreaListFirstPart,
	Vec4f position;      // 0x0
	uint16_t counts[6];  // 0x10 -- only 5 are used, last one is padding
	uint32_t offsets[5]; // 0x1c
)

struct GpGameplayAreaList {
	std::vector<GpGameplayAreaListFirstPart> first_part;
	std::vector<s32> second_part;
	s32 part_offsets[5];
};

struct Gameplay {
	std::vector<Gp_GC_8c_DL_70> gc_8c_dl_70;
	GpProperties properties;
	s32 before_us_strings[3] = {0, 0, 0};
	std::vector<GpString> us_english_strings;
	std::vector<GpString> uk_english_strings;
	std::vector<GpString> french_strings;
	std::vector<GpString> german_strings;
	std::vector<GpString> spanish_strings;
	std::vector<GpString> italian_strings;
	std::vector<GpString> japanese_strings;
	std::vector<GpString> korean_strings;
	std::vector<GpImportCamera> import_cameras;
	std::vector<Gp_GC_c_DL_8> gc_c_dl_8;
	MobyStore moby;
	std::vector<Gp_DL_3c> dl_3c;
	std::vector<Gp_GC_64_DL_48> gc_64_dl_48;
	GpMobyGroups moby_groups;
	Gp_GC_54_DL_38 gc_54_dl_38;
	std::vector<GpSphere> spheres;
	std::vector<GpCylinder> cylinders;
	std::vector<s32> gc_74_dl_58;
	std::vector<std::vector<Vec4f>> splines;
	std::vector<GpCuboid> cuboids;
	std::vector<u8> gc_88_dl_6c;
	Gp_GC_80_DL_64 gc_80_dl_64;
	GrindRails grindrails;
	GpGameplayAreaList gameplay_area_list;
};

struct LevelWad : Wad {
	Gameplay gameplay_core;
};

#endif
