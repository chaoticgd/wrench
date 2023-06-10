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

static bool get_pvar_type_and_data(const CppType*& pvar_type, std::vector<std::vector<u8>*>& pvar_data, Level& lvl);
static void generate_rows(const CppType& type, std::vector<u8>& pvar_data, std::vector<u8>& diff_data, const std::map<std::string, CppType>& types, s32 index, s32 offset, s32 depth);
static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type);

void pvar_inspector(Level& lvl) {
	const CppType* pvar_type;
	std::vector<std::vector<u8>*> pvar_data;
	if(!get_pvar_type_and_data(pvar_type, pvar_data, lvl)) {
		return;
	}
	
	verify_fatal(pvar_type->size != -1);
	if(pvar_type->size > (s32) pvar_data[0]->size()) {
		ImGui::Text("Error: Pvar data smaller than data type.");
		if(ImGui::Button("Extend Data")) {
			for(std::vector<u8>* pvars : pvar_data) {
				pvars->resize(pvar_type->size, 0);
			}
		}
		return;
	}
	
	// Determine which bits are equal for all selected objects.
	std::vector<u8> diff_data(pvar_data.size(), 0);
	for(size_t i = 1; i < pvar_data.size(); i++) {
		for(size_t j = 0; j < pvar_data.size(); j++) {
			diff_data[j] |= (*(pvar_data[0]))[j] ^ (*(pvar_data[i]))[j];
		}
	}
	
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	
	ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
	if(ImGui::CollapsingHeader("Pvars")) {
		ImGui::BeginChild("pvars");
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
		if(ImGui::BeginTable("mods", 3, flags)) {
			ImGui::TableSetupColumn("Ofs", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
			ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			
			verify_fatal(pvar_type);
			generate_rows(*pvar_type, *pvar_data[0], diff_data, lvl.level().forest().types(), -1, 0, 0);
			
			ImGui::EndTable();
		}
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
		ImGui::EndChild();
	}
	
	ImGui::PopStyleColor();
}

static bool get_pvar_type_and_data(const CppType*& pvar_type, std::vector<std::vector<u8>*>& pvar_data, Level& lvl) {
	s32 type = -1;
	lvl.instances().for_each_with(COM_PVARS, [&](Instance& inst) {
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
		s32 o_class = -1;
		for(MobyInstance& inst : lvl.instances().moby_instances) {
			if(inst.selected && o_class != -2) {
				if(o_class == -1) {
					o_class = inst.o_class;
				} else if(type != inst.o_class) {
					o_class = -1;
					break;
				}
				pvar_data.emplace_back(&inst.pvars());
			}
		}
		if(o_class > -1) {
			auto iter = lvl.moby_classes.find(o_class);
			if(iter != lvl.moby_classes.end() && iter->second.pvar_type) {
				pvar_type = iter->second.pvar_type;
				return true;
			}
		}
	} else if(type == INST_CAMERA) {
		// TODO
	} else if(type == INST_SOUND) {
		// TODO
	}
	
	return false;
}

static void generate_rows(const CppType& type, std::vector<u8>& pvar_data, std::vector<u8>& diff_data, const std::map<std::string, CppType>& types, s32 index, s32 offset, s32 depth) {
	if(index > -1) {
		ImGui::PushID(index);
	} else {
		ImGui::PushID(type.name.c_str());
	}
	defer([&](){ ImGui::PopID(); });
	
	if((depth > 0 || type.descriptor != CPP_STRUCT_OR_UNION) && type.descriptor != CPP_TYPE_NAME) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%x", offset);
		ImGui::TableNextColumn();
		if(index > -1) {
			std::string subscript = stringf("%*s[%d]", depth, "", index);
			if(ImGui::Selectable(subscript.c_str(), type.expanded)) {
				type.expanded = !type.expanded;
			}
		} else {
			std::string name = stringf("%*s%s", depth, "", type.name.c_str());
			if(ImGui::Selectable(name.c_str(), type.expanded)) {
				type.expanded = !type.expanded;
			}
		}
		ImGui::TableNextColumn();
	}
	
	switch(type.descriptor) {
		case CPP_ARRAY: {
			ImGui::Text("(array)");
			if(type.expanded) {
				for(s32 i = 0; i < type.array.element_count; i++) {
					generate_rows(*type.array.element_type, pvar_data, diff_data, types, i, offset + i * type.array.element_type->size, depth + 1);
				}
			}
			break;
		}
		case CPP_BUILT_IN: {
			ImGuiDataType imgui_type = cpp_built_in_type_to_imgui_data_type(type);
			ImGui::InputScalar("##input", imgui_type, pvar_data.data() + offset);
			break;
		}
		case CPP_ENUM: {
			ImGui::Text("(enum)");
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			if(depth > 0) {
				ImGui::Text("(struct or union)");
			}
			if(type.expanded || depth == 0) {
				for(s32 i = 0; i < type.struct_or_union.fields.size(); i++) {
					const CppType& field = type.struct_or_union.fields[i];
					generate_rows(field, pvar_data, diff_data, types, -1, offset + field.offset, depth + 1);
				}
			}
			break;
		}
		case CPP_TYPE_NAME: {
			auto iter = types.find(type.type_name.string);
			if(iter != types.end()) {
				generate_rows(iter->second, pvar_data, diff_data, types, -1, offset, depth + 1);
			} else {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%x", offset);
				ImGui::TableNextColumn();
				if(index > -1) {
					std::string subscript = stringf("[%d]", index);
					if(ImGui::Selectable(subscript.c_str(), type.expanded)) {
						type.expanded = !type.expanded;
					}
				} else {
					if(ImGui::Selectable(type.name.c_str(), type.expanded)) {
						type.expanded = !type.expanded;
					}
				}
				ImGui::TableNextColumn();
				ImGui::Text("(no definition available)");
			}
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			ImGui::InputScalar("##input", ImGuiDataType_U32, pvar_data.data() + offset);
			break;
		}
	}
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
