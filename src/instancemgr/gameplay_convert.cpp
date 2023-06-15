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

#include "gameplay_convert.h"

static void generate_psuedo_positions(Instances& instances);

void move_gameplay_to_instances(Instances& dest, HelpMessages* help_dest, OcclusionMappings* occl_dest, std::vector<CppType>& types_dest, Gameplay& src, Game game) {
	if(src.level_settings.has_value()) {
		dest.level_settings = std::move(*src.level_settings);
	}
	
	dest.moby_instances = std::move(opt_iterator(src.moby_instances));
	dest.spawnable_moby_count = opt_or_zero(src.spawnable_moby_count);
	dest.moby_classes = std::move(opt_iterator(src.moby_classes));
	dest.moby_groups = std::move(opt_iterator(src.moby_groups));
	dest.tie_instances = std::move(opt_iterator(src.tie_instances));
	dest.tie_groups = std::move(opt_iterator(src.tie_groups));
	dest.shrub_instances = std::move(opt_iterator(src.shrub_instances));
	dest.shrub_groups = std::move(opt_iterator(src.shrub_groups));
	
	dest.dir_lights = std::move(opt_iterator(src.dir_lights));
	dest.point_lights = std::move(opt_iterator(src.point_lights));
	dest.env_sample_points = std::move(opt_iterator(src.env_sample_points));
	dest.env_transitions = std::move(opt_iterator(src.env_transitions));
	
	dest.cuboids = std::move(opt_iterator(src.cuboids));
	dest.spheres = std::move(opt_iterator(src.spheres));
	dest.cylinders = std::move(opt_iterator(src.cylinders));
	dest.pills = std::move(opt_iterator(src.pills));
	
	dest.cameras = std::move(opt_iterator(src.cameras));
	dest.sound_instances = std::move(opt_iterator(src.sound_instances));
	dest.paths = std::move(opt_iterator(src.paths));
	dest.grind_paths = std::move(opt_iterator(src.grind_paths));
	dest.areas = std::move(opt_iterator(src.areas));
	
	if(help_dest) {
		help_dest->us_english = std::move(opt_iterator(src.us_english_help_messages));
		help_dest->uk_english = std::move(opt_iterator(src.uk_english_help_messages));
		help_dest->french = std::move(opt_iterator(src.french_help_messages));
		help_dest->german = std::move(opt_iterator(src.german_help_messages));
		help_dest->spanish = std::move(opt_iterator(src.spanish_help_messages));
		help_dest->italian = std::move(opt_iterator(src.italian_help_messages));
		help_dest->japanese = std::move(opt_iterator(src.japanese_help_messages));
		help_dest->korean = std::move(opt_iterator(src.korean_help_messages));
	}
	
	if(occl_dest && src.occlusion.has_value()) {
		*occl_dest = *src.occlusion;
	}
	
	recover_pvars(dest, types_dest, src, game);
	
	// Generate positions for objects that should be visible in the 3D view but
	// that don't have positions stored for them in the game's files.
	generate_psuedo_positions(dest);
}

static void generate_psuedo_positions(Instances& instances) {
	f32 moby_group_z = 0.f;
	f32 tie_group_z = 1.f;
	f32 shrub_group_z = 2.f;
	f32 area_z = 3.f;
	
	for(MobyGroupInstance& group : instances.moby_groups) {
		if(!group.members.empty()) {
			glm::vec3 pos = {0.f, 0.f, 0.f};
			s32 count = 0;
			for(MobyLink link : group.members) {
				MobyInstance* inst = instances.moby_instances.from_id(link.id);
				if(inst) {
					pos += inst->transform().pos();
					count++;
				}
			}
			group.transform().set_from_pos_rot_scale(pos * (1.f / (f32) count));
		} else {
			glm::vec3 pos((f32) group.id().value * 2.f, 0.f, moby_group_z);
			group.transform().set_from_pos_rot_scale(pos);
		}
	}
	
	for(TieGroupInstance& group : instances.tie_groups) {
		if(!group.members.empty()) {
			glm::vec3 pos = {0.f, 0.f, 0.f};
			s32 count = 0;
			for(TieLink link : group.members) {
				TieInstance* inst = instances.tie_instances.from_id(link.id);
				if(inst) {
					pos += inst->transform().pos();
					count++;
				}
			}
			group.transform().set_from_pos_rot_scale(pos * (1.f / (f32) count));
		} else {
			glm::vec3 pos((f32) group.id().value * 2.f, 0.f, tie_group_z);
			group.transform().set_from_pos_rot_scale(pos);
		}
	}
	
	for(ShrubGroupInstance& group : instances.shrub_groups) {
		if(!group.members.empty()) {
			glm::vec3 pos = {0.f, 0.f, 0.f};
			s32 count = 0;
			for(ShrubLink link : group.members) {
				ShrubInstance* inst = instances.shrub_instances.from_id(link.id);
				if(inst) {
					pos += inst->transform().pos();
					count++;
				}
			}
			group.transform().set_from_pos_rot_scale(pos * (1.f / (f32) count));
		} else {
			glm::vec3 pos((f32) group.id().value * 2.f, 0.f, shrub_group_z);
			group.transform().set_from_pos_rot_scale(pos);
		}
	}
	
	for(AreaInstance& area : instances.areas) {
		glm::vec3 pos = {0.f, 0.f, 0.f};
		s32 count = 0;
		for(PathLink link : area.paths) {
			PathInstance& path = instances.paths[link.id];
			if(!path.spline().empty()) {
				glm::vec3 path_pos = {0.f, 0.f, 0.f};
				for(glm::vec4& vertex : path.spline()) {
					path_pos += glm::vec3(vertex);
				}
				pos += path_pos * (1.f / (f32) path.spline().size());
			}
		}
		for(CuboidLink link : area.cuboids) {
			CuboidInstance& inst = instances.cuboids[link.id];
			pos += inst.transform().pos();
			count++;
		}
		for(SphereLink link : area.spheres) {
			SphereInstance& inst = instances.spheres[link.id];
			pos += inst.transform().pos();
			count++;
		}
		for(CylinderLink link : area.cylinders) {
			CylinderInstance& inst = instances.cylinders[link.id];
			pos += inst.transform().pos();
			count++;
		}
		if(count > 0) {
			area.transform().set_from_pos_rot_scale(pos * (1.f / (f32) count));
		} else {
			glm::vec3 pos((f32) area.id().value * 2.f, 0.f, area_z);
			area.transform().set_from_pos_rot_scale(pos);
		}
	}
}

void move_instances_to_gameplay(Gameplay& dest, Instances& src, HelpMessages* help_src, OcclusionMappings* occlusion_src, const std::map<std::string, CppType>& types_src) {
	build_pvars(dest, src, types_src);
	
	dest.level_settings = std::move(src.level_settings);
	
	if(help_src) {
		dest.us_english_help_messages = std::move(help_src->us_english);
		dest.uk_english_help_messages = std::move(help_src->uk_english);
		dest.french_help_messages = std::move(help_src->french);
		dest.german_help_messages = std::move(help_src->german);
		dest.spanish_help_messages = std::move(help_src->spanish);
		dest.italian_help_messages = std::move(help_src->italian);
		dest.japanese_help_messages = std::move(help_src->japanese);
		dest.korean_help_messages = std::move(help_src->korean);
	}
	
	dest.moby_instances = src.moby_instances.release();
	dest.spawnable_moby_count = src.spawnable_moby_count;
	dest.moby_classes = std::move(src.moby_classes);
	dest.moby_groups = src.moby_groups.release();
	dest.tie_instances = src.tie_instances.release();
	dest.tie_groups = src.tie_groups.release();
	dest.shrub_instances = (src.shrub_instances).release();
	dest.shrub_groups = src.shrub_groups.release();
	
	dest.dir_lights = src.dir_lights.release();
	dest.point_lights = src.point_lights.release();
	dest.env_sample_points = src.env_sample_points.release();
	dest.env_transitions = src.env_transitions.release();
	
	dest.cuboids = src.cuboids.release();
	dest.spheres = src.spheres.release();
	dest.cylinders = src.cylinders.release();
	dest.pills = src.pills.release();
	
	dest.cameras = src.cameras.release();
	dest.sound_instances = src.sound_instances.release();
	dest.paths = src.paths.release();
	dest.grind_paths = src.grind_paths.release();
	dest.areas = src.areas.release();
	
	if(occlusion_src && (!occlusion_src->tfrag_mappings.empty()
			|| !occlusion_src->tie_mappings.empty()
			|| !occlusion_src->moby_mappings.empty())) {
		dest.occlusion = *occlusion_src;
	}
}
