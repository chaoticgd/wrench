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
	s16 unknown_0;
	s16 unknown_2;
	s16 unknown_4;
	s16 unknown_6;
	u32 unknown_8;
	s16 unknown_c;
	s16 unknown_e;
	s8 unknown_10;
	s8 unknown_11;
	s16 unknown_12;
	u32 unknown_14;
	u32 unknown_18;
	s16 unknown_1c;
	s16 unknown_1e;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_2);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_6);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(unknown_e);
		DEF_PACKED_FIELD(unknown_10);
		DEF_PACKED_FIELD(unknown_11);
		DEF_PACKED_FIELD(unknown_12);
		DEF_PACKED_FIELD(unknown_14);
		DEF_PACKED_FIELD(unknown_18);
		DEF_PACKED_FIELD(unknown_1c);
		DEF_PACKED_FIELD(unknown_1e);
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
	Opt<s32> rac3_third_part;
	std::vector<PropertiesThirdPart> third_part;
	Opt<PropertiesFourthPart> fourth_part;
	Opt<PropertiesFifthPart> fifth_part;
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
		t.encoded_string("string", string);
		DEF_FIELD(id);
		DEF_FIELD(short_id);
		DEF_FIELD(third_person_id);
		DEF_FIELD(coop_id);
		DEF_FIELD(vag);
		DEF_FIELD(character);
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

struct LightTriggerInstance {
	Vec4f point;
	Mat3 matrix;
	Vec4f point_2;
	s32 unknown_40;
	s32 unknown_44;
	s32 light_index;
	s32 unknown_4c;
	s32 unknown_50;
	s32 unknown_54;
	s32 unknown_58;
	s32 unknown_5c;
	s32 unknown_60;
	s32 unknown_64;
	s32 unknown_68;
	s32 unknown_6c;
	s32 unknown_70;
	s32 unknown_74;
	s32 unknown_78;
	s32 unknown_7c;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(point);
		DEF_FIELD(matrix);
		DEF_FIELD(point_2);
		DEF_FIELD(unknown_40);
		DEF_FIELD(unknown_44);
		DEF_FIELD(light_index);
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
	s32 draw_distance;
	s32 occlusion_index;
	Mat3 matrix;
	Vec4f position;
	s32 directional_lights;
	s32 uid;
	OriginalIndex original_index;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(o_class);
		DEF_FIELD(draw_distance);
		DEF_FIELD(occlusion_index);
		DEF_FIELD(matrix);
		DEF_FIELD(position);
		DEF_FIELD(directional_lights);
		DEF_FIELD(uid);
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
	Opt<std::vector<LightTriggerInstance>> light_triggers;
	Opt<std::vector<ImportCamera>> cameras;
	Opt<std::vector<SoundInstance>> sound_instances;
	Opt<std::vector<s32>> moby_classes;
	Opt<std::vector<MobyInstance>> moby_instances;
	Opt<s32> dynamic_moby_count;
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
	Opt<std::vector<Area>> areas;
	
	Opt<std::vector<DirectionalLight>> lights;
	Opt<std::vector<s32>> tie_classes;
	Opt<std::vector<TieInstance>> tie_instances;
	Opt<std::vector<TieAmbientRgbas>> tie_ambient_rgbas;
	Opt<TieGroups> tie_groups;
	Opt<std::vector<s32>> shrub_classes;
	Opt<std::vector<ShrubInstance>> shrub_instances;
	Opt<ShrubGroups> shrub_groups;
	Opt<OcclusionClusters> occlusion_clusters;
	
	// Only used while reading the binary gameplay file.
	Opt<std::vector<PvarTableEntry>> pvars_temp;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(gc_8c_dl_70);
		DEF_FIELD(properties);
		DEF_FIELD(light_triggers);
		DEF_FIELD(cameras);
		DEF_FIELD(sound_instances);
		DEF_FIELD(moby_classes);
		DEF_FIELD(moby_instances);
		DEF_FIELD(dynamic_moby_count);
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
		DEF_FIELD(areas);
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

struct HelpMessages {
	Opt<std::vector<HelpMessage>> us_english;
	Opt<std::vector<HelpMessage>> uk_english;
	Opt<std::vector<HelpMessage>> french;
	Opt<std::vector<HelpMessage>> german;
	Opt<std::vector<HelpMessage>> spanish;
	Opt<std::vector<HelpMessage>> italian;
	Opt<std::vector<HelpMessage>> japanese;
	Opt<std::vector<HelpMessage>> korean;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(us_english);
		DEF_FIELD(uk_english);
		DEF_FIELD(french);
		DEF_FIELD(german);
		DEF_FIELD(spanish);
		DEF_FIELD(italian);
		DEF_FIELD(japanese);
		DEF_FIELD(korean);
	}
	
	void swap(Gameplay& gameplay) {
		us_english.swap(gameplay.us_english_help_messages);
		uk_english.swap(gameplay.uk_english_help_messages);
		french.swap(gameplay.french_help_messages);
		german.swap(gameplay.german_help_messages);
		spanish.swap(gameplay.spanish_help_messages);
		italian.swap(gameplay.italian_help_messages);
		japanese.swap(gameplay.japanese_help_messages);
		korean.swap(gameplay.korean_help_messages);
	}
};

Json write_gameplay_json(Gameplay& gameplay);
Json write_help_messages_json(HelpMessages& help_messages);

struct Chunk {
	Opt<std::vector<u8>> tfrags;
	Opt<std::vector<u8>> collision;
	Opt<std::vector<u8>> sound_bank;
};

struct Mission {
	Opt<std::vector<u8>> instances;
	Opt<std::vector<u8>> classes;
	Opt<std::vector<u8>> sound_bank;
};

struct CameraClass {
	std::string pvar_type;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(pvar_type);
	}
};

struct SoundClass {
	std::string pvar_type;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(pvar_type);
	}
};

struct MobyClass {
	std::string pvar_type;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(pvar_type);
	}
};

enum PvarFieldDescriptor {
	PVAR_INTEGERS_BEGIN = 0,
	PVAR_S8 = 1, PVAR_S16 = 2, PVAR_S32 = 3,
	PVAR_U8 = 4, PVAR_U16 = 5, PVAR_U32 = 6,
	PVAR_INTEGERS_END = 7,
	PVAR_F32 = 8,
	PVAR_RUNTIME_POINTER = 100,
	PVAR_RELATIVE_POINTER = 101,
	PVAR_SCRATCHPAD_POINTER = 102,
	PVAR_STRUCT = 103,
};

std::string pvar_descriptor_to_string(PvarFieldDescriptor descriptor);
PvarFieldDescriptor pvar_string_to_descriptor(std::string str);

struct PvarField {
	s32 offset;
	std::string name;
	PvarFieldDescriptor descriptor = PVAR_U8;
	std::string value_type; // Only set for PVAR_RELATIVE_POINTER and PVAR_STRUCT.
	
	s32 size() const;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(offset);
		DEF_FIELD(name);
		std::string type = pvar_descriptor_to_string(descriptor);
		DEF_FIELD(type);
		descriptor = pvar_string_to_descriptor(type);
		if(descriptor == PVAR_RELATIVE_POINTER || descriptor == PVAR_STRUCT) {
			DEF_FIELD(value_type);
		}
	}
};

struct PvarType {
	std::vector<PvarField> fields;
	
	bool insert_field(PvarField to_insert, bool sort);
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(fields);
	}
};

struct LevelWad : Wad {
	s32 level_number;
	std::optional<s32> reverb;
	std::vector<u8> primary;
	std::vector<u8> core_bank;
	Gameplay gameplay;
	HelpMessages help_messages;
	std::map<s32, Chunk> chunks;
	std::map<s32, Mission> missions;
	std::map<s32, CameraClass> camera_classes;
	std::map<s32, SoundClass> sound_classes;
	std::map<s32, MobyClass> moby_classes;
	std::map<std::string, PvarType> pvar_types;
	
	CameraClass& lookup_camera_class(s32 class_number);
	SoundClass& lookup_sound_class(s32 class_number);
	MobyClass& lookup_moby_class(s32 class_number);
};

std::unique_ptr<Wad> read_wad_json(fs::path src_path);
void write_wad_json(fs::path dest_path, Wad* base);

#endif
