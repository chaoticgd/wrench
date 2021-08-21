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

Json get_file_metadata(const char* format, const char* application) {
	return Json {
		{"format", format},
		{"format_version", 6},
		{"application", application},
		{"application_version", get_application_version_string()}
	};
}

static const char* APPLICATION_NAME = "Wrench WAD Utility";

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

CameraClass& LevelWad::lookup_camera_class(s32 class_number) {
	auto iter = camera_classes.find(class_number);
	if(iter != camera_classes.end()) {
		return iter->second;
	} else {
		CameraClass& class_object = camera_classes[class_number];
		class_object.pvar_type = "Camera" + std::to_string(class_number) + "Vars";
		return class_object;
	}
}

SoundClass& LevelWad::lookup_sound_class(s32 class_number) {
	auto iter = sound_classes.find(class_number);
	if(iter != sound_classes.end()) {
		return iter->second;
	} else {
		SoundClass& class_object = sound_classes[class_number];
		class_object.pvar_type = "Sound" + std::to_string(class_number) + "Vars";
		return class_object;
	}
}

MobyClass& LevelWad::lookup_moby_class(s32 class_number) {
	auto iter = moby_classes.find(class_number);
	if(iter != moby_classes.end()) {
		return iter->second;
	} else {
		MobyClass& class_object = moby_classes[class_number];
		class_object.pvar_type = "Moby" + std::to_string(class_number) + "Vars";
		return class_object;
	}
}

template <typename Map>
static void read_json_file_into_map(Map& map, const fs::path& src_dir, const Json& json, const char* name, const char* key_name) {
	map_from_json(map, Json::parse(read_file(src_dir/std::string(json[name])))[name], key_name);
}

Opt<Game> game_from_string(std::string str) {
	if(str == "R&C1") {
		return Game::RAC1;
	} else if(str == "R&C2") {
		return Game::RAC2;
	} else if(str == "R&C3") {
		return Game::RAC3;
	} else if(str == "Deadlocked") {
		return Game::DL;
	} else {
		return {};
	}
}

std::unique_ptr<Wad> read_wad_json(fs::path src_path) {
	fs::path src_dir = src_path.parent_path();
	Json json = Json::parse(read_file(src_path));
	
	if(!json.contains("game") || !json["game"].is_string()) {
		return nullptr;
	}
	Opt<Game> game = game_from_string(json["game"]);
	if(!game.has_value()) {
		fprintf(stderr, "error: Invalid game.\n");
		return nullptr;
	}
	
	WadType type;
	if(!json.contains("type") || !json["type"].is_string()) {
		return nullptr;
	}
	std::string type_str = json["type"];
	if(type_str == "level") {
		type = WadType::LEVEL;
	} else {
		fprintf(stderr, "error: Invalid WAD type.\n");
		return nullptr;
	}
	
	switch(type) {
		case WadType::LEVEL: {
			LevelWad wad;
			wad.game = *game;
			wad.type = type;
			wad.level_number = json["level_number"];
			if(wad.game != Game::RAC1) {
				wad.reverb = json["reverb"];
			}
			wad.code = read_file(src_dir/std::string(json["code"]));
			wad.asset_header = read_file(src_dir/std::string(json["asset_header"]));
			wad.small_textures = read_file(src_dir/std::string(json["small_textures"]));
			wad.hud_header = read_file(src_dir/std::string(json["hud_header"]));
			wad.hud_banks[0] = read_file(src_dir/std::string(json["hud_bank_0"]));
			wad.hud_banks[1] = read_file(src_dir/std::string(json["hud_bank_1"]));
			wad.hud_banks[2] = read_file(src_dir/std::string(json["hud_bank_2"]));
			wad.hud_banks[3] = read_file(src_dir/std::string(json["hud_bank_3"]));
			wad.hud_banks[4] = read_file(src_dir/std::string(json["hud_bank_4"]));
			wad.tfrags = read_file(src_dir/std::string(json["tfrags"]));
			wad.occlusion = read_file(src_dir/std::string(json["occlusion"]));
			wad.sky = read_file(src_dir/std::string(json["sky"]));
			//wad.collision = import_dae(read_file(src_dir/std::string(json["collision"]))).meshes.at(0);
			wad.collision_bin = read_file(src_dir/std::string(json["collision_bin"]));
			wad.textures = read_file(src_dir/std::string(json["textures"]));
			
			//wad.mobies = read_file(src_dir/std::string(json["mobies"]));
			//wad.ties = read_file(src_dir/std::string(json["ties"]));
			//wad.shrubs = read_file(src_dir/std::string(json["shrubs"]));
			wad.ratchet_seqs = read_file(src_dir/std::string(json["ratchet_seqs"]));
			verify(json.contains("moby8355_pvars") == (wad.game == Game::DL),
				(wad.game == Game::DL) ? "Missing moby8355_pvars file." : "moby8355_pvars present but not required.");
			if(wad.game == Game::DL) {
				wad.moby8355_pvars = read_file(src_dir/std::string(json["moby8355_pvars"]));
			}
			verify(json.contains("global_nav_data") == (wad.game == Game::DL),
				(wad.game == Game::DL) ? "Missing global_nav_data file." : "global_nav_data present but not required.");
			if(wad.global_nav_data.has_value()) {
				wad.global_nav_data = read_file(src_dir/std::string(json["global_nav_data"]));
			}
			if(wad.game != Game::RAC1) {
				wad.core_bank = read_file(src_dir/std::string(json["core_sound_bank"]));
			}
			read_json_file_into_map(wad.camera_classes, src_dir, json, "camera_classes", "class");
			read_json_file_into_map(wad.sound_classes, src_dir, json, "sound_classes", "class");
			read_json_file_into_map(wad.moby_classes, src_dir, json, "moby_classes", "class");
			read_json_file_into_map(wad.pvar_types, src_dir, json, "pvar_types", "name");
			from_json(wad.help_messages, Json::parse(read_file(src_dir/std::string(json["help_messages"]))));
			from_json(wad.gameplay, Json::parse(read_file(src_dir/std::string(json["gameplay"]))));
			if(json.contains("chunks")) {
				for(Json& chunk_json : json["chunks"]) {
					Chunk chunk;
					if(chunk_json.contains("tfrags")) {
						chunk.tfrags = read_file(src_dir/std::string(chunk_json["tfrags"]));
					}
					if(chunk_json.contains("collision")) {
						chunk.collision = read_file(src_dir/std::string(chunk_json["collision"]));
					}
					if(chunk_json.contains("sound_bank")) {
						chunk.sound_bank = read_file(src_dir/std::string(chunk_json["sound_bank"]));
					}
					s32 index = chunk_json["index"];
					wad.chunks.emplace(index, std::move(chunk));
				}
			}
			if(json.contains("missions")) {
				for(Json& mission_json : json["missions"]) {
					Mission mission;
					if(mission_json.contains("instances")) {
						mission.instances = read_file(src_dir/std::string(mission_json["instances"]));
					}
					if(mission_json.contains("classes")) {
						mission.classes = read_file(src_dir/std::string(mission_json["classes"]));
					}
					if(mission_json.contains("sound_bank")) {
						mission.sound_bank = read_file(src_dir/std::string(mission_json["sound_bank"]));
					}
					s32 index = mission_json["index"];
					verify(index >= 0 || index <= 127, "Mission index must be between 0 and 127.");
					wad.missions.emplace(index, std::move(mission));
				}
			}
			return std::make_unique<LevelWad>(std::move(wad));
		}
		default:
			assert(0);
	}
	return nullptr;
}

static std::string write_json_array_file(fs::path dest_dir, const char* file_name, Json data_json) {
	Json json;
	json["metadata"] = get_file_metadata(file_name, APPLICATION_NAME);
	json[file_name] = std::move(data_json);
	return write_file(dest_dir, std::string(file_name) + ".json", json.dump(1, '\t'));
}

static std::string write_json_object_file(fs::path dest_dir, const char* file_name, Json data_json) {
	Json json;
	json["metadata"] = get_file_metadata(file_name, APPLICATION_NAME);
	for(auto& item : data_json.items()) {
		json[item.key()] = std::move(item.value());
	}
	return write_file(dest_dir, std::string(file_name) + ".json", json.dump(1, '\t'));
}

void write_wad_json(fs::path dest_dir, Wad* base) {
	const char* json_file_name = nullptr;
	Json json;
	
	json["metadata"] = get_file_metadata("wad", APPLICATION_NAME);
	
	switch(base->game) {
		case Game::RAC1: json["game"] = "R&C1"; break;
		case Game::RAC2: json["game"] = "R&C2"; break;
		case Game::RAC3: json["game"] = "R&C3"; break;
		case Game::DL: json["game"] = "Deadlocked"; break;
		default: assert(false);
	}
	
	switch(base->type) {
		case WadType::LEVEL: {
			json_file_name = "level.json";
			LevelWad& wad = *dynamic_cast<LevelWad*>(base);
			json["type"] = "level";
			json["level_number"] = wad.level_number;
			if(wad.reverb.has_value()) {
				json["reverb"] = *wad.reverb;
			}
			json["code"] = write_file(dest_dir, "code.bin", wad.code);
			json["asset_header"] = write_file(dest_dir, "asset_header.bin", wad.asset_header);
			json["small_textures"] = write_file(dest_dir, "small_textures.bin", wad.small_textures);
			json["hud_header"] = write_file(dest_dir, "hud_header.bin", wad.hud_header);
			json["hud_bank_0"] = write_file(dest_dir, "hud_bank_0.bin", wad.hud_banks[0]);
			json["hud_bank_1"] = write_file(dest_dir, "hud_bank_1.bin", wad.hud_banks[1]);
			json["hud_bank_2"] = write_file(dest_dir, "hud_bank_2.bin", wad.hud_banks[2]);
			json["hud_bank_3"] = write_file(dest_dir, "hud_bank_3.bin", wad.hud_banks[3]);
			json["hud_bank_4"] = write_file(dest_dir, "hud_bank_4.bin", wad.hud_banks[4]);
			json["tfrags"] = write_file(dest_dir, "tfrags.bin", wad.tfrags);
			json["occlusion"] = write_file(dest_dir, "occlusion.bin", wad.occlusion);
			json["sky"] = write_file(dest_dir, "sky.bin", wad.sky);
			json["collision"] = write_file(dest_dir, "collision.dae", write_dae(mesh_to_dae(wad.collision)));
			json["collision_bin"] = write_file(dest_dir, "collision.bin", wad.collision_bin);
			json["textures"] = write_file(dest_dir, "textures.bin", wad.textures);
			
			fs::create_directory(dest_dir/std::string("mobies"));
			json["mobies"] = std::vector<std::string>{"mobies"};
			for(auto& [number, moby] : wad.moby_classes) {
				fs::path moby_dir = dest_dir/std::string("mobies")/std::to_string(number);
				fs::create_directories(moby_dir);
				Json moby_json;
				moby_json["class"] = number;
				moby_json["model"] = "model.bin";
				moby_json["pvar_type"] = moby.pvar_type;
				write_file(moby_dir, "model.bin", moby.model);
				write_file(moby_dir, "moby.json", moby_json.dump(1, '\t'));
				
			}
			
			fs::create_directory(dest_dir/std::string("ties"));
			json["ties"] = std::vector<std::string>{"ties"};
			for(auto& [number, tie] : wad.tie_classes) {
				fs::path tie_dir = dest_dir/std::string("ties")/std::to_string(number);
				fs::create_directories(tie_dir);
				Json tie_json;
				tie_json["class"] = number;
				tie_json["model"] = "model.bin";
				write_file(tie_dir, "model.bin", tie.model);
				write_file(tie_dir, "tie.json", tie_json.dump(1, '\t'));
			}
			
			fs::create_directory(dest_dir/std::string("shrubs"));
			json["shrubs"] = std::vector<std::string>{"shrubs"};
			for(auto& [number, shrub] : wad.shrub_classes) {
				fs::path shrub_dir = dest_dir/std::string("shrubs")/std::to_string(number);
				fs::create_directories(shrub_dir);
				Json shrub_json;
				shrub_json["class"] = number;
				shrub_json["model"] = "model.bin";
				write_file(shrub_dir, "model.bin", shrub.model);
				write_file(shrub_dir, "shrub.json", shrub_json.dump(1, '\t'));
			}
			
			json["ratchet_seqs"] = write_file(dest_dir, "ratchet_seqs.bin", wad.ratchet_seqs);
			if(wad.moby8355_pvars.has_value()) {
				json["moby8355_pvars"] = write_file(dest_dir, "moby8355_pvars.bin", *wad.moby8355_pvars);
			}
			if(wad.global_nav_data.has_value()) {
				json["global_nav_data"] = write_file(dest_dir, "global_nav_data.bin", *wad.global_nav_data);
			}
			json["core_sound_bank"] = write_file(dest_dir, "core_bank.bin", wad.core_bank);
			json["camera_classes"] = write_json_array_file(dest_dir, "camera_classes", map_to_json(wad.camera_classes, "class"));
			json["sound_classes"] = write_json_array_file(dest_dir, "sound_classes", map_to_json(wad.sound_classes, "class"));
			json["moby_classes"] = write_json_array_file(dest_dir, "moby_classes", map_to_json(wad.moby_classes, "class"));
			json["pvar_types"] = write_json_array_file(dest_dir, "pvar_types", map_to_json(wad.pvar_types, "name"));
			json["help_messages"] = write_json_object_file(dest_dir, "help_messages", to_json(wad.help_messages));
			json["gameplay"] = write_json_object_file(dest_dir, "gameplay", to_json(wad.gameplay));
			for(auto& [index, chunk] : wad.chunks) {
				auto chunk_name = [&](const char* name) {
					return "chunk" + std::to_string(index) + "_" + name + ".bin";
				};
				Json chunk_json;
				chunk_json["index"] = index;
				if(chunk.tfrags.has_value()) {
					chunk_json["tfrags"] = write_file(dest_dir, chunk_name("tfrags"), *chunk.tfrags);
				}
				if(chunk.collision.has_value()) {
					chunk_json["collision"] = write_file(dest_dir, chunk_name("collision"), *chunk.collision);
				}
				if(chunk.sound_bank.has_value()) {
					chunk_json["sound_bank"] = write_file(dest_dir, chunk_name("bank"), *chunk.sound_bank);
				}
				json["chunks"].emplace_back(chunk_json);
			}
			fs::path mission_instances_dir = "mission_instances";
			fs::path mission_classes_dir = "mission_classes";
			fs::path missions_banks_dir = "mission_banks";
			if(wad.missions.size() > 0) {
				fs::create_directory(dest_dir/mission_instances_dir);
				fs::create_directory(dest_dir/mission_classes_dir);
				fs::create_directory(dest_dir/missions_banks_dir);
			}
			for(auto& [index, mission] : wad.missions) {
				auto mission_name = [&](fs::path name) {
					return name/(std::to_string(index) + ".bin");
				};
				Json mission_json;
				mission_json["index"] = index;
				if(mission.instances.has_value()) {
					mission_json["instances"] = write_file(dest_dir, mission_name(mission_instances_dir), *mission.instances);
				}
				if(mission.classes.has_value()) {
					mission_json["classes"] = write_file(dest_dir, mission_name(mission_classes_dir), *mission.classes);
				}
				if(mission.sound_bank.has_value()) {
					mission_json["sound_bank"] = write_file(dest_dir, mission_name(missions_banks_dir), *mission.sound_bank);
				}
				json["missions"].emplace_back(std::move(mission_json));
			}
			break;
		}
		default:
			assert(0);
	}
	
	assert(json_file_name);
	write_file(dest_dir, json_file_name, json.dump(1, '\t'));
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
