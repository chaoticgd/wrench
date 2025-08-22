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

#include <algorithm>
#include <instancemgr/gameplay.h>
#include <instancemgr/gameplay_convert.h>

struct PvarHeaderSpec
{
	s32 pointer_offset;
	const char* type_name = "";
	const char* variable_name = "";
	s32 size = -1;
};

static const PvarHeaderSpec RAC_PVAR_SUB_VARS[] = {
	{0x00, "RacVars00", "racVars00", 0x40},
	{0x04, "RacVars04", "racVars04"},
	{0x08, "RacVars08", "racVars08", 0x40},
	{0x0c, "RacVars0c", "racVars0c", 0x10},
	{0x10, "RacVars10", "racVars10", 0x60},
	{0x14, "RacVars14", "racVars14", 0xb0},
	{0x18, "RacVars18", "racVars18", 0x50},
	{0x1c, nullptr},
};

static const PvarHeaderSpec GC_PVAR_SUB_VARS[] = {
	{0x00, "TargetVars", "targetVars", 0x30},
	{0x04, "GcVars04", "gcVars04"},
	{0x08, "GcVars08", "gcVars08", 0x40},
	{0x0c, "GcVars0c", "gcVars0c", 0x20},
	{0x10, "ReactVars", "reactVars", 0xb0},
	{0x14, "GcVars14", "gcVars14", 0x160},
	{0x18, "GcVars18", "gcVars18", 0xf0},
	{0x1c, "MoveVars_V2", "moveV2Vars", 0x20},
};

static const PvarHeaderSpec UYA_PVAR_SUB_VARS[] = {
	{0x00, "TargetVars", "targetVars", 0x40},
	{0x04, "UyaVars04", "uyaVars04"},
	{0x08, "TrackVars", "trackVars", 0x40},
	{0x0c, "UyaVars0c", "uyaVars0c", 0x20},
	{0x10, "ReactVars", "reactVars", 0xc0},
	{0x14, "UyaVars14", "uyaVars14", 0x10},
	{0x18, "UyaVars18", "uyaVars18"},
	{0x1c, "UyaVars1c", "uyaVars1c", 0xf0},
	{0x20, "ArmorVars", "armorVars", 0x20},
	{0x24, "UyaVars24", "uyaVars24", 0x20},
	{0x28, "UyaVars28", "uyaVars28"},
	{0x2c, "UyaVars2c", "uyaVars2c", 0x10},
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

struct PvarMobyWork
{
	bool has_sub_vars;
	std::vector<std::vector<u8>*> pvar_data;
	std::vector<s32> moby_links;
	std::vector<s32> pointers;
};

struct PvarWork
{
	s32 pvar_size;
	std::vector<s32> moby_links;
	std::vector<s32> pointers;
};

struct SubVarsInfo
{
	const PvarHeaderSpec* begin;
	const PvarHeaderSpec* end;
	s32 size;
};

static SubVarsInfo lookup_sub_vars(Game game);
static void generate_moby_pvar_types(
	std::vector<CppType>& dest,
	const std::map<s32, PvarMobyWork>& src,
	const SubVarsInfo& sub_vars,
	Game game);
static void generate_other_pvar_types(
	std::vector<CppType>& dest,
	const std::map<s32, PvarWork>& src,
	const char* type,
	Game game);

template <typename Callback>
void for_each_pvar_instance(Instances& dest, Callback callback)
{
	for (MobyInstance& inst : dest.moby_instances) {
		if (inst.pvars().temp_pvar_index > -1) {
			callback(inst);
		}
	}
	for (CameraInstance& inst : dest.cameras) {
		if (inst.pvars().temp_pvar_index > -1) {
			callback(inst);
		}
	}
	for (SoundInstance& inst : dest.sound_instances) {
		if (inst.pvars().temp_pvar_index > -1) {
			callback(inst);
		}
	}
}

void recover_pvars(
	Instances& dest, std::vector<CppType>& pvar_types_dest, const Gameplay& src, Game game)
{
	if (!src.pvar_table.has_value() || !src.pvar_data.has_value()) {
		return;
	}
	
	SubVarsInfo sub_vars = lookup_sub_vars(game);
	
	// Scatter pvar data amongst the moby, camera and sound instances.
	std::vector<Instance*> pvar_to_inst(src.pvar_table->size());
	for_each_pvar_instance(dest, [&](Instance& inst) {
		const PvarTableEntry& entry = src.pvar_table->at(inst.pvars().temp_pvar_index);
		inst.pvars().data = Buffer(*src.pvar_data).read_multiple<u8>(entry.offset, entry.size).copy();
		pvar_to_inst.at(inst.pvars().temp_pvar_index) = &inst;
	});
	
	// Build a map of all the moby classes that have pvars.
	std::map<s32, PvarMobyWork> moby_classes;
	for (MobyInstance& inst : dest.moby_instances) {
		if (!inst.pvars().data.empty()) {
			PvarMobyWork& moby_class = moby_classes[inst.o_class()];
			moby_class.pvar_data.emplace_back(&inst.pvars().data);
			if (inst.mode_bits & MOBY_MB1_HAS_SUB_VARS) {
				moby_class.has_sub_vars = true;
			}
		}
	}
	
	// Make sure that all pvars that exist for instances of the same class are
	// the same size, and make sure their sub vars structs are equal.
	for (const auto& [id, moby_class] : moby_classes) {
		const std::vector<u8>& first_pvar = *moby_class.pvar_data[0];
		if (moby_class.has_sub_vars) {
			verify(first_pvar.size() >= sub_vars.size, "Pvar with subvars flag is too small.");
		}
		for (size_t i = 1; i < moby_class.pvar_data.size(); i++) {
			const std::vector<u8>& cur_pvar = *moby_class.pvar_data[i];
			verify(first_pvar.size() == cur_pvar.size(), "Pvars of the same moby class with different sizes.");
			if (moby_class.has_sub_vars && !cur_pvar.empty()) {
				verify(cur_pvar.size() >= sub_vars.size, "Pvar with subvars flag is too small.");
				verify(memcmp(cur_pvar.data(), first_pvar.data(), sub_vars.size) == 0, "Pvars of the same class have different subvars.");
			}
		}
	}
	
	// Build a map of all the camera that have pvars.
	std::map<s32, PvarWork> camera_classes;
	for (CameraInstance& inst : dest.cameras) {
		if (!inst.pvars().data.empty()) {
			PvarWork& camera_class = camera_classes[inst.o_class()];
			camera_class.pvar_size = inst.pvars().data.size();
		}
	}
	
	// Build a map of all the sound that have pvars.
	std::map<s32, PvarWork> sound_classes;
	for (SoundInstance& inst : dest.sound_instances) {
		if (!inst.pvars().data.empty()) {
			PvarWork& sound_class = sound_classes[inst.o_class()];
			sound_class.pvar_size = inst.pvars().data.size();
		}
	}
	
	// Recover type information for moby link fields based on the moby link
	// fixup table included in the gameplay file.
	for (const PvarFixupEntry& entry : opt_iterator(src.pvar_moby_links)) {
		Instance* inst = pvar_to_inst.at(entry.pvar_index);
		if (inst->type() == INST_MOBY) {
			s32 o_class = ((MobyInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			moby_classes[o_class].moby_links.emplace_back(entry.offset);
		} else if (inst->type() == INST_CAMERA) {
			s32 o_class = ((CameraInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			camera_classes[o_class].moby_links.emplace_back(entry.offset);
		} else if (inst->type() == INST_SOUND) {
			s32 o_class = ((SoundInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			sound_classes[o_class].moby_links.emplace_back(entry.offset);
		}
	}
	
	// Recover type information for relative pointers.
	for (const PvarFixupEntry& entry : opt_iterator(src.pvar_relative_pointers)) {
		Instance* inst = pvar_to_inst.at(entry.pvar_index);
		PvarPointer& pointer = inst->pvars().pointers.emplace_back();
		pointer.offset = entry.offset;
		pointer.type = PvarPointerType::RELATIVE;
		if (inst->type() == INST_MOBY) {
			s32 o_class = ((MobyInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			moby_classes[o_class].pointers.emplace_back(entry.offset);
		} else if (inst->type() == INST_CAMERA) {
			s32 o_class = ((CameraInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			camera_classes[o_class].pointers.emplace_back(entry.offset);
		} else if (inst->type() == INST_SOUND) {
			s32 o_class = ((SoundInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			sound_classes[o_class].pointers.emplace_back(entry.offset);
		}
	}
	
	// Recover type information for pointers to the shared data section.
	for (const SharedDataEntry& entry : opt_iterator(src.shared_data_table)) {
		Instance* inst = pvar_to_inst.at(entry.pvar_index);
		PvarPointer& pointer = inst->pvars().pointers.emplace_back();
		pointer.offset = entry.pointer_offset;
		pointer.type = PvarPointerType::SHARED;
		pointer.shared_data_id = entry.shared_data_offset;
		if (inst->type() == INST_MOBY) {
			s32 o_class = ((MobyInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			moby_classes[o_class].pointers.emplace_back(entry.pointer_offset);
		} else if (inst->type() == INST_CAMERA) {
			s32 o_class = ((CameraInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			camera_classes[o_class].pointers.emplace_back(entry.pointer_offset);
		} else if (inst->type() == INST_SOUND) {
			s32 o_class = ((SoundInstance*) pvar_to_inst.at(entry.pvar_index))->o_class();
			sound_classes[o_class].pointers.emplace_back(entry.pointer_offset);
		}
	}
	
	// Now actually generate the C structs and dump them to strings.
	generate_moby_pvar_types(pvar_types_dest, moby_classes, sub_vars, game);
	generate_other_pvar_types(pvar_types_dest, camera_classes, "camera", game);
	generate_other_pvar_types(pvar_types_dest, sound_classes, "sound", game);
	
	// Dice the shared data section into shared data instances.
	s32 shared_offset = 0;
	while (shared_offset < opt_size(src.shared_data)) {
		s32 smallest_greater = INT32_MAX;
		for (const SharedDataEntry& entry : opt_iterator(src.shared_data_table)) {
			if (entry.shared_data_offset > shared_offset && entry.shared_data_offset <= src.shared_data->size()) {
				smallest_greater = std::min(entry.shared_data_offset, smallest_greater);
			}
		}
		SharedDataInstance& inst = dest.shared_data.create(shared_offset);
		if (smallest_greater != INT32_MAX) {
			const u8* begin = src.shared_data->data() + shared_offset;
			const u8* end = src.shared_data->data() + smallest_greater;
			inst.pvars().data = std::vector<u8>(begin, end);
			shared_offset = smallest_greater;
		} else {
			const u8* begin = src.shared_data->data() + shared_offset;
			const u8* end = src.shared_data->data() + src.shared_data->size();
			inst.pvars().data = std::vector<u8>(begin, end);
			shared_offset = src.shared_data->size();
		}
	}
}

static SubVarsInfo lookup_sub_vars(Game game)
{
	SubVarsInfo sub_vars;
	switch (game) {
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

static void generate_moby_pvar_types(
	std::vector<CppType>& dest,
	const std::map<s32, PvarMobyWork>& src,
	const SubVarsInfo& sub_vars,
	Game game)
{
	for (auto& [id, work] : src) {
		CppType& pvar_type = dest.emplace_back(CPP_STRUCT_OR_UNION);
		pvar_type.name = stringf("update%d", id);
		pvar_type.size = (s32) work.pvar_data[0]->size();
		
		s32 offset = 0;
		
		if (work.has_sub_vars) {
			CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
			field.name = "subVars";
			field.offset = offset;
			field.size = sub_vars.size;
			field.alignment = 4;
			offset += sub_vars.size;
			field.type_name.string = "SubVars";
		}
		
		while (offset < pvar_type.size) {
			bool hungry = true;
			
			// Check if there's a sub vars struct (e.g. TargetVars) at the
			// current offset.
			if (work.has_sub_vars) {
				for (const PvarHeaderSpec* spec = sub_vars.begin; spec < sub_vars.end; spec++) {
					s32 sub_var_offset = *(s32*) &(*work.pvar_data[0])[spec->pointer_offset];
					if (sub_var_offset == offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
						field.name = spec->variable_name;
						field.offset = offset;
						field.size = (spec->size > -1) ? spec->size : 4;
						field.alignment = 4;
						field.type_name.string = spec->type_name;
						if (spec->size > -1) {
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
			if (hungry) {
				for (s32 moby_link_offset : work.moby_links) {
					if (moby_link_offset == offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
						field.name = stringf("moby_%x", offset);
						field.offset = offset;
						field.size = 4;
						field.alignment = 4;
						field.type_name.string = "mobylink";
						offset += 4;
						hungry = false;
						break;
					}
				}
			}
			
			// Check if there's a relative or shared data pointer at the current offset.
			if (hungry) {
				for (s32 pointer_offset : work.pointers) {
					if (pointer_offset == offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_POINTER_OR_REFERENCE);
						field.name = stringf("pointer_%x", offset);
						field.offset = offset;
						field.size = 4;
						field.alignment = 4;
						field.pointer_or_reference.is_reference = false;
						field.pointer_or_reference.value_type = std::make_unique<CppType>(CPP_BUILT_IN);
						field.pointer_or_reference.value_type->built_in = CPP_VOID;
						offset += 4;
						hungry = false;
						break;
					}
				}
			}
			
			// If we can't recover any type information for this offset, append
			// a placeholder field.
			if (hungry) {
				CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
				field.name = stringf("unknown_%x", offset);
				field.offset = offset;
				field.size = 4;
				field.alignment = 4;
				offset += 4;
				field.built_in = CPP_INT;
			}
		}
	}
}

static void generate_other_pvar_types(
	std::vector<CppType>& dest, const std::map<s32, PvarWork>& src, const char* type, Game game)
{ 
	for (auto& [id, work] : src) {
		CppType& pvar_type = dest.emplace_back(CPP_STRUCT_OR_UNION);
		pvar_type.name = stringf("%s%d", type, id);
		pvar_type.size = work.pvar_size;
		
		s32 offset = 0;
		
		if (strcmp(type, "camera") == 0 && game == Game::DL) {
			CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
			field.name = "s";
			field.offset = offset;
			field.type_name.string = "cameraShared";
			offset += 0x20;
		}
		
		while (offset < pvar_type.size) {
			bool hungry = true;
			
			// Check if there's a moby link at the current offset.
			for (s32 moby_link_offset : work.moby_links) {
				if (moby_link_offset == offset) {
					CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_TYPE_NAME);
					field.name = stringf("moby_%x", offset);
					field.offset = offset;
					field.size = 4;
					field.alignment = 4;
					field.type_name.string = "mobylink";
					offset += 4;
					hungry = false;
					break;
				}
			}
			
			// Check if there's a relative or shared data pointer at the current offset.
			if (hungry) {
				for (s32 pointer_offset : work.pointers) {
					if (pointer_offset == offset) {
						CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_POINTER_OR_REFERENCE);
						field.name = stringf("pointer_%x", offset);
						field.offset = offset;
						field.size = 4;
						field.alignment = 4;
						field.pointer_or_reference.is_reference = false;
						field.pointer_or_reference.value_type = std::make_unique<CppType>(CPP_BUILT_IN);
						field.pointer_or_reference.value_type->built_in = CPP_VOID;
						offset += 4;
						hungry = false;
						break;
					}
				}
			}
			
			// If we can't recover any type information for this offset, append
			// a placeholder field.
			if (hungry) {
				CppType& field = pvar_type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
				field.name = stringf("unknown_%x", offset);
				field.offset = offset;
				field.size = 4;
				field.alignment = 4;
				offset += 4;
				field.built_in = CPP_INT;
			}
		}
	}
}

static void rewrite_pvar_links(std::vector<u8>& data, const CppType& type, s32 offset, const std::map<std::string, CppType>& types, const Instances& instances, const char* context);
static void enumerate_moby_links(std::vector<PvarFixupEntry>& dest, const CppType& type, s32 offset, s32 pvar_index, const std::map<std::string, CppType>& types);

void build_pvars(Gameplay& dest, const Instances& src, const std::map<std::string, CppType>& types_src)
{
	dest.pvar_table.emplace();
	dest.pvar_data.emplace();
	dest.pvar_moby_links.emplace();
	dest.pvar_relative_pointers.emplace();
	dest.shared_data.emplace();
	dest.shared_data_table.emplace();
	
	std::map<s32, s32> shared_data_offsets;
	for (const SharedDataInstance& inst : src.shared_data) {
		shared_data_offsets[inst.id().value] = dest.shared_data->size();
		dest.shared_data->insert(dest.shared_data->end(), BEGIN_END(inst.pvars().data));
	}
	
	auto func = [&](const Instance& inst) {
		std::vector<u8> pvars = inst.pvars().data;
		if (pvars.empty()) {
			return;
		}
		
		std::string type_name = pvar_type_name_from_instance(inst);
		if (type_name.empty()) {
			return;
		}
		
		auto type_iter = types_src.find(type_name);
		verify(type_iter != types_src.end(), "Failed to lookup pvar type '%s'.", type_name.c_str());
		const CppType& type = type_iter->second;
		verify(type.descriptor == CPP_STRUCT_OR_UNION && !type.struct_or_union.is_union, "Pvar type must be a struct.");
		
		// TODO: Figure out why this is failing with == instead of <=.
		//verify(align32(type.size, 16) <= pvars.size(),
		//	"Pvar data is the wrong size for type %s (%d vs %d). Size should be a multiple of 16 bytes.",
		//	type_name.c_str(), (s32) pvars.size(), align32(type.size, 16));
		
		// HACK: This check is only here because the it's failing for moby class
		// 3107 in Deadlocked.
		if (align32(type.size, 16) <= pvars.size()) {
			std::string context = stringf("pvar type %s", type.name.c_str());
			rewrite_pvar_links(pvars, type, 0, types_src, src, context.c_str());
			
			inst.pvars().temp_pvar_index = (s32) dest.pvar_table->size();
			PvarTableEntry& entry = dest.pvar_table->emplace_back();
			entry.offset = dest.pvar_data->size();
			entry.size = (s32) pvars.size();
			dest.pvar_data->insert(dest.pvar_data->end(), BEGIN_END(pvars));
			
			// Write fixup entries for moby links, so they can be rewritten at load
			// time by the game.
			std::vector<PvarFixupEntry> fixups;
			enumerate_moby_links(fixups, type, 0, inst.pvars().temp_pvar_index, types_src);
			for (const PvarFixupEntry& fixup : fixups) {
				s32 link = *(s32*) &pvars[fixup.offset];
				if (link > -1) {
					dest.pvar_moby_links->emplace_back(fixup);
				}
			}
			
			// Write fixup entries for relative pointers. These allows the game to
			// convert the sub var pointers and more to absolute pointers on load.
			for (const PvarPointer& pointer : inst.pvars().pointers) {
				if (pointer.type == PvarPointerType::RELATIVE) {
					PvarFixupEntry& entry = dest.pvar_relative_pointers->emplace_back();
					entry.pvar_index = inst.pvars().temp_pvar_index;
					entry.offset = pointer.offset;
				}
			}
			
			// Write fixup entries for pointers to the shared data, so that again
			// they can be converted to absolute pointers at load time.
			for (const PvarPointer& pointer : inst.pvars().pointers) {
				if (pointer.type == PvarPointerType::SHARED) {
					SharedDataEntry& entry = dest.shared_data_table->emplace_back();
					entry.pvar_index = (u16) inst.pvars().temp_pvar_index;
					entry.pointer_offset = (u16) pointer.offset;
					auto iter = shared_data_offsets.find(pointer.shared_data_id);
					verify(iter != shared_data_offsets.end(), "No shared data instance exists with ID '%d'.", pointer.shared_data_id);
					entry.shared_data_offset = iter->second;
				}
			}
		}
	};
	
	for (const Instance& inst : src.cameras) {
		func(inst);
	}
	for (const Instance& inst : src.sound_instances) {
		func(inst);
	}
	for (const Instance& inst : src.moby_instances) {
		func(inst);
	}
}

static void rewrite_pvar_links(
	std::vector<u8>& data,
	const CppType& type,
	s32 offset,
	const std::map<std::string, CppType>& types,
	const Instances& instances,
	const char* context)
{
	switch (type.descriptor) {
		case CPP_ARRAY: {
			for (s32 i = 0; i < type.array.element_count; i++) {
				rewrite_pvar_links(data, *type.array.element_type, offset + i * type.array.element_type->size, types, instances, context);
			}
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			if (!type.struct_or_union.is_union) {
				for (const CppType& field : type.struct_or_union.fields) {
					rewrite_pvar_links(data, field, offset + field.offset, types, instances, context);
				}
			}
			break;
		}
		case CPP_TYPE_NAME: {
			if (type.type_name.string.ends_with("link")) {
				verify(type.size == 4 && offset + 4 <= data.size(), "Size error rewriting link.");
				s32* link = (s32*) (data.data() + offset); 
				*link = rewrite_link(*link, type.type_name.string.c_str(), instances, context);
			} else {
				auto iter = types.find(type.type_name.string);
				verify(iter != types.end(), "Failed to lookup type '%s'.", type.type_name.string.c_str());
				rewrite_pvar_links(data, iter->second, offset, types, instances, context);
			}
			break;
		}
		default: {}
	}
}

static void enumerate_moby_links(
	std::vector<PvarFixupEntry>& dest,
	const CppType& type,
	s32 offset,
	s32 pvar_index,
	const std::map<std::string, CppType>& types)
{
	switch (type.descriptor) {
		case CPP_ARRAY: {
			for (s32 i = 0; i < type.array.element_count; i++) {
				enumerate_moby_links(dest, *type.array.element_type, offset + i * type.array.element_type->size, pvar_index, types);
			}
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			if (!type.struct_or_union.is_union) {
				for (const CppType& field : type.struct_or_union.fields) {
					enumerate_moby_links(dest, field, offset + field.offset, pvar_index, types);
				}
			}
			break;
		}
		case CPP_TYPE_NAME: {
			if (type.type_name.string == "mobylink") {
				PvarFixupEntry& entry = dest.emplace_back();
				entry.pvar_index = pvar_index;
				entry.offset = offset;
			} else {
				auto iter = types.find(type.type_name.string);
				verify(iter != types.end(), "Failed to lookup type '%s'.", type.type_name.string.c_str());
				enumerate_moby_links(dest, iter->second, offset, pvar_index, types);
			}
			break;
		}
		default: {}
	}
}

std::string pvar_type_name_from_instance(const Instance& inst)
{
	std::string type_name;
	if (inst.type() == INST_MOBY) {
		type_name = stringf("update%d", inst.o_class());
	} else if (inst.type() == INST_CAMERA) {
		type_name = stringf("camera%d", inst.o_class());
	} else if (inst.type() == INST_SOUND) {
		type_name = stringf("sound%d", inst.o_class());
	}
	return type_name;
}
