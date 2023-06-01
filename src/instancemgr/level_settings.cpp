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

LevelSettings read_level_settings(const WtfNode* node) {
	LevelSettings settings;
	
	const WtfAttribute* background_col_attrib = wtf_attribute(node, "background_col");
	if(background_col_attrib) {
		settings.background_colour.emplace();
		read_inst_attrib(*settings.background_colour, background_col_attrib, "background_col");
	}
	const WtfAttribute* fog_col_attrib = wtf_attribute(node, "fog_col");
	if(fog_col_attrib) {
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
	if(core_sounds_count_attrib) {
		settings.core_sounds_count = core_sounds_count_attrib->number.i;
	}
	
	const WtfAttribute* rac3_third_part_attrib = wtf_attribute_of_type(node, "rac3_third_part", WTF_NUMBER);
	if(rac3_third_part_attrib) {
		settings.rac3_third_part = rac3_third_part_attrib->number.i;
	}
	
	for(const WtfNode* plane_node = wtf_first_child(node, "ChunkPlane"); plane_node != nullptr; plane_node = wtf_next_sibling(plane_node, "ChunkPlane")) {
		ChunkPlane& plane = settings.chunk_planes.emplace_back();
		read_inst_field(plane.point, plane_node, "point");
		read_inst_field(plane.normal, plane_node, "normal");
	}
	
	return settings;
}

void write_level_settings(WtfWriter* ctx, const LevelSettings& settings) {
	if(settings.background_colour.has_value()) {
		write_inst_field(ctx, "background_col", *settings.background_colour);
	}
	if(settings.fog_colour.has_value()) {
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
	
	if(settings.core_sounds_count.has_value()) {
		wtf_write_integer_attribute(ctx, "core_sounds_count", *settings.core_sounds_count);
	}
	
	if(settings.rac3_third_part.has_value()) {
		wtf_write_integer_attribute(ctx, "rac3_third_part", *settings.rac3_third_part);
	}
	
	s32 i = 1;
	for(const ChunkPlane& plane : settings.chunk_planes) {
		wtf_begin_node(ctx, "ChunkPlane", std::to_string(i++).c_str());
		write_inst_field(ctx, "point", plane.point);
		write_inst_field(ctx, "normal", plane.normal);
		wtf_end_node(ctx);
	}
}
