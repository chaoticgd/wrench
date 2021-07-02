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
	
	const std::vector<u8> header = read_header(file);
	const WadFileDescription file_desc = match_wad(file, header);
	std::unique_ptr<Wad> wad = file_desc.create();
	assert(wad.get());
	Game game;
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		for(s32 i = 0; i < lump_desc.count; i++) {
			auto& [offset, size] = Buffer(header).read<SectorRange>(lump_desc.offset + i * 8, "WAD header");
			if(size.sectors != 0) {
				std::vector<u8> src = read_lump(file, offset, size);
				lump_desc.funcs.read(lump_desc, *wad.get(), src, game);
			}
		}
	}
	
	printf("Detected game: ");
	switch(game) {
		case Game::RAC1: printf("R&C1"); break;
		case Game::RAC2: printf("R&C2"); break;
		case Game::RAC3: printf("R&C3"); break;
		case Game::DL: printf("Deadlocked"); break;
	}
	printf("\n");
	
	for(auto& [name, asset] : wad->binary_assets) {
		if(asset.is_array) {
			fs::path dir = output_path/name;
			fs::create_directories(dir);
			for(size_t i = 0; i < asset.buffers.size(); i++) {
				fs::path path = dir/(std::to_string(i) + ".bin");
				write_file(path.string().c_str(), asset.buffers[i]);
			}
		} else {
			assert(asset.buffers.size() == 1);
			fs::path path = output_path/(name + ".bin");
			write_file(path.string().c_str(), asset.buffers[0]);
		}
	}
	
	if(LevelWad* level = dynamic_cast<LevelWad*>(wad.get())) {
		Json json = write_gameplay_json(level->gameplay);
		std::string str = json.dump(1, '\t');
		
		fs::path path = output_path/"gameplay.json";
		write_file(path.string().c_str(), str);
		
		Json help_json = write_help_messages(level->gameplay);
		std::string help_str = help_json.dump(1, '\t');
		fs::path help_path = output_path/"help_messages.json";
		write_file(help_path.string().c_str(), help_str);
		
		fs::path mission_instances_dir =  output_path/"gameplay_mission_instances";
		fs::create_directories(mission_instances_dir);
		
		for(size_t i = 0; i < level->gameplay_mission_instances.size(); i++) {
			Gameplay& mission_instances = level->gameplay_mission_instances[i];
			Json mission_instances_json = write_gameplay_json(mission_instances);
			std::string mission_instances_str = mission_instances_json.dump(1, '\t');
			fs::path path = mission_instances_dir/(std::to_string(i) + ".json");
			write_file(path.string().c_str(), mission_instances_str);
		}
	}
	
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