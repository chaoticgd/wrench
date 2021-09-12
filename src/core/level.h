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

#ifndef LEVEL_H
#define LEVEL_H

#include "json.h"
#include "util.h"
#include "collada.h"
#include "buffer.h"
#include "instance.h"
#include "texture.h"

struct RAC1_88 : Instance {
	RAC1_88() : Instance(INST_RAC1_88, COM_NONE, TransformMode::NONE) {}
	
	u32 unknown_0;
	u32 unknown_4;
	u32 unknown_8;
	u32 unknown_c;
	u32 unknown_10;
	u32 unknown_14;
	u32 unknown_18;
	u32 unknown_1c;
	u32 unknown_20;
	u32 unknown_24;
	u32 unknown_28;
	u32 unknown_2c;
	
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
	}
};

struct RAC1_7c : Instance {
	RAC1_7c() : Instance(INST_RAC1_7c, COM_NONE, TransformMode::NONE) {}
	
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
		DEF_FIELD(unknown_0);
		DEF_FIELD(unknown_4);
		DEF_FIELD(unknown_8);
		DEF_FIELD(unknown_c);
		DEF_FIELD(unknown_10);
		DEF_FIELD(unknown_14);
		DEF_FIELD(unknown_18);
		DEF_FIELD(unknown_1c);
	}
};

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

struct PropertiesFirstPart {
	Rgb96 background_colour;
	Rgb96 fog_colour;
	f32 fog_near_distance;
	f32 fog_far_distance;
	f32 fog_near_intensity;
	f32 fog_far_intensity;
	f32 death_height;
	Opt<s32> is_spherical_world;
	Opt<glm::vec3> sphere_centre;
	glm::vec3 ship_position;
	f32 ship_rotation_z;
	Rgb96 unknown_colour;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(background_colour);
		DEF_FIELD(fog_colour);
		DEF_FIELD(fog_near_distance);
		DEF_FIELD(fog_far_distance);
		DEF_FIELD(fog_near_intensity);
		DEF_FIELD(fog_far_intensity);
		DEF_FIELD(death_height);
		DEF_FIELD(is_spherical_world);
		DEF_FIELD(sphere_centre);
		DEF_FIELD(ship_position);
		DEF_FIELD(ship_rotation_z);
		DEF_FIELD(unknown_colour);
	}
};

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
	Opt<std::vector<PropertiesSecondPart>> second_part;
	Opt<s32> core_sounds_count;
	Opt<s32> rac3_third_part;
	Opt<std::vector<PropertiesThirdPart>> third_part;
	Opt<PropertiesFourthPart> fourth_part;
	Opt<PropertiesFifthPart> fifth_part;
	Opt<std::vector<s8>> sixth_part;
	
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
	s32 id;
	s16 short_id;
	s16 third_person_id;
	s16 coop_id;
	s16 vag;
	s16 character;
	
	template <typename T>
	void enumerate_fields(T& t) {
		t.encoded_string("string", string);
		DEF_FIELD(id);
		DEF_FIELD(short_id);
		DEF_FIELD(third_person_id);
		DEF_FIELD(coop_id);
		DEF_FIELD(vag);
		DEF_FIELD(character);
	}
};

struct LightTriggerInstance {
	s32 id;
	Vec4f point;
	Mat3 matrix;
	Vec4f point_2;
	s32 unknown_40;
	s32 unknown_44;
	s32 light;
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
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(id);
		DEF_FIELD(point);
		DEF_FIELD(matrix);
		DEF_FIELD(point_2);
		DEF_FIELD(unknown_40);
		DEF_FIELD(unknown_44);
		DEF_FIELD(light);
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
	}
};

struct Camera : Instance {
	Camera() : Instance(INST_CAMERA, COM_TRANSFORM | COM_PVARS, TransformMode::POSITION_ROTATION) {}
	s32 type;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(type);
	}
};

struct Cuboid : Instance {
	Cuboid() : Instance(INST_SPHERE, COM_TRANSFORM, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

struct Sphere : Instance {
	Sphere() : Instance(INST_SPHERE, COM_TRANSFORM, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

struct Cylinder : Instance {
	Cylinder() : Instance(INST_SPHERE, COM_TRANSFORM, TransformMode::MATRIX_INVERSE_ROTATION) {}
};

struct SoundInstance : Instance {
	SoundInstance() : Instance(INST_SOUND, COM_TRANSFORM | COM_PVARS, TransformMode::MATRIX_INVERSE_ROTATION) {}
	s16 o_class;
	s16 m_class;
	f32 range;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(o_class);
		DEF_FIELD(m_class);
		DEF_FIELD(range);
	}
};

struct MobyInstance : Instance {
	MobyInstance() : Instance(INST_MOBY, COM_TRANSFORM | COM_PVARS | COM_DRAW_DISTANCE | COM_COLOUR, TransformMode::POSITION_ROTATION_SCALE) {}
	s8 mission;
	s32 uid;
	s32 bolts;
	s32 o_class;
	s32 update_distance;
	s32 group;
	bool is_rooted;
	f32 rooted_distance;
	s32 occlusion;
	s32 mode_bits;
	s32 light;
	s32 rac1_unknown_4;
	s32 rac1_unknown_8;
	s32 rac1_unknown_c;
	s32 rac1_unknown_10;
	s32 rac1_unknown_14;
	s32 rac1_unknown_18;
	s32 rac1_unknown_1c;
	s32 rac1_unknown_20;
	s32 rac1_unknown_24;
	s32 rac1_unknown_28;
	s32 rac1_unknown_2c;
	s32 rac1_unknown_54;
	s32 rac1_unknown_5c;
	s32 rac1_unknown_60;
	s32 rac1_unknown_70;
	s32 rac1_unknown_74;
	s32 rac23_unknown_8;
	s32 rac23_unknown_c;
	s32 rac23_unknown_18;
	s32 rac23_unknown_1c;
	s32 rac23_unknown_20;
	s32 rac23_unknown_24;
	s32 rac23_unknown_38;
	s32 rac23_unknown_3c;
	s32 rac23_unknown_4c;
	s32 rac23_unknown_84;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(mission);
		DEF_FIELD(uid);
		DEF_FIELD(bolts);
		DEF_FIELD(o_class);
		DEF_FIELD(update_distance);
		DEF_FIELD(group);
		DEF_FIELD(is_rooted);
		DEF_FIELD(rooted_distance);
		DEF_FIELD(occlusion);
		DEF_FIELD(mode_bits);
		DEF_FIELD(light);
		DEF_FIELD(rac1_unknown_4);
		DEF_FIELD(rac1_unknown_8);
		DEF_FIELD(rac1_unknown_c);
		DEF_FIELD(rac1_unknown_10);
		DEF_FIELD(rac1_unknown_14);
		DEF_FIELD(rac1_unknown_18);
		DEF_FIELD(rac1_unknown_1c);
		DEF_FIELD(rac1_unknown_20);
		DEF_FIELD(rac1_unknown_24);
		DEF_FIELD(rac1_unknown_28);
		DEF_FIELD(rac1_unknown_2c);
		DEF_FIELD(rac1_unknown_54);
		DEF_FIELD(rac1_unknown_5c);
		DEF_FIELD(rac1_unknown_60);
		DEF_FIELD(rac1_unknown_70);
		DEF_FIELD(rac1_unknown_74);
		DEF_FIELD(rac23_unknown_8);
		DEF_FIELD(rac23_unknown_c);
		DEF_FIELD(rac23_unknown_18);
		DEF_FIELD(rac23_unknown_1c);
		DEF_FIELD(rac23_unknown_20);
		DEF_FIELD(rac23_unknown_24);
		DEF_FIELD(rac23_unknown_38);
		DEF_FIELD(rac23_unknown_3c);
		DEF_FIELD(rac23_unknown_4c);
		DEF_FIELD(rac23_unknown_84);
	}
};

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

struct Group {
	s32 id;
	std::vector<u16> members;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(id);
		DEF_FIELD(members);
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

struct Path : Instance {
	Path() : Instance(INST_PATH, COM_SPLINE, TransformMode::NONE) {}
};

struct GC_80_DL_64 {
	std::vector<u8> first_part;
	std::vector<u8> second_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_HEXDUMP(first_part);
		DEF_HEXDUMP(second_part);
	}
};

struct GrindPath : Instance {
	GrindPath() : Instance(INST_GRIND_PATH, COM_SPLINE | COM_BOUNDING_SPHERE, TransformMode::NONE) {}
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(unknown_4);
		DEF_FIELD(wrap);
		DEF_FIELD(inactive);
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
	s32 id;
	glm::vec4 bounding_sphere;
	s32 last_update_time;
	std::vector<s32> parts[5];
	
	template <typename T>
	void enumerate_fields(T& t) {
		std::vector<s32>& paths = parts[AREA_PART_PATHS];
		std::vector<s32>& cuboids = parts[AREA_PART_CUBOIDS];
		std::vector<s32>& spheres = parts[AREA_PART_SPHERES];
		std::vector<s32>& cylinders = parts[AREA_PART_CYLINDERS];
		std::vector<s32>& negative_cuboids = parts[AREA_PART_NEG_CUBOIDS];
		
		DEF_FIELD(id);
		DEF_FIELD(bounding_sphere);
		DEF_FIELD(last_update_time);
		DEF_FIELD(paths);
		DEF_FIELD(cuboids);
		DEF_FIELD(spheres);
		DEF_FIELD(cylinders);
		DEF_FIELD(negative_cuboids);
	}
};

struct DirectionalLight : Instance {
	DirectionalLight() : Instance(INST_LIGHT, COM_NONE, TransformMode::NONE) {}
	glm::vec4 colour_a;
	glm::vec4 direction_a;
	glm::vec4 colour_b;
	glm::vec4 direction_b;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(colour_a);
		DEF_FIELD(direction_a);
		DEF_FIELD(colour_b);
		DEF_FIELD(direction_b);
	}
};

struct TieInstance : Instance {
	TieInstance() : Instance(INST_TIE, COM_TRANSFORM | COM_DRAW_DISTANCE, TransformMode::MATRIX) {}
	s32 o_class;
	s32 occlusion_index;
	s32 directional_lights;
	s32 uid;
	std::vector<u8> ambient_rgbas;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(o_class);
		DEF_FIELD(occlusion_index);
		DEF_FIELD(directional_lights);
		DEF_FIELD(uid);
		DEF_HEXDUMP(ambient_rgbas);
	}
};

struct ShrubInstance : Instance {
	ShrubInstance() : Instance(INST_SHRUB, COM_TRANSFORM | COM_DRAW_DISTANCE | COM_COLOUR, TransformMode::MATRIX) {}
	s32 o_class;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_5c;
	s32 unknown_60;
	s32 unknown_64;
	s32 unknown_68;
	s32 unknown_6c;
	
	template <typename T>
	void enumerate_fields(T& t) {
		Instance::enumerate_fields(t);
		DEF_FIELD(o_class);
		DEF_FIELD(unknown_8);
		DEF_FIELD(unknown_c);
		DEF_FIELD(unknown_5c);
		DEF_FIELD(unknown_60);
		DEF_FIELD(unknown_64);
		DEF_FIELD(unknown_68);
		DEF_FIELD(unknown_6c);
	}
};

struct Occlusion {
	std::vector<u8> first_part;
	std::vector<u8> second_part;
	std::vector<u8> third_part;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_HEXDUMP(first_part);
		DEF_HEXDUMP(second_part);
		DEF_HEXDUMP(third_part);
	}
};

struct LevelWad;

struct Gameplay {
	Opt<std::vector<RAC1_88>> rac1_88;
	Opt<std::vector<u8>> rac1_78;
	Opt<std::vector<RAC1_7c>> rac1_7c;
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
	Opt<std::vector<Camera>> cameras;
	Opt<std::vector<SoundInstance>> sound_instances;
	Opt<std::vector<s32>> moby_classes;
	Opt<s32> dynamic_moby_count;
	Opt<std::vector<MobyInstance>> moby_instances;
	Opt<std::vector<Group>> moby_groups;
	Opt<std::vector<u8>> global_pvar;
	Opt<std::vector<s32>> gc_74_dl_58;
	Opt<std::vector<Path>> paths;
	Opt<std::vector<Cuboid>> cuboids;
	Opt<std::vector<Sphere>> spheres;
	Opt<std::vector<Cylinder>> cylinders;
	Opt<std::vector<u8>> gc_88_dl_6c;
	Opt<GC_80_DL_64> gc_80_dl_64;
	Opt<std::vector<GrindPath>> grind_paths;
	Opt<std::vector<Area>> areas;
	
	Opt<std::vector<DirectionalLight>> lights;
	Opt<std::vector<TieInstance>> tie_instances;
	Opt<std::vector<Group>> tie_groups;
	Opt<std::vector<ShrubInstance>> shrub_instances;
	Opt<std::vector<Group>> shrub_groups;
	Opt<Occlusion> occlusion;
	
	// Only used while reading the binary gameplay file.
	Opt<std::vector<PvarTableEntry>> pvars_temp;
	
	template <typename Callback>
	void for_each_instance_with(u32 required_components_mask, Callback callback) const;
	template <typename Callback>
	void for_each_instance_with(u32 required_components_mask, Callback callback);
	
	template <typename Callback>
	void for_each_instance(Callback callback) const {
		for_each_instance_with(COM_NONE, callback);
	}
	template <typename Callback>
	void for_each_instance(Callback callback) {
		for_each_instance_with(COM_NONE, callback);
	}
	
	// These functions don't skip over instances where pvars.size() == 0.
	template <typename Callback>
	void for_each_pvar_instance_const(Callback callback) const;
	template <typename Callback>
	void for_each_pvar_instance(Callback callback);
	// And these skip over instances that don't have an associated pvar type.
	template <typename Callback>
	void for_each_pvar_instance_const(const LevelWad& wad, Callback callback) const;
	template <typename Callback>
	void for_each_pvar_instance(const LevelWad& wad, Callback callback);
	
	void clear_selection();
	std::vector<InstanceId> selected_instances() const;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(rac1_88);
		DEF_HEXDUMP(rac1_78);
		DEF_FIELD(rac1_7c);
		DEF_FIELD(gc_8c_dl_70);
		DEF_FIELD(properties);
		DEF_FIELD(light_triggers);
		DEF_FIELD(cameras);
		DEF_FIELD(sound_instances);
		DEF_FIELD(moby_classes);
		DEF_FIELD(dynamic_moby_count);
		DEF_FIELD(moby_instances);
		DEF_FIELD(moby_groups);
		DEF_HEXDUMP(global_pvar);
		DEF_FIELD(gc_74_dl_58);
		DEF_FIELD(paths);
		DEF_FIELD(cuboids);
		DEF_FIELD(spheres);
		DEF_FIELD(cylinders);
		DEF_HEXDUMP(gc_88_dl_6c);
		DEF_FIELD(gc_80_dl_64);
		DEF_FIELD(grind_paths);
		DEF_FIELD(areas);
		DEF_FIELD(lights);
		DEF_FIELD(tie_instances);
		DEF_FIELD(tie_groups);
		DEF_FIELD(shrub_instances);
		DEF_FIELD(shrub_groups);
		DEF_FIELD(occlusion);
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
};

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
	static std::string get_pvar_type(s32 o_class);
	
	template <typename T>
	void enumerate_fields(T& t) {}
};

struct SoundClass {
	static std::string get_pvar_type(s32 o_class);
	
	template <typename T>
	void enumerate_fields(T& t) {}
};

struct Class {
	s32 o_class;
};

struct MobyClass : Class {
	Opt<std::vector<u8>> model;
	std::vector<Texture> textures;
	bool has_asset_table_entry = false;
	
	static std::string get_pvar_type(s32 o_class);
};

struct TieClass : Class {
	std::vector<u8> model;
	std::vector<Texture> textures;
};

struct ShrubClass : Class {
	std::vector<u8> model;
	std::vector<Texture> textures;
};

Json get_file_metadata(const char* format, const char* application);

enum PvarFieldDescriptor {
	PVAR_INTEGERS_BEGIN = 0,
	PVAR_S8 = 1, PVAR_S16 = 2, PVAR_S32 = 3,
	PVAR_U8 = 4, PVAR_U16 = 5, PVAR_U32 = 6,
	PVAR_INTEGERS_END = 7,
	PVAR_F32 = 8,
	PVAR_POINTERS_BEGIN = 100,
	PVAR_RUNTIME_POINTER = 101,
	PVAR_RELATIVE_POINTER = 102,
	PVAR_SCRATCHPAD_POINTER = 103,
	PVAR_GLOBAL_PVAR_POINTER = 104,
	PVAR_POINTERS_END = 105,
	PVAR_STRUCT = 106
};

std::string pvar_descriptor_to_string(PvarFieldDescriptor descriptor);
PvarFieldDescriptor pvar_string_to_descriptor(std::string str);

struct PvarField {
	s32 offset;
	std::string name;
	PvarFieldDescriptor descriptor = PVAR_U8;
	std::string value_type; // Only set for pointer types.
	
	s32 size() const;
	
	template <typename T>
	void enumerate_fields(T& t) {
		DEF_FIELD(offset);
		DEF_FIELD(name);
		std::string type = pvar_descriptor_to_string(descriptor);
		DEF_FIELD(type);
		descriptor = pvar_string_to_descriptor(type);
		if(descriptor > PVAR_POINTERS_BEGIN || descriptor < PVAR_POINTERS_END) {
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
	// Primary lump
	std::vector<u8> code;
	std::vector<u8> asset_header;
	std::vector<u8> hud_header;
	std::vector<u8> hud_banks[5];
	std::vector<u8> tfrags;
	std::vector<u8> occlusion;
	std::vector<u8> sky;
	Mesh collision;
	std::vector<u8> collision_bin;
	std::vector<Texture> tfrag_textures;
	std::vector<Texture> particle_textures;
	std::vector<Texture> fx_textures;
	std::vector<MobyClass> moby_classes;
	std::vector<TieClass> tie_classes;
	std::vector<ShrubClass> shrub_classes;
	std::vector<u8> particle_defs;
	std::vector<u8> sound_remap;
	std::vector<u8> light_cuboids;
	Opt<std::vector<u8>> transition_textures;
	Opt<std::vector<u8>> moby8355_pvars;
	Opt<std::vector<u8>> global_nav_data;
	std::vector<u8> core_bank;
	std::map<s32, CameraClass> camera_classes;
	std::map<s32, SoundClass> sound_classes;
	std::map<std::string, PvarType> pvar_types;
	HelpMessages help_messages;
	Gameplay gameplay;
	std::map<s32, Chunk> chunks;
	std::map<s32, Mission> missions;
};

Opt<Game> game_from_string(std::string str);
std::unique_ptr<Wad> read_wad_json(fs::path src_path);
void write_wad_json(fs::path dest_path, Wad* base);

template <typename Callback, typename InstanceVec>
static void for_each_instance_of_type_with(u32 required_components_mask, const InstanceVec& instances, Callback callback) {
	if(instances.has_value() && instances->size() > 0) {
		if(((*instances)[0].components_mask() & required_components_mask) == required_components_mask) {
			for(const Instance& instance : *instances) {
				callback(instance);
			}
		}
	}
}

template <typename Callback>
void Gameplay::for_each_instance_with(u32 required_components_mask, Callback callback) const {
	for_each_instance_of_type_with(required_components_mask, cameras, callback);
	for_each_instance_of_type_with(required_components_mask, sound_instances, callback);
	for_each_instance_of_type_with(required_components_mask, moby_instances, callback);
	for_each_instance_of_type_with(required_components_mask, spheres, callback);
	for_each_instance_of_type_with(required_components_mask, cylinders, callback);
	for_each_instance_of_type_with(required_components_mask, paths, callback);
	for_each_instance_of_type_with(required_components_mask, cuboids, callback);
	for_each_instance_of_type_with(required_components_mask, grind_paths, callback);
	for_each_instance_of_type_with(required_components_mask, lights, callback);
	for_each_instance_of_type_with(required_components_mask, tie_instances, callback);
	for_each_instance_of_type_with(required_components_mask, shrub_instances, callback);
}

template <typename Callback>
void Gameplay::for_each_instance_with(u32 required_components_mask, Callback callback) {
	static_cast<const Gameplay*>(this)->for_each_instance_with(required_components_mask, [&](const Instance& inst) {
		callback(const_cast<Instance&>(inst));
	});
}

template <typename Callback>
void Gameplay::for_each_pvar_instance_const(Callback callback) const {
	for(const Camera& inst : opt_iterator(cameras)) {
		callback(inst);
	}
	for(const SoundInstance& inst : opt_iterator(sound_instances)) {
		callback(inst);
	}
	for(const MobyInstance& inst : opt_iterator(moby_instances)) {
		callback(inst);
	}
}

template <typename Callback>
void Gameplay::for_each_pvar_instance(Callback callback) {
	for_each_pvar_instance_const([&](const Instance& inst) {
		callback(const_cast<Instance&>(inst));
	});
}

template <typename Callback>
void Gameplay::for_each_pvar_instance_const(const LevelWad& wad, Callback callback) const {
	for(const Camera& inst : opt_iterator(cameras)) {
		std::string type_name = CameraClass::get_pvar_type(inst.type);
		auto iter = wad.pvar_types.find(type_name);
		if(iter != wad.pvar_types.end()) {
			const PvarType& type = iter->second;
			callback(inst, type);
		}
	}
	for(const SoundInstance& inst : opt_iterator(sound_instances)) {
		std::string type_name = SoundClass::get_pvar_type(inst.o_class);
		auto iter = wad.pvar_types.find(type_name);
		if(iter != wad.pvar_types.end()) {
			const PvarType& type = iter->second;
			callback(inst, type);
		}
	}
	for(const MobyInstance& inst : opt_iterator(moby_instances)) {
		std::string type_name = MobyClass::get_pvar_type(inst.o_class);
		auto iter = wad.pvar_types.find(type_name);
		if(iter != wad.pvar_types.end()) {
			const PvarType& type = iter->second;
			callback(inst, type);
		}
	}
}

template <typename Callback>
void Gameplay::for_each_pvar_instance(const LevelWad& wad, Callback callback) {
	for_each_pvar_instance_const(wad, [&](const Instance& inst, const PvarType& type) {
		callback(const_cast<Instance&>(inst), type);
	});
}

#endif
