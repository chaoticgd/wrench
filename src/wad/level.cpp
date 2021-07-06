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

void read_gameplay_json(Gameplay& gameplay, Json& json) {
	from_json(gameplay, json);
}

template <typename Object>
void write_gameplay_section(Json& dest, std::optional<Object>& src, const char* name) {
	if(!src.has_value()) {
		dest[name] = {};
		return;
	}
	dest[name] = to_json(*src);
}

template <typename Object>
void write_gameplay_section(Json& dest, std::optional<std::vector<Object>>& src, const char* name) {
	if(!src.has_value()) {
		dest[name] = {};
		return;
	}
	for(Object& object : *src) {
		dest[name].emplace_back(to_json(object));
	}
}

Json write_gameplay_json(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = Json {
		{"format", "gameplay"},
		{"format_version", 0},
		{"application", "Wrench WAD Utility"},
		{"application_version", get_application_version_string()}
	};
	
	Json data = to_json(gameplay);
	for(auto& item : data.items()) {
		json[item.key()] = item.value();
	}
	
	return json;
}

void read_help_messages(Gameplay& gameplay, Json& json) {
	
}

Json write_help_messages(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = Json {
		{"format", "help_messages"},
		{"format_version", 0},
		{"application", "Wrench WAD Utility"},
		{"application_version", get_application_version_string()}
	};
	
	struct {
		const char* name;
		Opt<std::vector<HelpMessage>>* messages;
	} languages[8] = {
		{"us_english", &gameplay.us_english_help_messages},
		{"uk_english", &gameplay.uk_english_help_messages},
		{"french", &gameplay.french_help_messages},
		{"german", &gameplay.german_help_messages},
		{"spanish", &gameplay.spanish_help_messages},
		{"italian", &gameplay.italian_help_messages},
		{"japanese", &gameplay.japanese_help_messages},
		{"korean", &gameplay.korean_help_messages}
	};
	for(auto& language : languages) {
		if(language.messages->has_value()) {
			for(HelpMessage& message : **language.messages) {
				json[language.name].emplace_back(to_json(message));
			}
		}
	}
	return json;
}

void fixup_pvar_indices(Gameplay& gameplay) {
	s32 pvar_index = 0;
	if(gameplay.cameras.has_value()) {
		for(ImportCamera& camera : *gameplay.cameras) {
			if(camera.pvars.size() > 0) {
				camera.pvar_index = pvar_index++;
			} else {
				camera.pvar_index = -1;
			}
		}
	}
	if(gameplay.sound_instances.has_value()) {
		for(SoundInstance& inst : *gameplay.sound_instances) {
			if(inst.pvars.size() > 0) {
				inst.pvar_index = pvar_index++;
			} else {
				inst.pvar_index = -1;
			}
		}
	}
	if(gameplay.moby_instances.has_value()) {
		for(MobyInstance& inst : *gameplay.moby_instances) {
			if(inst.pvars.size() > 0) {
				inst.pvar_index = pvar_index++;
			} else {
				inst.pvar_index = -1;
			}
		}
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
			read_gameplay_json(wad.gameplay, gameplay_json);
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
	
	json["metadata"] = Json {
		{"format", "level"},
		{"format_version", 0},
		{"application", "Wrench WAD Utility"},
		{"application_version", get_application_version_string()}
	};
	
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
			for(auto& [index, chunk] : wad.chunks) {
				auto chunk_name = [&](const char* name) {
					return "chunk" + std::to_string(index) + "_" + name + ".bin";
				};
				json["chunks"].emplace_back(Json {
					{"index", index},
					{"tfrags", write_file(dest_path, chunk_name("tfrags"), chunk.tfrags)},
					{"collision", write_file(dest_path, chunk_name("collision"), chunk.collision)},
					{"sound_bank", write_file(dest_path, chunk_name("bank"), chunk.sound_bank)}
				});
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
				if(mission.instances.size() > 0) {
					mission_json["instances"] = write_file(dest_path, mission_name(mission_instances_dir), mission.instances);
				}
				if(mission.classes.size() > 0) {
					mission_json["classes"] = write_file(dest_path, mission_name(mission_classes_dir), mission.classes);
				}
				if(mission.sound_bank.size() > 0) {
					mission_json["sound_bank"] = write_file(dest_path, mission_name(missions_banks_dir), mission.sound_bank);
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
