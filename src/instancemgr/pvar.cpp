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

#include "pvar.h"

#include <instancemgr/gameplay.h>

struct PvarHeaderSpec {
	s32 pointer_offset;
	const char* type_name;
	const char* variable_name;
	s32 size = -1;
};

static PvarHeaderSpec GC_PVAR_SUB_VARS[] = {
	{0x00, "TargetVars", "targetVars"},
	{0x04, "GCVars04"},
	{0x08, "GCVars08"},
	{0x0c, "GCVars0c"},
	{0x10, "ReactVars", "reactVars"},
	{0x14, "GCVars14"},
	{0x18, "GCVars18"},
	{0x1c, "MoveVars_V2", "moveV2Vars"},
};

static PvarHeaderSpec DL_PVAR_SUB_VARS[] = {
	{0x00, "TargetVars", "targetVars", 0x90},
	{0x04, "npcVars", "npcVars", 0x40},
	{0x08, "TrackVars", "trackVars", 0x40},
	{0x0c, "BogeyVars", "bogeyVars", 0x90},
	{0x10, "ReactVars", "reactVars", 0x80},
	{0x14, "ScriptVars", "scriptVars"},
	{0x18, "MoveVars", "moveVars", 0xf0},
	{0x1c, "MoveVars_V2", "moveV2Vars", 0x1b0},
	{0x20, "ArmorVars", "armorVars", 0x20},
	{0x24, "TransportVars", "transportVars", 0x10},
	{0x28, "EffectorVars", "effectorVars", 0x10},
	{0x2c, "CommandVars", "commandVars", 0x14},
	{0x30, "RoleVars", "roleVars", 0x28},
	{0x34, "FlashVars", "flashVars", 0x10},
	{0x38, "SuckVars", "suckVars", 0x10},
	{0x3c, "NavigationVars", "navigationVars", 0x50},
	{0x40, "ObjectiveVars", "objectiveVars", 0x1c},
	{0x44, nullptr},
	{0x48, nullptr},
	{0x4c, nullptr}
};

template <typename Callback>
void for_each_pvar_instance(Instances& dest, Callback callback) {
	for(MobyInstance& inst : dest.moby_instances) {
		if(inst.temp_pvar_index() > -1) {
			callback(inst);
		}
	}
	for(CameraInstance& inst : dest.cameras) {
		if(inst.temp_pvar_index() > -1) {
			callback(inst);
		}
	}
	for(SoundInstance& inst : dest.sound_instances) {
		if(inst.temp_pvar_index() > -1) {
			callback(inst);
		}
	}
}

void recover_pvars(Instances& dest, std::vector<CppType>& types_dest, const Gameplay& src) {
	if(!src.pvar_table.has_value() || !src.pvar_data.has_value()) {
		return;
	}
	
	Game game = Game::DL;
	
	PvarHeaderSpec* sub_vars_begin;
	PvarHeaderSpec* sub_vars_end;
	s32 sub_vars_size;
	switch(game) {
		case Game::RAC: {
			sub_vars_begin = DL_PVAR_SUB_VARS;
			sub_vars_end = DL_PVAR_SUB_VARS + ARRAY_SIZE(DL_PVAR_SUB_VARS);
			sub_vars_size = 0x50;
			break;
		}
		case Game::GC: {
			sub_vars_begin = DL_PVAR_SUB_VARS;
			sub_vars_end = DL_PVAR_SUB_VARS + ARRAY_SIZE(DL_PVAR_SUB_VARS);
			sub_vars_size = 0x50;
			break;
		}
		case Game::UYA: {
			sub_vars_begin = DL_PVAR_SUB_VARS;
			sub_vars_end = DL_PVAR_SUB_VARS + ARRAY_SIZE(DL_PVAR_SUB_VARS);
			sub_vars_size = 0x50;
			break;
		}
		case Game::DL: {
			sub_vars_begin = DL_PVAR_SUB_VARS;
			sub_vars_end = DL_PVAR_SUB_VARS + ARRAY_SIZE(DL_PVAR_SUB_VARS);
			sub_vars_size = 0x50;
			break;
		}
		default: verify_not_reached("Invalid game.");
	}
	;
	
	// Scatter pvar data amongst the moby, camera and sound instances.
	for_each_pvar_instance(dest, [&](Instance& inst) {
		const PvarTableEntry& entry = src.pvar_table->at(inst.temp_pvar_index());
		inst.pvars() = Buffer(*src.pvar_data).read_multiple<u8>(entry.offset, entry.size).copy();
	});
	
	struct PvarMobyWork {
		bool has_sub_vars;
		std::vector<std::vector<u8>*> pvar_data;
	};
	
	std::map<s32, PvarMobyWork> moby_classes;
	for(MobyInstance& inst : dest.moby_instances) {
		if(!inst.pvars().empty()) {
			PvarMobyWork& moby_class = moby_classes[inst.o_class];
			moby_class.pvar_data.emplace_back(&inst.pvars());
			if(inst.mode_bits & MOBY_MB1_HAS_SUB_VARS) {
				moby_class.has_sub_vars = true;
			}
		}
	}
	
	// Make sure that all pvars that exist for instances of the same class are
	// the same size, and make sure their sub vars structs are equal.
	for(const auto& [id, moby_class] : moby_classes) {
		const std::vector<u8>& first_pvar = *moby_class.pvar_data[0];
		if(moby_class.has_sub_vars) {
			verify(first_pvar.size() >= sub_vars_size, "Pvar with subvars flag is too small.");
		}
		for(size_t i = 1; i < moby_class.pvar_data.size(); i++) {
			const std::vector<u8>& cur_pvar = *moby_class.pvar_data[i];
			verify(first_pvar.size() == cur_pvar.size(), "Pvars of the same moby class with different sizes.");
			if(moby_class.has_sub_vars && !cur_pvar.empty()) {
				verify(cur_pvar.size() >= sub_vars_size, "Pvar with subvars flag is too small.");
				verify(memcmp(cur_pvar.data(), first_pvar.data(), sub_vars_size) == 0, "Pvars of the same class have different subvars.");
			}
		}
	}
	
	for(auto& [id, moby_class] : moby_classes) {
		CppType& pvar_type = types_dest.emplace_back(CPP_STRUCT_OR_UNION);
		pvar_type.name = stringf("update%d", id);
		pvar_type.size = moby_class.pvar_data[0]->size();
		
		s32 offset = 0;
		
		if(moby_class.has_sub_vars) {
			CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
			field.name = "subVars";
			field.offset = offset;
			offset += sub_vars_size;
			field.type_name.string = "SubVars";
		}
		
		while(offset < (s32) moby_class.pvar_data[0]->size()) {
			bool hungry = true;
			if(moby_class.has_sub_vars) {
				for(PvarHeaderSpec* sub_var = sub_vars_begin; sub_var < sub_vars_end; sub_var++) {
					s32 sub_var_offset = *(s32*) &(*moby_class.pvar_data[0])[sub_var->pointer_offset];
					if(sub_var_offset == offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
						field.name = sub_var->variable_name;
						field.offset = offset;
						if(sub_var->size > -1) {
							offset += sub_var->size;
						} else {
							offset += 4;
						}
						field.type_name.string = sub_var->type_name;
						hungry = false;
						break;
					}
				}
			}
			
			if(hungry) {
				CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
				field.name = stringf("unknown_%x", offset);
				field.offset = offset;
				offset += 4;
				field.built_in = CPP_INT;
			}
		}
	}
}

void build_pvars(Gameplay& dest, const Instances& src, const std::vector<CppType>& types_src) {
	
}
