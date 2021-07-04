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

#include <fstream>

#include "util.h"
#include "tests.h"
#include "wad_file.h"

static void run_extractor(fs::path input_path, fs::path output_path);
static void run_raw_extractor(fs::path input_path, fs::path output_path);

int main(int argc, char** argv) {
	verify(argc == 3 || argc == 4, "Wrong number of arguments.");
	
	std::string mode = argv[1];
	fs::path input_path = argv[2];
	
	if(mode == "extract") {
		run_extractor(input_path, argc == 4 ? argv[3] : "wad_extracted");
	} else if(mode == "extract_raw") {
		run_raw_extractor(input_path, argc == 4 ? argv[3] : "raw_extracted");
	} else if(mode == "test") {
		verify(argc == 4, "No game specified.");
		Game game;
		if(strcmp(argv[3], "rac1") == 0) {
			game = Game::RAC1;
		} else if(strcmp(argv[3], "rac2") == 0) {
			game = Game::RAC2;
		} else if(strcmp(argv[3], "rac3") == 0) {
			game = Game::RAC3;
		} else if(strcmp(argv[3], "dl") == 0) {
			game = Game::DL;
		} else {
			verify_not_reached("Invalid game specified.");
		}
		run_tests(input_path, game);
	} else {
		verify_not_reached("Invalid command: %s", mode.c_str());
	}
}

static void run_extractor(fs::path input_path, fs::path output_path) {
	FILE* file = fopen(input_path.string().c_str(), "rb");
	verify(file, "Failed to open input file.");
	
	const std::vector<u8> header_vec = read_header(file);
	Buffer header(header_vec);
	const WadFileDescription file_desc = match_wad(file, header);
	std::unique_ptr<Wad> wad = file_desc.create();
	assert(wad.get());
	Game game;
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		if(lump_desc.funcs.read != nullptr) {
			for(s32 i = 0; i < lump_desc.count; i++) {
				auto& [offset, size] = header.read<SectorRange>(lump_desc.offset + i * 8, "WAD header");
				if(size.sectors != 0) {
					std::vector<u8> src = read_lump(file, offset, size);
					lump_desc.funcs.read(lump_desc, *wad.get(), src, game);
				}
			}
		}
	}
	
	const char* index_file_name;
	Json index_json;
	
	switch(game) {
		case Game::RAC1: index_json["game"] = "R&C1"; break;
		case Game::RAC2: index_json["game"] = "R&C2"; break;
		case Game::RAC3: index_json["game"] = "R&C3"; break;
		case Game::DL: index_json["game"] = "Deadlocked"; break;
	}
	
	if(LevelWad* level = dynamic_cast<LevelWad*>(wad.get())) {
		index_file_name = "level.json";
		
		auto level_number = find_lump(file_desc, "level_number");
		assert(level_number);
		index_json["level_number"] = header.read<s32>(level_number->offset, "level number");
		
		auto reverb = find_lump(file_desc, "reverb");
		if(reverb.has_value()) {
			index_json["reverb"] = header.read<s32>(reverb->offset, "reverb");
		}
		
		auto max_mission_size_1 = find_lump(file_desc, "max_mission_size_1");
		if(max_mission_size_1.has_value()) {
			index_json["max_mission_size_1"] = header.read<s32>(max_mission_size_1->offset, "max_mission_size_1");
		}
		
		auto max_mission_size_2 = find_lump(file_desc, "max_mission_size_2");
		if(max_mission_size_2.has_value()) {
			index_json["max_mission_size_2"] = header.read<s32>(max_mission_size_2->offset, "max_mission_size_2");
		}
		
		Json gameplay_json = write_gameplay_json(level->gameplay);
		fs::path path = "gameplay.json";
		write_file(output_path/path, gameplay_json.dump(1, '\t'));
		index_json["gameplay"] = path.string();
		
		Json help_json = write_help_messages(level->gameplay);
		fs::path help_path = "help_messages.json";
		write_file(output_path/help_path, help_json.dump(1, '\t'));
		index_json["help_messages"] = help_path.string();
		
		//fs::path mission_instances_dir =  output_path/"gameplay_mission_instances";
		//fs::create_directories(mission_instances_dir);
		//
		//for(size_t i = 0; i < level->gameplay_mission_instances.size(); i++) {
		//	Gameplay& mission_instances = level->gameplay_mission_instances[i];
		//	Json mission_instances_json = write_gameplay_json(mission_instances);
		//	std::string mission_instances_str = mission_instances_json.dump(1, '\t');
		//	fs::path path = mission_instances_dir/(std::to_string(i) + ".json");
		//	write_file(path.string().c_str(), mission_instances_str);
		//}
	}
	
	for(auto& [name, asset] : wad->binary_assets) {
		if(asset.is_array) {
			fs::create_directories(output_path/name);
			for(size_t i = 0; i < asset.buffers.size(); i++) {
				fs::path path = fs::path(name)/(std::to_string(i) + ".bin");
				write_file(output_path/path, asset.buffers[i]);
				index_json[name].emplace_back(path.string());
			}
		} else {
			assert(asset.buffers.size() == 1);
			fs::path path = name + ".bin";
			write_file(output_path/path, asset.buffers[0]);
			index_json[name] = path.string();
		}
	}
	
	write_file(output_path/index_file_name, index_json.dump(1, '\t'));
	
	fclose(file);
}

static void run_raw_extractor(fs::path input_path, fs::path output_path) {
	FILE* file = fopen(input_path.string().c_str(), "rb");
	verify(file, "Failed to open input file.");
	
	const std::vector<u8> header = read_header(file);
	const WadFileDescription file_desc = match_wad(file, header);
	std::unique_ptr<Wad> wad = file_desc.create();
	assert(wad.get());
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		if(lump_desc.count > 1) {
			fs::path dir = output_path/lump_desc.name;
			fs::create_directories(dir);
		}
		for(s32 i = 0; i < lump_desc.count; i++) {
			auto& [offset, size] = Buffer(header).read<SectorRange>(lump_desc.offset + i * 8, "WAD header");
			if(size.sectors != 0) {
				std::vector<u8> src = read_lump(file, offset, size);
				if(lump_desc.count > 1) {
					fs::path path = output_path/lump_desc.name/(std::to_string(i) + ".bin");
					write_file(path.string().c_str(), src);
				} else {
					fs::path path = output_path/(std::string(lump_desc.name) + ".bin");
					write_file(path.string().c_str(), src);
				}
			}
		}
	}
}
