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

#include "json.h"
#include "util.h"
#include "buffer.h"

packed_struct(Gp_GC_8c_DL_70,
	u8 data[0x20];
)

packed_struct(Rgb96,
	s32 r;
	s32 g;
	s32 b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD("r", r);
		DEF_FIELD("g", g);
		DEF_FIELD("b", b);
	}
)

packed_struct(GpPropertiesFirstPart,
	Rgb96 background_col;   // 0x0
	Rgb96 fog_col;          // 0xc
	f32 fog_near_dist;      // 0x18
	f32 fog_far_dist;       // 0x1c
	f32 fog_near_intensity; // 0x20
	f32 fog_far_intensity;  // 0x24
	f32 death_height;       // 0x28
	s32 is_spherical_world; // 0x2c
	Vec3f sphere_centre;    // 0x30
	f32 unknown_3c;         // 0x3c
	f32 unknown_40;         // 0x40
	f32 unknown_44;         // 0x44
	s32 unknown_48;         // 0x48
	s32 unknown_4c;         // 0x4c
	s32 unknown_50;         // 0x50
	s32 unknown_54;         // 0x54
	s32 unknown_58;         // 0x58
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_OBJECT("background_col", background_col);
		DEF_OBJECT("fog_col", fog_col);
		DEF_FIELD("fog_near_dist", fog_near_dist);
		DEF_FIELD("fog_far_dist", fog_far_dist);
		DEF_FIELD("fog_near_intensity", fog_near_intensity);
		DEF_FIELD("fog_far_intensity", fog_far_intensity);
		DEF_FIELD("death_height", death_height);
		DEF_FIELD("is_spherical_world", is_spherical_world);
		DEF_OBJECT("sphere_centre", sphere_centre);
		DEF_FIELD("unknown_3c", unknown_3c);
		DEF_FIELD("unknown_40", unknown_40);
		DEF_FIELD("unknown_44", unknown_44);
		DEF_FIELD("unknown_48", unknown_48);
		DEF_FIELD("unknown_4c", unknown_4c);
		DEF_FIELD("unknown_50", unknown_50);
		DEF_FIELD("unknown_54", unknown_54);
		DEF_FIELD("unknown_58", unknown_58);
	}
)

packed_struct(GpPropertiesSecondPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	s32 unknown_18;
	s32 unknown_1c;
)

packed_struct(GpPropertiesThirdPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
)

packed_struct(GpPropertiesFourthPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
)

packed_struct(GpPropertiesFifthPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	s32 sixth_part_count;
)

struct GpProperties {
	GpPropertiesFirstPart first_part;
	std::vector<GpPropertiesSecondPart> second_part;
	s32 core_sounds_count;
	std::vector<GpPropertiesThirdPart> third_part;
	GpPropertiesFourthPart fourth_part;
	GpPropertiesFifthPart fifth_part;
	std::vector<s8> sixth_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		first_part.enumerate_fields(t);
	}
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

struct ImportCamera {
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	s32 unknown_18;
	s32 pvar_index; // Only used during reading!
	std::vector<u8> pvars;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_HEXDUMP("pvar", pvars);
	}
};

packed_struct(GpShape,
	Mat3 matrix;
	Vec4f pos;
	Mat3 imatrix;
	Vec4f rot;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_OBJECT("matrix", matrix);
		DEF_OBJECT("pos", pos);
		DEF_OBJECT("imatrix", imatrix);
		DEF_OBJECT("rot", rot);
	}
)

struct SoundInstance {
	s16 o_class;
	s16 m_class;
	s32 pvar_index; // Only used during reading!
	f32 range;
	GpShape cuboid;
	std::vector<u8> pvars;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD("o_class", o_class);
		DEF_FIELD("m_class", m_class);
		DEF_FIELD("range", range);
		DEF_OBJECT("cuboid", cuboid);
		DEF_HEXDUMP("pvar", pvars);
	}
};

struct MobyInstance {
	s32 size;
	s8 mission;
	s32 uid;
	s32 bolts;
	s32 o_class;
	f32 scale;
	s16 draw_dist;
	s32 update_dist;
	Vec3f position;
	Vec3f rotation;
	s8 group;
	s32 is_rooted;
	f32 rooted_dist;
	s32 pvar_index; // Only used during reading!
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
	std::vector<u8> pvars;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD("size", size);
		DEF_FIELD("mission", mission);
		DEF_FIELD("uid", uid);
		DEF_FIELD("bolts", bolts);
		DEF_FIELD("o_class", o_class);
		DEF_FIELD("scale", scale);
		DEF_FIELD("draw_dist", draw_dist);
		DEF_FIELD("update_dist", update_dist);
		DEF_OBJECT("position", position);
		DEF_OBJECT("rotation", rotation);
		DEF_FIELD("group", group);
		DEF_FIELD("is_rooted", is_rooted);
		DEF_FIELD("rooted_dist", rooted_dist);
		DEF_FIELD("lights_1", lights_1);
		DEF_FIELD("lights_2", lights_2);
		DEF_FIELD("lights_3", lights_3);
		DEF_FIELD("unknown_20", dl.unknown_20);
		DEF_FIELD("unknown_24", dl.unknown_24);
		DEF_FIELD("unknown_4c", dl.unknown_4c);
		DEF_FIELD("unknown_54", dl.unknown_54);
		DEF_FIELD("unknown_58", dl.unknown_58);
		DEF_FIELD("unknown_68", dl.unknown_68);
		DEF_FIELD("unknown_6c", dl.unknown_6c);
		DEF_HEXDUMP("pvar", pvars);
	}
};

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

struct MobyStore {
	std::vector<s32> classes;
	s32 dynamic_count;
	std::vector<MobyInstance> instances;
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

packed_struct(GpBSphere,
	f32 x;
	f32 y;
	f32 z;
	f32 rad;
)

enum class AreaPart {
	PATHS = 0,
	CUBOIDS = 1,
	SPHERES = 2,
	CYLINDERS = 3,
	NEG_CUBOIDS = 4
};

struct GpArea {
	GpBSphere bsphere;
	s32 last_update_time;
	std::vector<s32> parts[5];
};

struct Gameplay {
	std::vector<Gp_GC_8c_DL_70> gc_8c_dl_70;
	GpProperties properties;
	std::vector<GpString> us_english_strings;
	std::vector<GpString> uk_english_strings;
	std::vector<GpString> french_strings;
	std::vector<GpString> german_strings;
	std::vector<GpString> spanish_strings;
	std::vector<GpString> italian_strings;
	std::vector<GpString> japanese_strings;
	std::vector<GpString> korean_strings;
	std::vector<ImportCamera> import_cameras;
	std::vector<SoundInstance> sound_instances;
	MobyStore moby;
	std::vector<Gp_DL_3c> dl_3c;
	std::vector<Gp_GC_64_DL_48> gc_64_dl_48;
	GpMobyGroups moby_groups;
	Gp_GC_54_DL_38 gc_54_dl_38;
	std::vector<GpShape> spheres;
	std::vector<GpShape> cylinders;
	std::vector<s32> gc_74_dl_58;
	std::vector<std::vector<Vec4f>> splines;
	std::vector<GpShape> cuboids;
	std::vector<u8> gc_88_dl_6c;
	Gp_GC_80_DL_64 gc_80_dl_64;
	GrindRails grindrails;
	std::vector<GpArea> gameplay_area_list;
	
	// Only used while reading the binary gameplay file, empty otherwise.
	std::optional<std::vector<PvarTableEntry>> pvars_temp;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_OBJECT("properties", properties);
		DEF_OBJECT_LIST("cameras", import_cameras);
		DEF_OBJECT_LIST("sound_instances", sound_instances);
		DEF_OBJECT_LIST("moby_instances", moby.instances);
		DEF_OBJECT_LIST("spheres", spheres);
		DEF_OBJECT_LIST("cylinders", cylinders);
		DEF_OBJECT_LIST("cuboids", cuboids);
	}
};

struct LevelWad : Wad {
	Gameplay gameplay_core;
};

Json write_gameplay_json(Gameplay& gameplay);

#endif
