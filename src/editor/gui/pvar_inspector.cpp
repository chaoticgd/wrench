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

#include "pvar_inspector.h"

#include <map>
#include <editor/app.h>

struct PvarInspectorState {
	Level* lvl;
	const CppType* type;
	std::vector<InstanceId>* ids;
	std::vector<u8>* pvars;
	std::vector<u8>* diff;
};

static const CppType* get_pvar_type(const Level& lvl);
template <typename ThisInstance>
const CppType* get_single_pvar_type(const InstanceList<ThisInstance>& instances, const std::map<s32, EditorClass>& classes);
static bool check_pvar_data_size(Level& lvl, const CppType& type, const std::vector<InstanceId>& ids, const std::vector<std::vector<u8>*>& pvars);
static void generate_rows(const CppType& type, const std::string& name, const PvarInspectorState& inspector, s32 index, s32 offset, s32 depth, s32 indent);
static void push_poke_pvar_command(Level& lvl, s32 offset, const u8* data, s32 size, const std::vector<InstanceId>& ids);
static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type);

void pvar_inspector(Level& lvl) {
	const CppType* pvar_type = get_pvar_type(lvl);
	if(!pvar_type) {
		// No objects are selected, the selected objects have no pvars, or
		// multiple different types of objects are selected.
		return;
	}
	
	if(ImGui::CollapsingHeader("Pvars")) {
		ImGui::BeginChild("pvars");
	
		std::vector<InstanceId> ids;
		std::vector<std::vector<u8>*> pvars;
		lvl.instances().for_each_with(COM_PVARS, [&](Instance& inst) {
			if(inst.selected) {
				ids.emplace_back(inst.id());
				pvars.emplace_back(&inst.pvars().data);
			}
		});
		
		if(!check_pvar_data_size(lvl, *pvar_type, ids, pvars)) {
			// The pvar data size does not match the size of the C++ type, and the
			// user has been prompted to fix the issue.
			ImGui::EndChild();
			return;
		}
		
		// Determine which bits are equal for all selected objects.
		std::vector<u8> diff(pvars[0]->size(), 0);
		for(size_t i = 1; i < pvars.size(); i++) {
			for(size_t j = 0; j < pvars[i]->size(); j++) {
				diff[j] |= (*pvars[0])[j] ^ (*pvars[i])[j];
			}
		}
		
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
		if(ImGui::BeginTable("pvar_table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("Ofs", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			
			PvarInspectorState inspector;
			inspector.lvl = &lvl;
			inspector.ids = &ids;
			inspector.pvars = pvars[0];
			inspector.diff = &diff;
			generate_rows(*pvar_type, pvar_type->name, inspector, -1, 0, 0, 0);
			
			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::EndChild();
	}
	
}

static const CppType* get_pvar_type(const Level& lvl) {
	s32 type = -1;
	lvl.instances().for_each_with(COM_PVARS, [&](const Instance& inst) {
		if(inst.selected && type != -2) {
			if(type == -1) {
				type = inst.type();
			} else if(type != inst.type()) {
				// If multiple objects of different types are selected we don't
				// display the pvar inspector.
				type = -2;
			}
		}
	});
	
	// If only a single class of object is selected, and that class has pvars,
	// find the pvar type information and pvar data for it, otherwise return.
	if(type == INST_MOBY) {
		return get_single_pvar_type(lvl.instances().moby_instances, lvl.moby_classes);
	} else if(type == INST_CAMERA) {
		return get_single_pvar_type(lvl.instances().cameras, lvl.camera_classes);
	} else if(type == INST_SOUND) {
		return get_single_pvar_type(lvl.instances().sound_instances, lvl.sound_classes);
	}
	
	return nullptr;
}

template <typename ThisInstance>
const CppType* get_single_pvar_type(const InstanceList<ThisInstance>& instances, const std::map<s32, EditorClass>& classes) {
	s32 o_class = -1;
	for(const ThisInstance& inst : instances) {
		if(inst.selected) {
			if(o_class == -1) {
				o_class = inst.o_class();
			} else if(o_class != inst.o_class()) {
				o_class = -1;
				break;
			}
		}
	}
	if(o_class > -1) {
		auto iter = classes.find(o_class);
		if(iter != classes.end() && iter->second.pvar_type) {
			return iter->second.pvar_type;
		}
	}
	return nullptr;
}

struct ResizePvarInfo {
	InstanceId id;
	size_t old_size;
	std::vector<u8> truncated_data;
};

struct ResizePvarCommand {
	size_t new_size;
	std::vector<ResizePvarInfo> instances;
};

static bool check_pvar_data_size(Level& lvl, const CppType& type, const std::vector<InstanceId>& ids, const std::vector<std::vector<u8>*>& pvars) {
	verify_fatal(type.size > -1);
	
	bool needs_resize = false;
	for(size_t i = 0; i < pvars.size(); i++) {
		if(type.size != pvars[i]->size()) {
			needs_resize = true;
		}
	}
	
	if(needs_resize) {
		ImGui::TextWrapped(
			"Pvar data size doesn't match C++ type size. "
			"This could be a problem with the pvar data or the C++ type. "
			"Only use the button below in the former case.");
		
		if(ImGui::Button("Resize Pvar Data")) {
			ResizePvarCommand command;
			command.new_size = type.size;
			for(size_t i = 0; i < ids.size(); i++) {
				ResizePvarInfo& instance = command.instances.emplace_back();
				instance.id = ids[i];
				instance.old_size = pvars[i]->size();
				if(instance.old_size > command.new_size) {
					size_t truncated_size = instance.old_size - command.new_size;
					instance.truncated_data.resize(truncated_size);
					memcpy(&instance.truncated_data[0], &(*pvars[i])[command.new_size], truncated_size);
				}
			}
			
			lvl.push_command<ResizePvarCommand>(std::move(command),
				[](Level& lvl, ResizePvarCommand& command) {
					std::map<InstanceId, ResizePvarInfo*> map;
					for(ResizePvarInfo& record : command.instances) {
						map[record.id] = &record;
					}
					lvl.instances().for_each_with(COM_PVARS, [&](Instance& inst) {
						if(map.find(inst.id()) != map.end()) {
							inst.pvars().data.resize(command.new_size, 0);
						}
					});
				},
				[](Level& lvl, ResizePvarCommand& command) {
					std::map<InstanceId, ResizePvarInfo*> map;
					for(ResizePvarInfo& record : command.instances) {
						map[record.id] = &record;
					}
					lvl.instances().for_each_with(COM_PVARS, [&](Instance& inst) {
						auto info = map.find(inst.id());
						if(info != map.end()) {
							if(!info->second->truncated_data.empty()) {
								inst.pvars().data.insert(inst.pvars().data.end(), BEGIN_END(info->second->truncated_data));
							} else {
								inst.pvars().data.resize(info->second->old_size);
							}
						}
					});
				});
		}
	}
	
	return !needs_resize;
}

static void generate_rows(const CppType& type, const std::string& name, const PvarInspectorState& inspector, s32 index, s32 offset, s32 depth, s32 indent) {
	ImGui::PushID(index);
	defer([&]() { ImGui::PopID(); });
	
	static std::map<s64, bool> expanded_map;
	
	if(depth > 0 && type.descriptor != CPP_TYPE_NAME) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%x", offset);
		ImGui::TableNextColumn();
		for(s32 i = 0; i < indent - 1; i++) {
			ImGui::Text(" ");
			ImGui::SameLine();
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", name.c_str());
		ImGui::TableNextColumn();
	}
	
	switch(type.descriptor) {
		case CPP_ARRAY: {
			bool& expanded = expanded_map[(s64) offset | ((s64) depth << 32)];
			
			verify_fatal(type.array.element_type.get());
			ImGui::AlignTextToFramePadding();
			std::string subscript = stringf("%s[%d]", type.array.element_type->name.c_str(), type.array.element_count);
			if(ImGui::Selectable(subscript.c_str(), expanded, ImGuiSelectableFlags_SpanAllColumns)) {
				expanded = !expanded;
			}
			if(expanded) {
				for(s32 i = 0; i < type.array.element_count; i++) {
					std::string subscript = stringf("%*s[%d]", depth, "", i);
					generate_rows(*type.array.element_type, subscript, inspector, i, offset + i * type.array.element_type->size, depth + 1, indent + 1);
				}
			}
			break;
		}
		case CPP_BUILT_IN: {
			u8 data[16];
			verify_fatal(type.size <= 16);
			memcpy(data, &(*inspector.pvars)[offset], type.size);
			
			ImGuiDataType imgui_type = cpp_built_in_type_to_imgui_data_type(type);
			const char* format = ImGui::DataTypeGetInfo(imgui_type)->PrintFmt;
			
			char data_as_string[64];
			static u8 zero[16] = {};
			if(memcmp(&(*inspector.diff)[offset], zero, type.size) == 0) {
				// This value is the same for all selected objects, so display
				// it normally.
				ImGui::DataTypeFormatString(data_as_string, ARRAY_SIZE(data_as_string), imgui_type, data, format);
			} else {
				// Multiple objects are selected where this value differs, so
				// display a blank text field.
				data_as_string[0] = 0;
			}
			
			if(ImGui::InputText("##input", data_as_string, ARRAY_SIZE(data_as_string), ImGuiInputTextFlags_EnterReturnsTrue)) {
				if(ImGui::DataTypeApplyFromText(data_as_string, imgui_type, data, format)) {
					push_poke_pvar_command(*inspector.lvl, offset, data, type.size, *inspector.ids);
				}
			}
			break;
		}
		case CPP_ENUM: {
			ImGui::Text("(enum)");
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			bool& expanded = expanded_map[(s64) offset | ((s64) depth << 32)];
			
			if(depth > 0) {
				ImGui::AlignTextToFramePadding();
				std::string struct_name = stringf("struct %s", type.name.c_str());
				if(ImGui::Selectable(struct_name.c_str(), expanded, ImGuiSelectableFlags_SpanAllColumns)) {
					expanded = !expanded;
				}
			}
			if(expanded || depth == 0) {
				for(s32 i = 0; i < type.struct_or_union.fields.size(); i++) {
					const CppType& field = type.struct_or_union.fields[i];
					generate_rows(field, field.name, inspector, i, offset + field.offset, depth + 1, indent + 1);
				}
			}
			break;
		}
		case CPP_TYPE_NAME: {
			auto& types = inspector.lvl->level().forest().types();
			auto iter = types.find(type.type_name.string);
			if(iter != types.end()) {
				generate_rows(iter->second, type.name, inspector, -1, offset, depth + 1, indent);
			} else {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%x", offset);
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", name.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("(no definition available)");
			}
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			s32 pointer = *(s32*) &(*inspector.pvars)[offset];
			ImGui::Text("pointer 0x%x", pointer);
			break;
		}
	}
}

struct PokePvarInfo {
	InstanceId id;
	u8 old_data[16];
};

struct PokePvarCommand {
	s32 offset;
	s32 size;
	u8 new_data[16];
	std::vector<PokePvarInfo> instances;
};

static void push_poke_pvar_command(Level& lvl, s32 offset, const u8* data, s32 size, const std::vector<InstanceId>& ids) {
	PokePvarCommand command;
	command.offset = offset;
	command.size = size;
	memcpy(command.new_data, data, size);
	for(const InstanceId& id : ids) {
		Instance* inst = lvl.instances().from_id(id);
		verify_fatal(inst);
		
		PokePvarInfo& info = command.instances.emplace_back();
		info.id = id;
		verify_fatal(offset + size <= inst->pvars().data.size());
		memcpy(info.old_data, &(inst->pvars().data[offset]), size);
	}
	
	lvl.push_command<PokePvarCommand>(std::move(command),
		[](Level& lvl, PokePvarCommand& command) {
			for(PokePvarInfo& info : command.instances) {
				Instance* inst = lvl.instances().from_id(info.id);
				verify_fatal(inst);
				verify_fatal(command.offset + command.size <= inst->pvars().data.size());
				memcpy(&(inst->pvars().data[command.offset]), command.new_data, command.size);
			}
		},
		[](Level& lvl, PokePvarCommand& command) {
			for(PokePvarInfo& info : command.instances) {
				Instance* inst = lvl.instances().from_id(info.id);
				verify_fatal(inst);
				verify_fatal(command.offset + command.size <= inst->pvars().data.size());
				memcpy(&(inst->pvars().data[command.offset]), info.old_data, command.size);
			}
		});
}

static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type) {
	if(cpp_is_built_in_integer(type.built_in)) {
		bool is_signed = cpp_is_built_in_signed(type.built_in);
		switch(type.size) {
			case 1: return is_signed ? ImGuiDataType_S8 : ImGuiDataType_U8;
			case 2: return is_signed ? ImGuiDataType_S16 : ImGuiDataType_U16;
			case 4: return is_signed ? ImGuiDataType_S32 : ImGuiDataType_U32;
			case 8: return is_signed ? ImGuiDataType_S64 : ImGuiDataType_U64;
		}
	} else if(cpp_is_built_in_float(type.built_in)) {
		switch(type.size) {
			case 4: return ImGuiDataType_Float;
			case 8: return ImGuiDataType_Double;
		}
	}
	return ImGuiDataType_U8;
}
