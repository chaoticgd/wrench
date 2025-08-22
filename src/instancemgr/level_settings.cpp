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

#include "level_settings.h"

#include <algorithm>
#include <core/png.h>
#include <instancemgr/wtf_glue.h>
#include <instancemgr/gameplay_convert.h>

LevelSettings read_level_settings(const WtfNode* node)
{
	LevelSettings settings;
	
	const WtfAttribute* background_col_attrib = wtf_attribute(node, "background_col");
	if (background_col_attrib) {
		settings.background_colour.emplace();
		read_inst_attrib(*settings.background_colour, background_col_attrib, "background_col");
	}
	const WtfAttribute* fog_col_attrib = wtf_attribute(node, "fog_col");
	if (fog_col_attrib) {
		settings.fog_colour.emplace();
		read_inst_attrib(*settings.fog_colour, fog_col_attrib, "fog_col");
	}
	read_inst_field(settings.fog_near_dist, node, "fog_near_dist");
	read_inst_field(settings.fog_far_dist, node, "fog_far_dist");
	read_inst_field(settings.fog_near_intensity, node, "fog_near_intensity");
	read_inst_field(settings.fog_far_intensity, node, "fog_far_intensity");
	read_inst_field(settings.death_height, node, "death_height");
	read_inst_field(settings.is_spherical_world, node, "is_spherical_world");
	read_inst_field(settings.sphere_pos, node, "sphere_pos");
	read_inst_field(settings.ship_pos, node, "ship_pos");
	read_inst_field(settings.ship_rot_z, node, "ship_rot_z");
	read_inst_field(settings.ship_path, node, "ship_path");
	read_inst_field(settings.ship_camera_cuboid_start, node, "ship_camera_cuboid_start");
	read_inst_field(settings.ship_camera_cuboid_end, node, "ship_camera_cuboid_end");
	
	const WtfAttribute* core_sounds_count_attrib = wtf_attribute_of_type(node, "core_sounds_count", WTF_NUMBER);
	if (core_sounds_count_attrib) {
		settings.core_sounds_count = core_sounds_count_attrib->number.i;
	}
	
	const WtfAttribute* rac3_third_part_attrib = wtf_attribute_of_type(node, "rac3_third_part", WTF_NUMBER);
	if (rac3_third_part_attrib) {
		settings.rac3_third_part = rac3_third_part_attrib->number.i;
	}
	
	const WtfAttribute* dbg_attack_damage_attrib = wtf_attribute(node, "dbg_attack_damage");
	if (dbg_attack_damage_attrib) {
		settings.dbg_attack_damage.emplace();
		read_inst_attrib(*settings.dbg_attack_damage, dbg_attack_damage_attrib, "dbg_attack_damage");
	}
	
	for (const WtfNode* plane_node = wtf_first_child(node, "ChunkPlane"); plane_node != nullptr; plane_node = wtf_next_sibling(plane_node, "ChunkPlane")) {
		ChunkPlane& plane = settings.chunk_planes.emplace_back();
		read_inst_field(plane.point, plane_node, "point");
		read_inst_field(plane.normal, plane_node, "normal");
	}
	
	for (const WtfNode* third_part = wtf_first_child(node, "DlThirdPart"); third_part != nullptr; third_part = wtf_next_sibling(third_part, "DlThirdPart")) {
		if (!settings.third_part.has_value()) {
			settings.third_part.emplace();
		}
		LevelSettingsThirdPart& dest = settings.third_part->emplace_back();
		read_inst_field(dest.unknown_0, third_part, "unknown_0");
		read_inst_field(dest.unknown_4, third_part, "unknown_4");
		read_inst_field(dest.unknown_8, third_part, "unknown_8");
		read_inst_field(dest.unknown_c, third_part, "unknown_c");
	}
	
	const WtfNode* reward_stats = wtf_child(node, nullptr, "reward_stats");
	if (reward_stats) {
		settings.reward_stats.emplace();
		read_inst_field(settings.reward_stats->xp_decay_rate, reward_stats, "xp_decay_rate");
		read_inst_field(settings.reward_stats->xp_decay_min, reward_stats, "xp_decay_min");
		read_inst_field(settings.reward_stats->bolt_decay_rate, reward_stats, "bolt_decay_rate");
		read_inst_field(settings.reward_stats->bolt_decay_min, reward_stats, "bolt_decay_min");
		read_inst_field(settings.reward_stats->unknown_10, reward_stats, "unknown_10");
		read_inst_field(settings.reward_stats->unknown_14, reward_stats, "unknown_14");
	}
	
	const WtfNode* fifth_part = wtf_child(node, nullptr, "fifth_part");
	if (fifth_part) {
		settings.fifth_part.emplace();
		read_inst_field(settings.fifth_part->unknown_0, fifth_part, "unknown_0");
		read_inst_field(settings.fifth_part->moby_inst_count, fifth_part, "moby_inst_count");
		read_inst_field(settings.fifth_part->unknown_8, fifth_part, "unknown_8");
		read_inst_field(settings.fifth_part->unknown_c, fifth_part, "unknown_c");
		read_inst_field(settings.fifth_part->unknown_10, fifth_part, "unknown_10");
		read_inst_field(settings.fifth_part->dbg_hit_points, fifth_part, "dbg_hit_points");
	}
	
	return settings;
}

void rewrite_level_settings_links(LevelSettings& settings, const Instances& instances)
{
	settings.ship_path = rewrite_link(settings.ship_path.id, INST_PATH, instances, "gameplay level settings");
	if (settings.ship_path.id > -1) {
		settings.ship_camera_cuboid_start = rewrite_link(settings.ship_camera_cuboid_start.id, INST_CUBOID, instances, "gameplay level settings");
		settings.ship_camera_cuboid_end = rewrite_link(settings.ship_camera_cuboid_end.id, INST_CUBOID, instances, "gameplay level settings");
	} else {
		settings.ship_camera_cuboid_start = 0;
		settings.ship_camera_cuboid_end = 0;
	}
}

void write_level_settings(WtfWriter* ctx, const LevelSettings& settings)
{
	if (settings.background_colour.has_value()) {
		write_inst_field(ctx, "background_col", *settings.background_colour);
	}
	if (settings.fog_colour.has_value()) {
		write_inst_field(ctx, "fog_col", *settings.fog_colour);
	}
	write_inst_field(ctx, "fog_near_dist", settings.fog_near_dist);
	write_inst_field(ctx, "fog_far_dist", settings.fog_far_dist);
	write_inst_field(ctx, "fog_near_intensity", settings.fog_near_intensity);
	write_inst_field(ctx, "fog_far_intensity", settings.fog_far_intensity);
	write_inst_field(ctx, "death_height", settings.death_height);
	write_inst_field(ctx, "is_spherical_world", settings.is_spherical_world);
	write_inst_field(ctx, "sphere_pos", settings.sphere_pos);
	write_inst_field(ctx, "ship_pos", settings.ship_pos);
	write_inst_field(ctx, "ship_rot_z", settings.ship_rot_z);
	write_inst_field(ctx, "ship_path", settings.ship_path);
	write_inst_field(ctx, "ship_camera_cuboid_start", settings.ship_camera_cuboid_start);
	write_inst_field(ctx, "ship_camera_cuboid_end", settings.ship_camera_cuboid_end);
	
	if (settings.core_sounds_count.has_value()) {
		wtf_write_integer_attribute(ctx, "core_sounds_count", *settings.core_sounds_count);
	}
	
	if (settings.rac3_third_part.has_value()) {
		wtf_write_integer_attribute(ctx, "rac3_third_part", *settings.rac3_third_part);
	}
	
	if (settings.dbg_attack_damage.has_value()) {
		write_inst_field(ctx, "dbg_attack_damage", *settings.dbg_attack_damage);
	}
	
	for (s32 i = 0; i < (s32) settings.chunk_planes.size(); i++) {
		const ChunkPlane& plane = settings.chunk_planes[i];
		wtf_begin_node(ctx, "ChunkPlane", std::to_string(i).c_str());
		write_inst_field(ctx, "point", plane.point);
		write_inst_field(ctx, "normal", plane.normal);
		wtf_end_node(ctx);
	}
	
	for (s32 i = 0; i < (s32) opt_size(settings.third_part); i++) {
		const LevelSettingsThirdPart& third_part = (*settings.third_part)[i];
		wtf_begin_node(ctx, "DlThirdPart", std::to_string(i).c_str());
		write_inst_field(ctx, "unknown_0", third_part.unknown_0);
		write_inst_field(ctx, "unknown_4", third_part.unknown_4);
		write_inst_field(ctx, "unknown_8", third_part.unknown_8);
		write_inst_field(ctx, "unknown_c", third_part.unknown_c);
		wtf_end_node(ctx);
	}
	
	if (settings.reward_stats.has_value()) {
		wtf_begin_node(ctx, nullptr, "reward_stats");
		const LevelSettingsRewardStats& stats = *settings.reward_stats;
		write_inst_field(ctx, "xp_decay_rate", stats.xp_decay_rate);
		write_inst_field(ctx, "xp_decay_min", stats.xp_decay_min);
		write_inst_field(ctx, "bolt_decay_rate", stats.bolt_decay_rate);
		write_inst_field(ctx, "bolt_decay_min", stats.bolt_decay_min);
		write_inst_field(ctx, "unknown_10", stats.unknown_10);
		write_inst_field(ctx, "unknown_14", stats.unknown_14);
		wtf_end_node(ctx);
	}
	
	if (settings.fifth_part.has_value()) {
		wtf_begin_node(ctx, nullptr, "fifth_part");
		const LevelSettingsFifthPart& fifth_part = *settings.fifth_part;
		write_inst_field(ctx, "unknown_0", fifth_part.unknown_0);
		write_inst_field(ctx, "moby_inst_count", fifth_part.moby_inst_count);
		write_inst_field(ctx, "unknown_8", fifth_part.unknown_8);
		write_inst_field(ctx, "unknown_c", fifth_part.unknown_c);
		write_inst_field(ctx, "unknown_10", fifth_part.unknown_10);
		write_inst_field(ctx, "dbg_hit_points", fifth_part.dbg_hit_points);
		wtf_end_node(ctx);
	}
}

s32 chunk_index_from_position(const glm::vec3& point, const LevelSettings& level_settings)
{
	if (!level_settings.chunk_planes.empty()) {
		glm::vec3 plane_1_point = level_settings.chunk_planes[0].point;
		glm::vec3 plane_1_normal = level_settings.chunk_planes[0].normal;
		if (glm::dot(plane_1_normal, point - plane_1_point) > 0.f) {
			return 1;
		}
		if (level_settings.chunk_planes.size() > 1) {
			glm::vec3 plane_2_point = level_settings.chunk_planes[1].point;
			glm::vec3 plane_2_normal = level_settings.chunk_planes[1].normal;
			if (glm::dot(plane_2_normal, point - plane_2_point) > 0.f) {
				return 2;
			}
		}
	}
	return 0;
}
