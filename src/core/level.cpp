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

#include "png.h"

Json get_file_metadata(const char* format, const char* application) {
	return Json {
		{"format", format},
		{"format_version", 7},
		{"application", application}
	};
}

static const char* APPLICATION_NAME = "Wrench WAD Utility";

std::string CameraClass::get_pvar_type(s32 o_class) {
	return "Camera" + std::to_string(o_class) + "Vars";
}

std::string SoundClass::get_pvar_type(s32 o_class) {
	return "Sound" + std::to_string(o_class) + "Vars";
}

std::string MobyClass::get_pvar_type(s32 o_class) {
	return "Moby" + std::to_string(o_class) + "Vars";
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
			assert(0);
	}
}

void Gameplay::clear_selection() {
	for_each_instance([&](Instance& inst) {
		inst.selected = false;
	});
}

std::vector<InstanceId> Gameplay::selected_instances() const {
	std::vector<InstanceId> ids;
	for_each_instance([&](const Instance& inst) {
		if(inst.selected) {
			ids.push_back(inst.id());
		}
	});
	return ids;
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

template <typename Map>
static void read_json_file_into_map(Map& map, const fs::path& src_dir, const Json& json, const char* name, const char* key_name) {
	map_from_json(map, Json::parse(read_file(src_dir/std::string(json[name])))[name], key_name);
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
		default: assert(0);
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
