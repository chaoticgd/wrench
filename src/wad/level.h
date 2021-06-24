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

packed_struct(GpProperties,
	u8 data[0x340];
)

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
	s32 uid;
	s32 o_class;
	f32 scale;
	Vec3f position;
	Vec3f rotation;
	s32 pvar_index;
	struct {
		s32 unknown_4;
		s32 unknown_c;
		s32 unknown_18;
		s32 unknown_1c;
		s32 unknown_20;
		s32 unknown_24;
		s32 unknown_40;
		s32 unknown_44;
		s32 unknown_48;
		s32 unknown_4c;
		s32 unknown_54;
		s32 unknown_58;
		s32 unknown_5c;
		s32 unknown_60;
		s32 unknown_64;
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

packed_struct(DualMatrix,
	Mat4 mat_1;
	Mat4 mat_2;
)

struct Grindrail {
	std::vector<Vec4f> vertices;
};

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

packed_struct(Gp_GC_98_DL_74_FirstPart,
	Vec4f position;      // 0x0
	uint16_t counts[6];  // 0x10 -- only 5 are used, last one is padding
	uint32_t offsets[5]; // 0x1c
)

struct Gp_GC_98_DL_74 {
	std::vector<Gp_GC_98_DL_74_FirstPart> first_part;
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
	std::vector<DualMatrix> spheres;
	std::vector<DualMatrix> cylinders;
	std::vector<s32> gc_74_dl_58;
	std::vector<std::vector<Vec4f>> splines;
	std::vector<DualMatrix> cuboids;
	std::vector<u8> gc_88_dl_6c;
	Gp_GC_80_DL_64 gc_80_dl_64;
	GrindRails grindrails;
	Gp_GC_98_DL_74 gc_98_dl_74;
};

struct LevelWad : Wad {
	Gameplay gameplay_core;
};

#endif
