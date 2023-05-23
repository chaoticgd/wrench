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

#include "level.h"

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
	read_inst_field(settings.unknown_1, node, "unknown_1");
	read_inst_field(settings.unknown_2, node, "unknown_2");
	read_inst_field(settings.unknown_3, node, "unknown_3");
	
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
	write_inst_field(ctx, "unknown_1", settings.unknown_1);
	write_inst_field(ctx, "unknown_2", settings.unknown_2);
	write_inst_field(ctx, "unknown_3", settings.unknown_3);
	
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

s32 PvarField::size() const {
	switch(descriptor) {
		case PVAR_S8:
		case PVAR_U8:
			return 1;
		case PVAR_S16:
		case PVAR_U16:
			return 2;
		case PVAR_S32:
		case PVAR_U32:
		case PVAR_F32:
		case PVAR_RUNTIME_POINTER:
		case PVAR_RELATIVE_POINTER:
		case PVAR_SCRATCHPAD_POINTER:
		case PVAR_GLOBAL_PVAR_POINTER:
			return 4;
		default:
			verify_not_reached_fatal("Invalid pvar field size.");
	}
}

bool PvarType::insert_field(PvarField to_insert, bool sort) {
	// If a field already exists in the given byte range, try to merge them.
	for(PvarField& existing : fields) {
		s32 to_insert_end = to_insert.offset + to_insert.size();
		s32 existing_end = existing.offset + existing.size();
		if((to_insert.offset >= existing.offset && to_insert.offset < existing_end)
			|| (to_insert_end > existing.offset && to_insert_end <= existing_end)) {
			bool offsets_equal = to_insert.offset == existing.offset;
			bool descriptors_equal = to_insert.descriptor == existing.descriptor;
			bool type_equal = to_insert.value_type == existing.value_type ||
				(to_insert.descriptor != PVAR_STRUCT && to_insert.descriptor != PVAR_RELATIVE_POINTER);
			if(offsets_equal && descriptors_equal && type_equal) {
				if(to_insert.name != "") {
					existing.name = to_insert.name;
				}
				return true;
			}
			return false;
		}
	}
	fields.emplace_back(std::move(to_insert));
	if(sort) {
		std::sort(BEGIN_END(fields), [](PvarField& lhs, PvarField& rhs)
			{ return lhs.offset < rhs.offset; });
	}
	return true;
}

std::string pvar_descriptor_to_string(PvarFieldDescriptor descriptor) {
	switch(descriptor) {
		case PVAR_S8: return "s8";
		case PVAR_S16: return "s16";
		case PVAR_S32: return "s32";
		case PVAR_U8: return "u8";
		case PVAR_U16: return "u16";
		case PVAR_U32: return "u32";
		case PVAR_F32: return "f32";
		case PVAR_RUNTIME_POINTER: return "runtime_pointer";
		case PVAR_RELATIVE_POINTER: return "relative_pointer";
		case PVAR_SCRATCHPAD_POINTER: return "scratchpad_pointer";
		case PVAR_GLOBAL_PVAR_POINTER: return "global_pvar_pointer";
		case PVAR_STRUCT: return "struct";
		default: verify_not_reached_fatal("Invalid pvar type descriptor.");
	}
}
PvarFieldDescriptor pvar_string_to_descriptor(std::string str) {
	if(str == "s8") return PVAR_S8;
	if(str == "s16") return PVAR_S16;
	if(str == "s32") return PVAR_S32;
	if(str == "u8") return PVAR_U8;
	if(str == "u16") return PVAR_U16;
	if(str == "u32") return PVAR_U32;
	if(str == "f32") return PVAR_F32;
	if(str == "runtime_pointer") return PVAR_RUNTIME_POINTER;
	if(str == "relative_pointer") return PVAR_RELATIVE_POINTER;
	if(str == "scratchpad_pointer") return PVAR_SCRATCHPAD_POINTER;
	if(str == "global_pvar_pointer") return PVAR_GLOBAL_PVAR_POINTER;
	if(str == "struct") return PVAR_STRUCT;
	verify_not_reached("Invalid pvar field type.");
}
