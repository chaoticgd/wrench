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
		{"format_version", 0},
		{"application", application},
		{"application_version", get_application_version_string()}
	};
}

static const char* APPLICATION_NAME = "Wrench WAD Utility";

Json write_gameplay_json(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = get_file_metadata("gameplay", APPLICATION_NAME);
	
	Json data = to_json(gameplay);
	for(auto& item : data.items()) {
		json[item.key()] = item.value();
	}
	
	return json;
}

Json write_help_messages_json(HelpMessages& help_messages) {
	Json json;
	
	json["metadata"] = get_file_metadata("help_messages", APPLICATION_NAME);
	
	Json data = to_json(help_messages);
	for(auto& item : data.items()) {
		json[item.key()] = item.value();
	}
	
	return json;
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
			return 4;
		default:
			assert(0);
	}
}

bool PvarType::insert_field(PvarField to_insert) {
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

#define CHECK(condition) if(!(condition)) return nullptr

std::unique_ptr<Wad> read_wad_json(fs::path src_path) {
	fs::path src_dir = src_path.parent_path();
	Json json = Json::parse(read_file(src_path));
	
	Game game;
	CHECK(json.contains("game") && json["game"].is_string());
	std::string game_str = json["game"];
	if(game_str == "R&C1") {
		game = Game::RAC1;
	} else if(game_str == "R&C2") {
		game = Game::RAC2;
	} else if(game_str == "R&C3") {
		game = Game::RAC3;
	} else if(game_str == "Deadlocked") {
		game = Game::DL;
	} else {
		fprintf(stderr, "error: Invalid game.\n");
		return nullptr;
	}
	
	WadType type;
	CHECK(json.contains("type") && json["type"].is_string());
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
			wad.game = game;
			wad.type = type;
			wad.level_number = json["level_number"];
			wad.reverb = json["reverb"];
			wad.primary = read_file(src_dir/json["primary"]);
			wad.core_bank = read_file(src_dir/json["core_sound_bank"]);
			Json gameplay_json = Json::parse(read_file(src_dir/json["gameplay"]));
			from_json(wad.gameplay, gameplay_json);
			map_from_json(wad.camera_classes, Json::parse(read_file(src_dir/json["camera_classes"])), "class");
			map_from_json(wad.sound_classes, Json::parse(read_file(src_dir/json["sound_classes"])), "class");
			map_from_json(wad.moby_classes, Json::parse(read_file(src_dir/json["moby_classes"])), "class");
			map_from_json(wad.pvar_types, Json::parse(read_file(src_dir/json["pvar_types"])), "name");
			Json help_messages_json = Json::parse(read_file(src_dir/json["help_messages"]));
			from_json(wad.help_messages, help_messages_json);
			if(json.contains("chunks")) {
				for(Json& chunk_json : json["chunks"]) {
					Chunk chunk;
					if(chunk_json.contains("tfrags")) {
						chunk.tfrags = read_file(src_dir/chunk_json["tfrags"]);
					}
					if(chunk_json.contains("collision")) {
						chunk.collision = read_file(src_dir/chunk_json["collision"]);
					}
					if(chunk_json.contains("sound_bank")) {
						chunk.sound_bank = read_file(src_dir/chunk_json["sound_bank"]);
					}
					s32 index = chunk_json["index"];
					wad.chunks.emplace(index, std::move(chunk));
				}
			}
			if(json.contains("missions")) {
				for(Json& mission_json : json["missions"]) {
					Mission mission;
					if(mission_json.contains("instances")) {
						mission.instances = read_file(src_dir/mission_json["instances"]);
					}
					if(mission_json.contains("classes")) {
						mission.classes = read_file(src_dir/mission_json["classes"]);
					}
					if(mission_json.contains("sound_bank")) {
						mission.sound_bank = read_file(src_dir/mission_json["sound_bank"]);
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

void write_wad_json(fs::path dest_path, Wad* base) {
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
			json["primary"] = write_file(dest_path, "primary.bin", wad.primary);
			json["core_sound_bank"] = write_file(dest_path, "core_bank.bin", wad.core_bank);
			std::string gameplay_str = write_gameplay_json(wad.gameplay).dump(1, '\t');
			json["gameplay"] = write_file(dest_path, "gameplay.json", gameplay_str);
			std::string help_messages_str = write_help_messages_json(wad.help_messages).dump(1, '\t');
			json["camera_classes"] = write_file(dest_path, "camera_classes.json", map_to_json(wad.camera_classes, "class").dump(1, '\t'));
			json["sound_classes"] = write_file(dest_path, "sound_classes.json", map_to_json(wad.sound_classes, "class").dump(1, '\t'));
			json["moby_classes"] = write_file(dest_path, "moby_classes.json", map_to_json(wad.moby_classes, "class").dump(1, '\t'));
			json["pvar_types"] = write_file(dest_path, "pvar_types.json", map_to_json(wad.pvar_types, "name").dump(1, '\t'));
			json["help_messages"] = write_file(dest_path, "help_messages.json", help_messages_str);
			for(auto& [index, chunk] : wad.chunks) {
				auto chunk_name = [&](const char* name) {
					return "chunk" + std::to_string(index) + "_" + name + ".bin";
				};
				Json chunk_json;
				chunk_json["index"] = index;
				if(chunk.tfrags.has_value()) {
					chunk_json["tfrags"] = write_file(dest_path, chunk_name("tfrags"), *chunk.tfrags);
				}
				if(chunk.collision.has_value()) {
					chunk_json["collision"] = write_file(dest_path, chunk_name("collision"), *chunk.collision);
				}
				if(chunk.tfrags.has_value()) {
					chunk_json["sound_bank"] = write_file(dest_path, chunk_name("bank"), *chunk.sound_bank);
				}
				json["chunks"].emplace_back(chunk_json);
			}
			fs::path mission_instances_dir = "mission_instances";
			fs::path mission_classes_dir = "mission_classes";
			fs::path missions_banks_dir = "mission_banks";
			if(wad.missions.size() > 0) {
				fs::create_directory(dest_path/mission_instances_dir);
				fs::create_directory(dest_path/mission_classes_dir);
				fs::create_directory(dest_path/missions_banks_dir);
			}
			for(auto& [index, mission] : wad.missions) {
				auto mission_name = [&](fs::path name) {
					return name/(std::to_string(index) + ".bin");
				};
				Json mission_json;
				mission_json["index"] = index;
				if(mission.instances.has_value()) {
					mission_json["instances"] = write_file(dest_path, mission_name(mission_instances_dir), *mission.instances);
				}
				if(mission.classes.has_value()) {
					mission_json["classes"] = write_file(dest_path, mission_name(mission_classes_dir), *mission.classes);
				}
				if(mission.sound_bank.has_value()) {
					mission_json["sound_bank"] = write_file(dest_path, mission_name(missions_banks_dir), *mission.sound_bank);
				}
				json["missions"].emplace_back(std::move(mission_json));
			}
			break;
		}
		default:
			assert(0);
	}
	
	assert(json_file_name);
	write_file(dest_path, json_file_name, json.dump(1, '\t'));
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
	if(str == "struct") return PVAR_STRUCT;
	verify_not_reached("Invalid pvar field type.");
}
