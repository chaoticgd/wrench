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

#include <instancemgr/instance.h>
#include <instancemgr/level_settings.h>

// Represents a gameplay source file.
struct Instances
{
	LevelSettings level_settings;
	
	// objects
	InstanceList<MobyInstance> moby_instances;
	s32 spawnable_moby_count = 400;
	std::vector<s32> moby_classes;
	InstanceList<MobyGroupInstance> moby_groups;
	InstanceList<TieInstance> tie_instances;
	InstanceList<TieGroupInstance> tie_groups;
	InstanceList<ShrubInstance> shrub_instances;
	InstanceList<ShrubGroupInstance> shrub_groups;
	
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
	InstanceList<AreaInstance> areas;
	InstanceList<SharedDataInstance> shared_data;
	
	template <typename Callback>
	void for_each_with(u32 required_components_mask, Callback callback) const;
	template <typename Callback>
	void for_each_with(u32 required_components_mask, Callback callback);
	
	template <typename Callback>
	void for_each(Callback callback) const;
	template <typename Callback>
	void for_each(Callback callback);
	
	Instance* from_id(InstanceId id);
	
	void clear_selection();
	std::vector<InstanceId> selected_instances() const;
	
	Instances* core = nullptr; // Only used during packing to reference the gameplay core from the mission instances.
};

struct HelpMessage
{
	Opt<std::string> string;
	s32 id;
	s16 short_id;
	s16 third_person_id;
	s16 coop_id;
	s16 vag;
	s16 character;
};

struct HelpMessages
{
	Opt<std::vector<u8>> us_english;
	Opt<std::vector<u8>> uk_english;
	Opt<std::vector<u8>> french;
	Opt<std::vector<u8>> german;
	Opt<std::vector<u8>> spanish;
	Opt<std::vector<u8>> italian;
	Opt<std::vector<u8>> japanese;
	Opt<std::vector<u8>> korean;
};

Instances read_instances(std::string& src);
std::string write_instances(const Instances& src, const char* application_name, const char* application_version);

template <typename Callback, typename InstanceVec>
static void for_each_instance_of_type_with(
	u32 required_components_mask, const InstanceVec& instances, Callback callback)
{
	if(!instances.empty()) {
		if((instances[0].components_mask() & required_components_mask) == required_components_mask) {
			for(const Instance& instance : instances) {
				callback(instance);
			}
		}
	}
}

template <typename Callback>
void Instances::for_each_with(u32 required_components_mask, Callback callback) const
{
#define DEF_INSTANCE(inst_type, inst_type_uppercase, inst_variable, link_type) \
	for_each_instance_of_type_with(required_components_mask, inst_variable, callback);
#define GENERATED_INSTANCE_MACRO_CALLS
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_MACRO_CALLS
#undef DEF_INSTANCE
}

template <typename Callback>
void Instances::for_each_with(u32 required_components_mask, Callback callback)
{
	static_cast<const Instances*>(this)->for_each_with(required_components_mask, [&](const Instance& inst) {
		callback(const_cast<Instance&>(inst));
	});
}

template <typename Callback>
void Instances::for_each(Callback callback) const
{
	this->for_each_with(COM_NONE, callback);
}

template <typename Callback>
void Instances::for_each(Callback callback)
{
	this->for_each_with(COM_NONE, callback);
}

#endif
