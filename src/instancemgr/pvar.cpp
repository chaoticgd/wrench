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
	const char* type_name = "";
	const char* variable_name = "";
	s32 size = -1;
};

static const PvarHeaderSpec RAC_PVAR_SUB_VARS[] = {
	{0x00, "RacVars00", ""},
	{0x04, "RacVars04"},
	{0x08, "RacVars08"},
	{0x0c, "RacVars0c"},
	{0x10, "RacVars10", ""},
	{0x14, "RacVars14"},
	{0x18, "RacVars18", ""},
	{0x1c, "RacVars1c", ""},
};

static const PvarHeaderSpec GC_PVAR_SUB_VARS[] = {
	{0x00, "TargetVars", "targetVars", 0x40},
	{0x04, "GcVars04"},
	{0x08, "GcVars08"},
	{0x0c, "GcVars0c"},
	{0x10, "ReactVars", "reactVars", 0xb0},
	{0x14, "GcVars14"},
	{0x18, "GcVars18", "", 0xf0},
	{0x1c, "MoveVars_V2", "moveV2Vars"},
};

static const PvarHeaderSpec UYA_PVAR_SUB_VARS[] = {
	{0x00, "UyaVars00", ""},
	{0x04, "UyaVars04"},
	{0x08, "UyaVars08"},
	{0x0c, "UyaVars0c"},
	{0x10, "UyaVars10", ""},
	{0x14, "UyaVars14"},
	{0x18, "UyaVars18", ""},
	{0x1c, "UyaVars1c", ""},
	{0x20, "UyaVars20", ""},
	{0x24, "UyaVars24"},
	{0x28, "UyaVars28", ""},
	{0x2c, "UyaVars2c", ""},
};

static const PvarHeaderSpec DL_PVAR_SUB_VARS[] = {
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

struct PvarMobyWork {
	bool has_sub_vars;
	std::vector<std::vector<u8>*> pvar_data;
	std::vector<s32> moby_links;
};

struct PvarWork {
	s32 pvar_size;
	std::vector<s32> moby_links;
};

struct SubVarsInfo {
	const PvarHeaderSpec* begin;
	const PvarHeaderSpec* end;
	s32 size;
};

static SubVarsInfo lookup_sub_vars(Game game);
static void generate_moby_pvar_types(std::vector<CppType>& dest, const std::map<s32, PvarMobyWork>& src, const SubVarsInfo& sub_vars, Game game);
static void generate_other_pvar_types(std::vector<CppType>& dest, const std::map<s32, PvarWork>& src, const char* type, Game game);

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

void recover_pvars(Instances& dest, std::vector<CppType>& pvar_types_dest, const Gameplay& src, Game game) {
	if(!src.pvar_table.has_value() || !src.pvar_data.has_value()) {
		return;
	}
	
	SubVarsInfo sub_vars = lookup_sub_vars(game);
	
	// Scatter pvar data amongst the moby, camera and sound instances.
	std::vector<Instance*> pvar_to_inst(src.pvar_table->size());
	for_each_pvar_instance(dest, [&](Instance& inst) {
		const PvarTableEntry& entry = src.pvar_table->at(inst.temp_pvar_index());
		inst.pvars() = Buffer(*src.pvar_data).read_multiple<u8>(entry.offset, entry.size).copy();
		pvar_to_inst.at(inst.temp_pvar_index()) = &inst;
	});
	
	// Build a map of all the moby classes that have pvars.
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
			verify(first_pvar.size() >= sub_vars.size, "Pvar with subvars flag is too small.");
		}
		for(size_t i = 1; i < moby_class.pvar_data.size(); i++) {
			const std::vector<u8>& cur_pvar = *moby_class.pvar_data[i];
			verify(first_pvar.size() == cur_pvar.size(), "Pvars of the same moby class with different sizes.");
			if(moby_class.has_sub_vars && !cur_pvar.empty()) {
				verify(cur_pvar.size() >= sub_vars.size, "Pvar with subvars flag is too small.");
				verify(memcmp(cur_pvar.data(), first_pvar.data(), sub_vars.size) == 0, "Pvars of the same class have different subvars.");
			}
		}
	}
	
	// Build a map of all the camera that have pvars.
	std::map<s32, PvarWork> camera_classes;
	for(CameraInstance& inst : dest.cameras) {
		if(!inst.pvars().empty()) {
			PvarWork& camera_class = camera_classes[inst.type];
			camera_class.pvar_size = inst.pvars().size();
		}
	}
	
	// Build a map of all the sound that have pvars.
	std::map<s32, PvarWork> sound_classes;
	for(SoundInstance& inst : dest.sound_instances) {
		if(!inst.pvars().empty()) {
			PvarWork& sound_class = sound_classes[inst.o_class];
			sound_class.pvar_size = inst.pvars().size();
		}
	}
	
	// Recover type information for moby link fields based on the moby link
	// fixup table included in the gameplay file.
	for(const PvarFixupEntry& entry : opt_iterator(src.pvar_moby_links)) {
		Instance* inst = pvar_to_inst[entry.pvar_index];
		if(inst->type() == INST_MOBY) {
			s32 o_class = ((MobyInstance*) pvar_to_inst[entry.pvar_index])->o_class;
			moby_classes[o_class].moby_links.emplace_back(entry.offset);
		} else if(inst->type() == INST_CAMERA) {
			s32 type = ((CameraInstance*) pvar_to_inst[entry.pvar_index])->type;
			camera_classes[type].moby_links.emplace_back(entry.offset);
		} else if(inst->type() == INST_SOUND) {
			s32 o_class = ((SoundInstance*) pvar_to_inst[entry.pvar_index])->o_class;
			sound_classes[o_class].moby_links.emplace_back(entry.offset);
		}
	}
	
	// Now actually generate the C structs and dump them to strings.
	generate_moby_pvar_types(pvar_types_dest, moby_classes, sub_vars, game);
	generate_other_pvar_types(pvar_types_dest, camera_classes, "camera", game);
	generate_other_pvar_types(pvar_types_dest, sound_classes, "sound", game);
}

static SubVarsInfo lookup_sub_vars(Game game) {
	SubVarsInfo sub_vars;
	switch(game) {
		case Game::RAC: {
			sub_vars.begin = RAC_PVAR_SUB_VARS;
			sub_vars.end = RAC_PVAR_SUB_VARS + ARRAY_SIZE(RAC_PVAR_SUB_VARS);
			sub_vars.size = 0x20;
			break;
		}
		case Game::GC: {
			sub_vars.begin = GC_PVAR_SUB_VARS;
			sub_vars.end = GC_PVAR_SUB_VARS + ARRAY_SIZE(GC_PVAR_SUB_VARS);
			sub_vars.size = 0x20;
			break;
		}
		case Game::UYA: {
			sub_vars.begin = UYA_PVAR_SUB_VARS;
			sub_vars.end = UYA_PVAR_SUB_VARS + ARRAY_SIZE(UYA_PVAR_SUB_VARS);
			sub_vars.size = 0x30;
			break;
		}
		case Game::DL: {
			sub_vars.begin = DL_PVAR_SUB_VARS;
			sub_vars.end = DL_PVAR_SUB_VARS + ARRAY_SIZE(DL_PVAR_SUB_VARS);
			sub_vars.size = 0x50;
			break;
		}
		default: verify_not_reached("Invalid game.");
	}
	return sub_vars;
}

static void generate_moby_pvar_types(std::vector<CppType>& dest, const std::map<s32, PvarMobyWork>& src, const SubVarsInfo& sub_vars, Game game) {
	for(auto& [id, work] : src) {
		CppType& pvar_type = dest.emplace_back(CPP_STRUCT_OR_UNION);
		pvar_type.name = stringf("update%d", id);
		pvar_type.size = (s32) work.pvar_data[0]->size();
		
		s32 offset = 0;
		
		if(work.has_sub_vars) {
			CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
			field.name = "subVars";
			field.offset = offset;
			offset += sub_vars.size;
			field.type_name.string = "SubVars";
		}
		
		while(offset < pvar_type.size) {
			bool hungry = true;
			
			// Check if there's a sub vars struct (e.g. TargetVars) at the
			// current offset.
			if(work.has_sub_vars && game == Game::DL) {
				for(const PvarHeaderSpec* spec = sub_vars.begin; spec < sub_vars.end; spec++) {
					s32 sub_var_offset = *(s32*) &(*work.pvar_data[0])[spec->pointer_offset];
					if(sub_var_offset == offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
						field.name = spec->variable_name;
						field.offset = offset;
						field.type_name.string = spec->type_name;
						if(spec->size > -1) {
							offset += spec->size;
						} else {
							offset += 4;
						}
						hungry = false;
						break;
					}
				}
			}
			
			// Check if there's a moby link at the current offset.
			if(hungry) {
				for(s32 moby_link_offset : work.moby_links) {
					if(offset == moby_link_offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
						field.name = stringf("moby_%x", offset);
						field.offset = offset;
						field.type_name.string = "mobylink";
						offset += 4;
						hungry = false;
						break;
					}
				}
			}
			
			// If we can't recover any type information for this offset, append
			// a placeholder field.
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

static void generate_other_pvar_types(std::vector<CppType>& dest, const std::map<s32, PvarWork>& src, const char* type, Game game) {
	for(auto& [id, work] : src) {
		CppType& pvar_type = dest.emplace_back(CPP_STRUCT_OR_UNION);
		pvar_type.name = stringf("%s%d", type, id);
		pvar_type.size = work.pvar_size;
		
		s32 offset = 0;
		
		while(offset < pvar_type.size) {
			bool hungry = true;
			
			// Check if there's a moby link at the current offset.
			for(s32 moby_link_offset : work.moby_links) {
				if(offset == moby_link_offset) {
					CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
					field.name = stringf("moby_%x", offset);
					field.offset = offset;
					field.type_name.string = "mobylink";
					offset += 4;
					hungry = false;
					break;
				}
			}
			
			// If we can't recover any type information for this offset, append
			// a placeholder field.
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
