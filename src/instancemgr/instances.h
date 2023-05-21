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

#ifndef INSTANCEMGR_INSTANCES_H
#define INSTANCEMGR_INSTANCES_H

#include <instancemgr/level.h>
#include <instancemgr/instance.h>

// Represents a gameplay source file.
struct Instances {
	LevelSettings level_settings;
	
	// objects
	InstanceList<MobyInstance> moby_instances;
	s32 dynamic_moby_count;
	std::vector<s32> moby_classes;
	std::vector<Group> moby_groups;
	InstanceList<TieInstance> tie_instances;
	std::vector<Group> tie_groups;
	InstanceList<ShrubInstance> shrub_instances;
	std::vector<Group> shrub_groups;
	std::vector<u8> global_pvar;
	
	// environment/lighting
	InstanceList<DirLightInstance> dir_lights;
	InstanceList<PointLightInstance> point_lights;
	InstanceList<EnvSamplePointInstance> env_sample_points;
	InstanceList<EnvTransitionInstance> env_transitions;
	
	// volumes
	InstanceList<CuboidInstance> cuboids;
	InstanceList<SphereInstance> spheres;
	InstanceList<CylinderInstance> cylinders;
	InstanceList<PillInstance> pills;
	
	// misc
	InstanceList<CameraInstance> cameras;
	InstanceList<SoundInstance> sound_instances;
	InstanceList<PathInstance> paths;
	InstanceList<GrindPathInstance> grind_paths;
	std::vector<Area> areas;
	
	template <typename Callback>
	void for_each_with(u32 required_components_mask, Callback callback) const;
	template <typename Callback>
	void for_each_with(u32 required_components_mask, Callback callback);
	
	template <typename Callback>
	void for_each(Callback callback) const;
	template <typename Callback>
	void for_each(Callback callback);
	
	void clear_selection();
	std::vector<InstanceId> selected_instances() const;
};

struct HelpMessage {
	Opt<std::string> string;
	s32 id;
	s16 short_id;
	s16 third_person_id;
	s16 coop_id;
	s16 vag;
	s16 character;
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

Instances read_instances(std::string& src);
std::string write_instances(const Instances& src);

template <typename Callback, typename InstanceVec>
static void for_each_instance_of_type_with(u32 required_components_mask, const InstanceVec& instances, Callback callback) {
	if(!instances.empty()) {
		if((instances[0].components_mask() & required_components_mask) == required_components_mask) {
			for(const Instance& instance : instances) {
				callback(instance);
			}
		}
	}
}

template <typename Callback>
void Instances::for_each_with(u32 required_components_mask, Callback callback) const {
	for_each_instance_of_type_with(required_components_mask, moby_instances, callback);
	for_each_instance_of_type_with(required_components_mask, tie_instances, callback);
	for_each_instance_of_type_with(required_components_mask, shrub_instances, callback);
	
	for_each_instance_of_type_with(required_components_mask, dir_lights, callback);
	for_each_instance_of_type_with(required_components_mask, point_lights, callback);
	for_each_instance_of_type_with(required_components_mask, env_sample_points, callback);
	for_each_instance_of_type_with(required_components_mask, env_transitions, callback);
	
	for_each_instance_of_type_with(required_components_mask, cuboids, callback);
	for_each_instance_of_type_with(required_components_mask, spheres, callback);
	for_each_instance_of_type_with(required_components_mask, cylinders, callback);
	for_each_instance_of_type_with(required_components_mask, pills, callback);
	
	for_each_instance_of_type_with(required_components_mask, cameras, callback);
	for_each_instance_of_type_with(required_components_mask, sound_instances, callback);
	for_each_instance_of_type_with(required_components_mask, paths, callback);
	for_each_instance_of_type_with(required_components_mask, grind_paths, callback);
}

template <typename Callback>
void Instances::for_each_with(u32 required_components_mask, Callback callback) {
	static_cast<const Instances*>(this)->for_each_with(required_components_mask, [&](const Instance& inst) {
		callback(const_cast<Instance&>(inst));
	});
}

template <typename Callback>
void Instances::for_each(Callback callback) const {
	this->for_each_with(COM_NONE, callback);
}

template <typename Callback>
void Instances::for_each(Callback callback) {
	this->for_each_with(COM_NONE, callback);
}

#endif
