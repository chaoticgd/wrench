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
		DEF_PACKED_FIELD(r);
		DEF_PACKED_FIELD(g);
		DEF_PACKED_FIELD(b);
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
		DEF_PACKED_FIELD(background_col);
		DEF_PACKED_FIELD(fog_col);
		DEF_PACKED_FIELD(fog_near_dist);
		DEF_PACKED_FIELD(fog_far_dist);
		DEF_PACKED_FIELD(fog_near_intensity);
		DEF_PACKED_FIELD(fog_far_intensity);
		DEF_PACKED_FIELD(death_height);
		DEF_PACKED_FIELD(is_spherical_world);
		DEF_PACKED_FIELD(sphere_centre);
		DEF_PACKED_FIELD(unknown_3c);
		DEF_PACKED_FIELD(unknown_40);
		DEF_PACKED_FIELD(unknown_44);
		DEF_PACKED_FIELD(unknown_48);
		DEF_PACKED_FIELD(unknown_4c);
		DEF_PACKED_FIELD(unknown_50);
		DEF_PACKED_FIELD(unknown_54);
		DEF_PACKED_FIELD(unknown_58);
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
		DEF_HEXDUMP(pvars);
	}
};

packed_struct(GpShape,
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

packed_struct(GC_84_Instance,
	u8 unknown_0[0x90];
	
	template <typename T>
	void enumerate_fields(T& t) {}
)
static_assert(sizeof(GC_84_Instance) == 0x90);

struct SoundInstance {
	s16 o_class;
	s16 m_class;
	s32 pvar_index; // Only used during reading!
	f32 range;
	GpShape cuboid;
	std::vector<u8> pvars;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(o_class);
		DEF_FIELD(m_class);
		DEF_FIELD(range);
		DEF_FIELD(cuboid);
		DEF_HEXDUMP(pvars);
	}
};

struct MobyInstance {
	s32 size;
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
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(size);
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
	}
};

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

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

struct Gp_GC_80_DL_64 {
	std::vector<u8> first_part;
	std::vector<u8> second_part;
};

packed_struct(GpBoundingSphere,
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
	GpBoundingSphere bounding_sphere;
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	std::vector<Vec4f> vertices;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(bounding_sphere);
		DEF_FIELD(unknown_4);
		DEF_FIELD(wrap);
		DEF_FIELD(inactive);
		DEF_FIELD(vertices);
	}
};

enum AreaPart {
	AREA_PART_PATHS = 0,
	AREA_PART_CUBOIDS = 1,
	AREA_PART_SPHERES = 2,
	AREA_PART_CYLINDERS = 3,
	AREA_PART_NEG_CUBOIDS = 4
};

struct GpArea {
	GpBoundingSphere bounding_sphere;
	s32 last_update_time;
	std::vector<s32> parts[5];
	
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
	}
};

struct GpDirectionalLight {
	Vec4f color_a;
	Vec4f dir_a;
	Vec4f color_b;
	Vec4f dir_b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(color_a);
		DEF_FIELD(dir_a);
		DEF_FIELD(color_b);
		DEF_FIELD(dir_b);
	}
};

packed_struct(GpTieInstance,
	s32 o_class;    // 0x0
	s32 unknown_4;  // 0x4
	s32 unknown_8;  // 0x8
	s32 unknown_c;  // 0xc
	Mat3 matrix;    // 0x10
	Vec4f position; // 0x40
	s32 unknown_50; // 0x50
	s32 uid;        // 0x54
	s32 unknown_58; // 0x58
	s32 unknown_5c; // 0x5c
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(o_class);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(matrix);
		DEF_PACKED_FIELD(position);
		DEF_PACKED_FIELD(unknown_50);
		DEF_PACKED_FIELD(uid);
		DEF_PACKED_FIELD(unknown_58);
		DEF_PACKED_FIELD(unknown_5c);
	}
)
static_assert(sizeof(GpTieInstance) == 0x60);

struct GpTieAmbientRgbas {
	s16 id;
	std::vector<u8> data;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(id);
		DEF_HEXDUMP(data);
	}
};

struct GpTieGroups {
	std::vector<s32> first_part;
	std::vector<s8> second_part;
};

packed_struct(GpShrubInstance,
	s32 o_class;    // 0x0
	f32 unknown_4;  // 0x4
	s32 unknown_8;  // 0x8
	s32 unknown_c;  // 0xc
	Mat3 matrix;    // 0x10
	Vec4f position; // 0x40
	s32 unknown_50; // 0x50
	s32 unknown_54; // 0x54
	s32 unknown_58; // 0x58
	s32 unknown_5c; // 0x5c
	s32 unknown_60; // 0x60
	s32 unknown_64; // 0x64
	s32 unknown_68; // 0x68
	s32 unknown_6c; // 0x6c
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(o_class);
		DEF_PACKED_FIELD(unknown_4);
		DEF_PACKED_FIELD(unknown_8);
		DEF_PACKED_FIELD(unknown_c);
		DEF_PACKED_FIELD(matrix);
		DEF_PACKED_FIELD(position);
		DEF_PACKED_FIELD(unknown_50);
		DEF_PACKED_FIELD(unknown_54);
		DEF_PACKED_FIELD(unknown_58);
		DEF_PACKED_FIELD(unknown_5c);
		DEF_PACKED_FIELD(unknown_60);
		DEF_PACKED_FIELD(unknown_64);
		DEF_PACKED_FIELD(unknown_68);
		DEF_PACKED_FIELD(unknown_6c);
	}
)

struct GpShrubGroups {
	std::vector<s32> first_part;
	std::vector<s8> second_part;
};

packed_struct(OcclusionPair,
	s32 unknown_0;
	s32 unknown_4;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_PACKED_FIELD(unknown_0);
		DEF_PACKED_FIELD(unknown_4);
	}
)

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
	Opt<std::vector<GC_84_Instance>> gc_84;
	
	// Deadlocked gameplay core
	Opt<std::vector<Gp_GC_8c_DL_70>> gc_8c_dl_70;
	Opt<GpProperties> properties;
	Opt<std::vector<GpString>> us_english_strings;
	Opt<std::vector<GpString>> uk_english_strings;
	Opt<std::vector<GpString>> french_strings;
	Opt<std::vector<GpString>> german_strings;
	Opt<std::vector<GpString>> spanish_strings;
	Opt<std::vector<GpString>> italian_strings;
	Opt<std::vector<GpString>> japanese_strings;
	Opt<std::vector<GpString>> korean_strings;
	Opt<std::vector<ImportCamera>> cameras;
	Opt<std::vector<SoundInstance>> sound_instances;
	Opt<std::vector<s32>> moby_classes;
	Opt<std::vector<MobyInstance>> moby_instances;
	Opt<s32> dynamic_moby_count;
	Opt<std::vector<Gp_DL_3c>> gc_58_dl_3c;
	Opt<std::vector<Gp_GC_64_DL_48>> gc_64_dl_48;
	Opt<GpMobyGroups> moby_groups;
	Opt<Gp_GC_54_DL_38> gc_54_dl_38;
	Opt<std::vector<GpShape>> spheres;
	Opt<std::vector<GpShape>> cylinders;
	Opt<std::vector<s32>> gc_74_dl_58;
	Opt<std::vector<std::vector<Vec4f>>> paths;
	Opt<std::vector<GpShape>> cuboids;
	Opt<std::vector<u8>> gc_88_dl_6c;
	Opt<Gp_GC_80_DL_64> gc_80_dl_64;
	Opt<std::vector<GrindPath>> grindpaths;
	Opt<std::vector<GpArea>> gameplay_area_list;
	
	// Deadlocked art instances
	Opt<std::vector<GpDirectionalLight>> lights;
	Opt<std::vector<s32>> tie_classes;
	Opt<std::vector<GpTieInstance>> tie_instances;
	Opt<std::vector<GpTieAmbientRgbas>> tie_ambient_rgbas;
	Opt<GpTieGroups> tie_groups;
	Opt<std::vector<s32>> shrub_classes;
	Opt<std::vector<GpShrubInstance>> shrub_instances;
	Opt<GpShrubGroups> shrub_groups;
	Opt<OcclusionClusters> occlusion_clusters;
	
	// Only used while reading the binary gameplay file, empty otherwise.
	Opt<std::vector<PvarTableEntry>> pvars_temp;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_OPTIONAL_FIELD(gc_84);
		
		DEF_OPTIONAL_FIELD(properties);
		DEF_OPTIONAL_FIELD(cameras);
		DEF_OPTIONAL_FIELD(sound_instances);
		DEF_OPTIONAL_FIELD(moby_instances);
		DEF_OPTIONAL_FIELD(spheres);
		DEF_OPTIONAL_FIELD(cylinders);
		DEF_OPTIONAL_FIELD(paths);
		DEF_OPTIONAL_FIELD(cuboids);
		DEF_OPTIONAL_FIELD(grindpaths);
		DEF_OPTIONAL_FIELD(gameplay_area_list);
		
		DEF_OPTIONAL_FIELD(lights);
		DEF_OPTIONAL_FIELD(tie_classes);
		DEF_OPTIONAL_FIELD(tie_instances);
		DEF_OPTIONAL_FIELD(tie_ambient_rgbas);
		DEF_OPTIONAL_FIELD(shrub_classes);
		DEF_OPTIONAL_FIELD(shrub_instances);
		DEF_OPTIONAL_FIELD(occlusion_clusters);
	}
};

struct LevelWad : Wad {
	Gameplay gameplay;
	std::vector<Gameplay> gameplay_mission_instances;
};

Json write_gameplay_json(Gameplay& gameplay);

#endif
