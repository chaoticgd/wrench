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

using OriginalIndex = s32;

packed_struct(GC_8c_DL_70,
	u32 unknown_0;
	u32 unknown_4;
	u32 unknown_8;
	u32 unknown_c;
	u32 unknown_10;
	u32 unknown_14;
	u32 unknown_18;
	u32 unknown_1c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(unknown_10);
		DEF_PACKED_FIELD(unknown_14);
		DEF_PACKED_FIELD(unknown_18);
		DEF_PACKED_FIELD(unknown_1c);
	}
)

packed_struct(Rgb96,
	s32 r;
	s32 g;
	s32 b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(r);
		DEF_PACKED_FIELD(g);
		DEF_PACKED_FIELD(b);
	}
)

packed_struct(PropertiesFirstPart,
	Rgb96 background_col;   // 0x0
	Rgb96 fog_col;          // 0xc
	f32 fog_near_dist;      // 0x18
	f32 fog_far_dist;       // 0x1c
	f32 fog_near_intensity; // 0x20
	f32 fog_far_intensity;  // 0x24
	f32 death_height;       // 0x28
	s32 is_spherical_world; // 0x2c
	Vec3f sphere_centre;    // 0x30
	Vec3f ship_position;    // 0x3c
	f32 ship_rotation_z;    // 0x48
	s32 unknown_4c;         // 0x4c
	s32 unknown_50;         // 0x50
	s32 unknown_54;         // 0x54
	s32 unknown_58;         // 0x58
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(background_col);
		DEF_PACKED_FIELD(fog_col);
		DEF_PACKED_FIELD(fog_near_dist);
		DEF_PACKED_FIELD(fog_far_dist);
		DEF_PACKED_FIELD(fog_near_intensity);
		DEF_PACKED_FIELD(fog_far_intensity);
		DEF_PACKED_FIELD(death_height);
		DEF_PACKED_FIELD(is_spherical_world);
		DEF_PACKED_FIELD(sphere_centre);
		DEF_PACKED_FIELD(ship_position);
		DEF_PACKED_FIELD(ship_rotation_z);
		DEF_PACKED_FIELD(unknown_4c);
		DEF_PACKED_FIELD(unknown_50);
		DEF_PACKED_FIELD(unknown_54);
		DEF_PACKED_FIELD(unknown_58);
	}
)

packed_struct(PropertiesSecondPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	s32 unknown_18;
	s32 unknown_1c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(unknown_10);
		DEF_PACKED_FIELD(unknown_14);
		DEF_PACKED_FIELD(unknown_18);
		DEF_PACKED_FIELD(unknown_1c);
	}
)

packed_struct(PropertiesThirdPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
	}
)

packed_struct(PropertiesFourthPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(unknown_10);
		DEF_PACKED_FIELD(unknown_14);
	}
)

packed_struct(PropertiesFifthPart,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	s32 sixth_part_count;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(unknown_10);
		DEF_PACKED_FIELD(unknown_14);
		DEF_PACKED_FIELD(sixth_part_count);
	}
)

struct Properties {
	PropertiesFirstPart first_part;
	std::vector<PropertiesSecondPart> second_part;
	s32 core_sounds_count;
	s32 rac3_third_part = 0;
	std::vector<PropertiesThirdPart> third_part;
	PropertiesFourthPart fourth_part;
	PropertiesFifthPart fifth_part;
	std::vector<s8> sixth_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
		DEF_FIELD(core_sounds_count);
		DEF_FIELD(rac3_third_part);
		DEF_FIELD(third_part);
		DEF_FIELD(fourth_part);
		DEF_FIELD(fifth_part);
		DEF_FIELD(sixth_part);
	}
};

struct HelpMessage {
	std::optional<std::string> string;
	s16 id;
	s16 short_id;
	s16 third_person_id;
	s16 coop_id;
	s16 vag;
	s16 character;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(string);
		DEF_FIELD(id);
		DEF_FIELD(short_id);
		DEF_FIELD(third_person_id);
		DEF_FIELD(coop_id);
		DEF_FIELD(vag);
		DEF_FIELD(character);
		DEF_FIELD(original_index);
	}
};

struct GC_84_Instance {
	f32 unknown_0;
	f32 unknown_4;
	f32 unknown_8;
	f32 unknown_c;
	f32 unknown_10;
	f32 unknown_14;
	f32 unknown_18;
	f32 unknown_1c;
	s32 unknown_20;
	s32 unknown_24;
	s32 unknown_28;
	s32 unknown_2c;
	s32 unknown_30;
	s32 unknown_34;
	s32 unknown_38;
	s32 unknown_3c;
	s32 unknown_40;
	s32 unknown_44;
	s32 unknown_48;
	s32 unknown_4c;
	f32 unknown_50;
	f32 unknown_54;
	f32 unknown_58;
	f32 unknown_5c;
	s32 unknown_60;
	s32 unknown_64;
	s32 unknown_68;
	s32 unknown_6c;
	s32 unknown_70;
	s32 unknown_74;
	s32 unknown_78;
	s32 unknown_7c;
	s32 unknown_80;
	s32 unknown_84;
	s32 unknown_88;
	s32 unknown_8c;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(unknown_0);
		DEF_FIELD(unknown_4);
		DEF_FIELD(unknown_8);
		DEF_FIELD(unknown_c);
		DEF_FIELD(unknown_10);
		DEF_FIELD(unknown_14);
		DEF_FIELD(unknown_18);
		DEF_FIELD(unknown_1c);
		DEF_FIELD(unknown_20);
		DEF_FIELD(unknown_24);
		DEF_FIELD(unknown_28);
		DEF_FIELD(unknown_2c);
		DEF_FIELD(unknown_30);
		DEF_FIELD(unknown_34);
		DEF_FIELD(unknown_38);
		DEF_FIELD(unknown_3c);
		DEF_FIELD(unknown_40);
		DEF_FIELD(unknown_44);
		DEF_FIELD(unknown_48);
		DEF_FIELD(unknown_4c);
		DEF_FIELD(unknown_50);
		DEF_FIELD(unknown_54);
		DEF_FIELD(unknown_58);
		DEF_FIELD(unknown_5c);
		DEF_FIELD(unknown_60);
		DEF_FIELD(unknown_64);
		DEF_FIELD(unknown_68);
		DEF_FIELD(unknown_6c);
		DEF_FIELD(unknown_70);
		DEF_FIELD(unknown_74);
		DEF_FIELD(unknown_78);
		DEF_FIELD(unknown_7c);
		DEF_FIELD(unknown_80);
		DEF_FIELD(unknown_84);
		DEF_FIELD(unknown_88);
		DEF_FIELD(unknown_8c);
		DEF_FIELD(original_index);
	}
};

struct ImportCamera {
	s32 type;
	Vec3f position;
	Vec3f rotation;
	s32 pvar_index;
	std::vector<u8> pvars;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(type);
		DEF_FIELD(position);
		DEF_FIELD(rotation);
		DEF_HEXDUMP(pvars);
		DEF_FIELD(original_index);
	}
};

packed_struct(ShapePacked,
	Mat3 matrix;
	Vec4f pos;
	Mat3 imatrix;
	Vec4f rot;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(matrix);
		DEF_PACKED_FIELD(pos);
		DEF_PACKED_FIELD(imatrix);
		DEF_PACKED_FIELD(rot);
	}
)

struct Shape {
	Mat3 matrix;
	Vec4f pos;
	Mat3 imatrix;
	Vec4f rot;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(matrix);
		DEF_FIELD(pos);
		DEF_FIELD(imatrix);
		DEF_FIELD(rot);
		DEF_FIELD(original_index);
	}
};

struct SoundInstance {
	s16 o_class;
	s16 m_class;
	s32 pvar_index;
	f32 range;
	ShapePacked cuboid;
	std::vector<u8> pvars;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(o_class);
		DEF_FIELD(m_class);
		DEF_FIELD(range);
		DEF_FIELD(cuboid);
		DEF_HEXDUMP(pvars);
		DEF_FIELD(original_index);
	}
};

struct MobyInstance {
	s8 mission;
	s32 uid;
	s32 bolts;
	s32 o_class;
	f32 scale;
	s32 draw_dist;
	s32 update_dist;
	Vec3f position;
	Vec3f rotation;
	s32 group;
	s32 is_rooted;
	f32 rooted_dist;
	s32 occlusion;
	s32 mode_bits;
	Rgb96 light_col;
	s32 light;
	
	struct {
		s32 unknown_8;
		s32 unknown_c;
		s32 unknown_18;
		s32 unknown_1c;
		s32 unknown_20;
		s32 unknown_24;
		s32 unknown_38;
		s32 unknown_3c;
		s32 unknown_4c;
		s32 unknown_84;
	} rac23;
	
	s32 pvar_index; // Only used during reading!
	std::vector<u8> pvars;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(mission);
		DEF_FIELD(uid);
		DEF_FIELD(bolts);
		DEF_FIELD(o_class);
		DEF_FIELD(scale);
		DEF_FIELD(draw_dist);
		DEF_FIELD(update_dist);
		DEF_FIELD(position);
		DEF_FIELD(rotation);
		DEF_FIELD(group);
		DEF_FIELD(is_rooted);
		DEF_FIELD(rooted_dist);
		DEF_FIELD(occlusion);
		DEF_FIELD(mode_bits);
		DEF_FIELD(light_col);
		DEF_FIELD(light);
		DEF_HEXDUMP(pvars);
		DEF_FIELD(original_index);
		
		DEF_FIELD(rac23.unknown_8);
		DEF_FIELD(rac23.unknown_c);
		DEF_FIELD(rac23.unknown_18);
		DEF_FIELD(rac23.unknown_1c);
		DEF_FIELD(rac23.unknown_20);
		DEF_FIELD(rac23.unknown_24);
		DEF_FIELD(rac23.unknown_38);
		DEF_FIELD(rac23.unknown_3c);
		DEF_FIELD(rac23.unknown_4c);
		DEF_FIELD(rac23.unknown_84);
	}
};

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

packed_struct(DL_3c,
	int32_t unknown_0;
	int32_t unknown_4;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
	}
)

packed_struct(GC_64_DL_48,
	uint8_t unknown_0;
	uint8_t unknown_4;
	uint8_t unknown_8;
	uint8_t unknown_c;
	uint8_t unknown_10;
	uint8_t unknown_14;
	uint8_t unknown_18;
	uint8_t unknown_1c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(unknown_10);
		DEF_PACKED_FIELD(unknown_14);
		DEF_PACKED_FIELD(unknown_18);
		DEF_PACKED_FIELD(unknown_1c);
	}
)

struct MobyGroups {
	std::vector<s32> first_part;
	std::vector<s8> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
	}
};

struct GC_54_DL_38 {
	std::vector<s8> first_part;
	std::vector<s64> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
	}
};

struct Path {
	std::vector<Vec4f> vertices;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(vertices);
		DEF_FIELD(original_index);
	}
};

struct GC_80_DL_64 {
	std::vector<u8> first_part;
	std::vector<u8> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
	}
};

packed_struct(BoundingSphere,
	f32 x;
	f32 y;
	f32 z;
	f32 radius;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(x);
		DEF_PACKED_FIELD(y);
		DEF_PACKED_FIELD(z);
		DEF_PACKED_FIELD(radius);
	}
)

struct GrindPath {
	BoundingSphere bounding_sphere;
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	std::vector<Vec4f> vertices;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(bounding_sphere);
		DEF_FIELD(unknown_4);
		DEF_FIELD(wrap);
		DEF_FIELD(inactive);
		DEF_FIELD(vertices);
		DEF_FIELD(original_index);
	}
};

enum AreaPart {
	AREA_PART_PATHS = 0,
	AREA_PART_CUBOIDS = 1,
	AREA_PART_SPHERES = 2,
	AREA_PART_CYLINDERS = 3,
	AREA_PART_NEG_CUBOIDS = 4
};

struct Area {
	BoundingSphere bounding_sphere;
	s32 last_update_time;
	std::vector<s32> parts[5];
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		std::vector<s32>& paths = parts[AREA_PART_PATHS];
		std::vector<s32>& cuboids = parts[AREA_PART_CUBOIDS];
		std::vector<s32>& spheres = parts[AREA_PART_SPHERES];
		std::vector<s32>& cylinders = parts[AREA_PART_CYLINDERS];
		std::vector<s32>& negative_cuboids = parts[AREA_PART_NEG_CUBOIDS];
		
		DEF_FIELD(bounding_sphere);
		DEF_FIELD(last_update_time);
		DEF_FIELD(paths);
		DEF_FIELD(cuboids);
		DEF_FIELD(spheres);
		DEF_FIELD(cylinders);
		DEF_FIELD(negative_cuboids);
		DEF_FIELD(original_index);
	}
};

struct DirectionalLight {
	Vec4f color_a;
	Vec4f dir_a;
	Vec4f color_b;
	Vec4f dir_b;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(color_a);
		DEF_FIELD(dir_a);
		DEF_FIELD(color_b);
		DEF_FIELD(dir_b);
		DEF_FIELD(original_index);
	}
};

struct TieInstance {
	s32 o_class;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	Mat3 matrix;
	Vec4f position;
	s32 unknown_50;
	s32 uid;
	s32 unknown_58;
	s32 unknown_5c;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(o_class);
		DEF_FIELD(unknown_4);
		DEF_FIELD(unknown_8);
		DEF_FIELD(unknown_c);
		DEF_FIELD(matrix);
		DEF_FIELD(position);
		DEF_FIELD(unknown_50);
		DEF_FIELD(uid);
		DEF_FIELD(unknown_58);
		DEF_FIELD(unknown_5c);
		DEF_FIELD(original_index);
	}
};

struct TieAmbientRgbas {
	s16 id;
	std::vector<u8> data;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(id);
		DEF_HEXDUMP(data);
		DEF_FIELD(original_index);
	}
};

struct TieGroups {
	std::vector<s32> first_part;
	std::vector<s8> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
	}
};

struct ShrubInstance {
	s32 o_class;
	f32 draw_distance;
	s32 unknown_8;
	s32 unknown_c;
	Mat3 matrix;
	Vec4f position;
	Rgb96 light_col;
	s32 unknown_5c;
	s32 unknown_60;
	s32 unknown_64;
	s32 unknown_68;
	s32 unknown_6c;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(o_class);
		DEF_FIELD(draw_distance);
		DEF_FIELD(unknown_8);
		DEF_FIELD(unknown_c);
		DEF_FIELD(matrix);
		DEF_FIELD(position);
		DEF_FIELD(light_col);
		DEF_FIELD(unknown_5c);
		DEF_FIELD(unknown_60);
		DEF_FIELD(unknown_64);
		DEF_FIELD(unknown_68);
		DEF_FIELD(unknown_6c);
		DEF_FIELD(original_index);
	}
};

struct ShrubGroups {
	std::vector<s32> first_part;
	std::vector<s8> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
	}
};

struct OcclusionPair {
	s32 unknown_0;
	s32 unknown_4;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(unknown_0);
		DEF_FIELD(unknown_4);
		DEF_FIELD(original_index);
	}
};

struct OcclusionClusters {
	std::vector<OcclusionPair> first_part;
	std::vector<OcclusionPair> second_part;
	std::vector<OcclusionPair> third_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(first_part);
		DEF_FIELD(second_part);
		DEF_FIELD(third_part);
	}
};

template <typename T>
using Opt = std::optional<T>;

struct Gameplay {
	Opt<std::vector<GC_8c_DL_70>> gc_8c_dl_70;
	Opt<Properties> properties;
	Opt<std::vector<HelpMessage>> us_english_help_messages;
	Opt<std::vector<HelpMessage>> uk_english_help_messages;
	Opt<std::vector<HelpMessage>> french_help_messages;
	Opt<std::vector<HelpMessage>> german_help_messages;
	Opt<std::vector<HelpMessage>> spanish_help_messages;
	Opt<std::vector<HelpMessage>> italian_help_messages;
	Opt<std::vector<HelpMessage>> japanese_help_messages;
	Opt<std::vector<HelpMessage>> korean_help_messages;
	Opt<std::vector<GC_84_Instance>> gc_84;
	Opt<std::vector<ImportCamera>> cameras;
	Opt<std::vector<SoundInstance>> sound_instances;
	Opt<std::vector<s32>> moby_classes;
	Opt<std::vector<MobyInstance>> moby_instances;
	Opt<s32> dynamic_moby_count;
	Opt<std::vector<DL_3c>> gc_58_dl_3c;
	Opt<std::vector<GC_64_DL_48>> gc_64_dl_48;
	Opt<MobyGroups> moby_groups;
	Opt<GC_54_DL_38> gc_54_dl_38;
	Opt<std::vector<Shape>> spheres;
	Opt<std::vector<Shape>> cylinders;
	Opt<std::vector<s32>> gc_74_dl_58;
	Opt<std::vector<Path>> paths;
	Opt<std::vector<Shape>> cuboids;
	Opt<std::vector<u8>> gc_88_dl_6c;
	Opt<GC_80_DL_64> gc_80_dl_64;
	Opt<std::vector<GrindPath>> grind_paths;
	Opt<std::vector<Area>> gameplay_area_list;
	
	Opt<std::vector<DirectionalLight>> lights;
	Opt<std::vector<s32>> tie_classes;
	Opt<std::vector<TieInstance>> tie_instances;
	Opt<std::vector<TieAmbientRgbas>> tie_ambient_rgbas;
	Opt<TieGroups> tie_groups;
	Opt<std::vector<s32>> shrub_classes;
	Opt<std::vector<ShrubInstance>> shrub_instances;
	Opt<ShrubGroups> shrub_groups;
	Opt<OcclusionClusters> occlusion_clusters;
	
	// Only used while reading the binary gameplay file, empty otherwise.
	Opt<std::vector<PvarTableEntry>> pvars_temp;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(gc_8c_dl_70);
		DEF_FIELD(properties);
		DEF_FIELD(us_english_help_messages);
		DEF_FIELD(uk_english_help_messages);
		DEF_FIELD(french_help_messages);
		DEF_FIELD(german_help_messages);
		DEF_FIELD(spanish_help_messages);
		DEF_FIELD(italian_help_messages);
		DEF_FIELD(japanese_help_messages);
		DEF_FIELD(korean_help_messages);
		DEF_FIELD(gc_84);
		DEF_FIELD(cameras);
		DEF_FIELD(sound_instances);
		DEF_FIELD(moby_classes);
		DEF_FIELD(moby_instances);
		DEF_FIELD(dynamic_moby_count);
		DEF_FIELD(gc_58_dl_3c);
		DEF_FIELD(gc_64_dl_48);
		DEF_FIELD(moby_groups);
		DEF_FIELD(gc_54_dl_38);
		DEF_FIELD(spheres);
		DEF_FIELD(cylinders);
		DEF_FIELD(gc_74_dl_58);
		DEF_FIELD(paths);
		DEF_FIELD(cuboids);
		DEF_FIELD(gc_88_dl_6c);
		DEF_FIELD(gc_80_dl_64);
		DEF_FIELD(grind_paths);
		DEF_FIELD(gameplay_area_list);
		DEF_FIELD(lights);
		DEF_FIELD(tie_classes);
		DEF_FIELD(tie_instances);
		DEF_FIELD(tie_ambient_rgbas);
		DEF_FIELD(tie_groups);
		DEF_FIELD(shrub_classes);
		DEF_FIELD(shrub_instances);
		DEF_FIELD(shrub_groups);
		DEF_FIELD(occlusion_clusters);
	}
};

void read_gameplay_json(Gameplay& gameplay, Json& json);
Json write_gameplay_json(Gameplay& gameplay);
void read_help_messages(Gameplay& gameplay, Json& json);
Json write_help_messages(Gameplay& gameplay);

void fixup_pvar_indices(Gameplay& gameplay);

struct Chunk {
	std::vector<u8> tfrags;
	std::vector<u8> collision;
	std::vector<u8> sound_bank;
};

struct Mission {
	std::vector<u8> instances;
	std::vector<u8> classes;
	std::vector<u8> sound_bank;
};

struct LevelWad : Wad {
	s32 level_number;
	std::optional<s32> reverb;
	std::vector<u8> primary;
	std::vector<u8> core_bank;
	Gameplay gameplay;
	std::vector<u8> unknown_28;
	std::map<s32, Chunk> chunks;
	std::map<s32, Mission> missions;
};

std::unique_ptr<Wad> read_wad_json(fs::path src_path);
void write_wad_json(fs::path dest_path, Wad* base);

#endif
