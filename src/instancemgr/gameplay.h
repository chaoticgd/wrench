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

#ifndef INSTANCEMGR_GAMEPLAY_H
#define INSTANCEMGR_GAMEPLAY_H

#include <functional>

#include <core/util.h>
#include <instancemgr/level.h>

struct HelpMessage {
	Opt<std::string> string;
	s32 id;
	s16 short_id;
	s16 third_person_id;
	s16 coop_id;
	s16 vag;
	s16 character;
};

struct Gameplay {
	Opt<std::vector<u8>> rac1_78;
	Opt<std::vector<PointLight>> point_lights;
	Opt<std::vector<EnvSamplePointInstance>> env_sample_points;
	Opt<Properties> properties;
	Opt<std::vector<HelpMessage>> us_english_help_messages;
	Opt<std::vector<HelpMessage>> uk_english_help_messages;
	Opt<std::vector<HelpMessage>> french_help_messages;
	Opt<std::vector<HelpMessage>> german_help_messages;
	Opt<std::vector<HelpMessage>> spanish_help_messages;
	Opt<std::vector<HelpMessage>> italian_help_messages;
	Opt<std::vector<HelpMessage>> japanese_help_messages;
	Opt<std::vector<HelpMessage>> korean_help_messages;
	Opt<std::vector<EnvTransitionInstance>> env_transitions;
	Opt<std::vector<Camera>> cameras;
	Opt<std::vector<SoundInstance>> sound_instances;
	Opt<std::vector<s32>> moby_classes;
	Opt<s32> dynamic_moby_count;
	Opt<std::vector<MobyInstance>> moby_instances;
	Opt<std::vector<Group>> moby_groups;
	Opt<std::vector<u8>> global_pvar;
	Opt<std::vector<Path>> paths;
	Opt<std::vector<Cuboid>> cuboids;
	Opt<std::vector<Sphere>> spheres;
	Opt<std::vector<Cylinder>> cylinders;
	Opt<std::vector<Pill>> pills;
	Opt<GC_80_DL_64> gc_80_dl_64;
	Opt<std::vector<GrindPath>> grind_paths;
	Opt<std::vector<Area>> areas;
	
	Opt<std::vector<DirectionalLight>> lights;
	Opt<std::vector<TieInstance>> tie_instances;
	Opt<std::vector<Group>> tie_groups;
	Opt<std::vector<ShrubInstance>> shrub_instances;
	Opt<std::vector<Group>> shrub_groups;
	Opt<OcclusionMappings> occlusion;
	
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
	void for_each_pvar_instance_const(const PvarTypes& types, Callback callback) const;
	template <typename Callback>
	void for_each_pvar_instance(const PvarTypes& types, Callback callback);
	
	void clear_selection();
	std::vector<InstanceId> selected_instances() const;
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
};

// *****************************************************************************

using GameplayBlockReadFunc = std::function<void(PvarTypes& types, Gameplay& gameplay, Buffer src, Game game)>;
using GameplayBlockWriteFunc = std::function<bool(OutBuffer dest, const PvarTypes& types, const Gameplay& gameplay, Game game)>;

struct GameplayBlockFuncs {
	GameplayBlockReadFunc read;
	GameplayBlockWriteFunc write;
};

struct GameplayBlockDescription {
	s32 header_pointer_offset;
	GameplayBlockFuncs funcs;
	const char* name;
};

extern const std::vector<GameplayBlockDescription> RAC_GAMEPLAY_BLOCKS;
extern const std::vector<GameplayBlockDescription> GC_UYA_GAMEPLAY_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS;

void read_gameplay(Gameplay& gameplay, PvarTypes& types, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks);
std::vector<u8> write_gameplay(const Gameplay& gameplay_arg, const PvarTypes& types, Game game, const std::vector<GameplayBlockDescription>& blocks);
const std::vector<GameplayBlockDescription>* gameplay_block_descriptions_from_game(Game game);
std::vector<u8> write_occlusion_mappings(const Gameplay& gameplay, Game game);

// *****************************************************************************

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
	for_each_instance_of_type_with(required_components_mask, point_lights, callback);
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
void Gameplay::for_each_pvar_instance_const(const PvarTypes& types, Callback callback) const {
	for(const Camera& inst : opt_iterator(cameras)) {
		auto iter = types.camera.find(inst.type);
		if(iter != types.camera.end()) {
			const PvarType& type = iter->second;
			callback(inst, type);
		}
	}
	for(const SoundInstance& inst : opt_iterator(sound_instances)) {
		auto iter = types.sound.find(inst.o_class);
		if(iter != types.sound.end()) {
			const PvarType& type = iter->second;
			callback(inst, type);
		}
	}
	for(const MobyInstance& inst : opt_iterator(moby_instances)) {
		auto iter = types.moby.find(inst.o_class);
		if(iter != types.moby.end()) {
			const PvarType& type = iter->second;
			callback(inst, type);
		}
	}
}

template <typename Callback>
void Gameplay::for_each_pvar_instance(const PvarTypes& types, Callback callback) {
	for_each_pvar_instance_const(types, [&](const Instance& inst, const PvarType& type) {
		callback(const_cast<Instance&>(inst), type);
	});
}

#endif
