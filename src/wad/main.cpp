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

#include "../core/util.h"
#include "../core/timer.h"
#include "moby.h"
#include "tests.h"
#include "wad_file.h"

static void extract(fs::path input_path, fs::path output_path);
static void build(fs::path input_path, fs::path output_path);
static void extract_moby(fs::path input_path, fs::path output_path, const char* game);

int main(int argc, char** argv) {
	if(argc == 3 || argc == 4) {
		std::string mode = argv[1];
		fs::path input_path = argv[2];
		if(mode == "extract") {
			extract(input_path, argc == 4 ? argv[3] : "wad_extracted");
			return 0;
		} else if(mode == "build") {
			build(input_path, argc == 4 ? argv[3] : "built.wad");
			return 0;
		} else if(mode == "test") {
			run_tests(input_path);
			return 0;
		}
	} else if(argc == 5) {
		std::string mode = argv[1];
		if(mode == "extract_moby_debug") {
			extract_moby(argv[2], argv[3], argv[4]);
			return 0;
		}
	}
	
	printf("Extract and build Ratchet & Clank WAD files.\n");
	printf("\n");
	printf("usage: \n");
	printf("  %s extract <input wad> <output dir>\n", argv[0]);
	printf("  %s build <input level json> <output wad>\n", argv[0]);
	printf("  %s test <level wads dir>\n", argv[0]);
	return 1;
}

static void extract(fs::path input_path, fs::path output_path) {
	start_timer("Extract WAD");
	FILE* wad_file = fopen(input_path.string().c_str(), "rb");
	verify(wad_file, "Failed to open input file.");
	std::unique_ptr<Wad> wad = read_wad(wad_file);
	write_wad_json(output_path, wad.get());
	fclose(wad_file);
	stop_timer();
}

static void build(fs::path input_path, fs::path output_path) {
	start_timer("Building WAD");
	start_timer("Collecting files");
	std::unique_ptr<Wad> wad = read_wad_json(input_path);
	stop_timer();
	FILE* wad_file = fopen(output_path.string().c_str(), "wb");
	verify(wad_file, "Failed to open output file for writing.");
	write_wad(wad_file, wad.get());
	fclose(wad_file);
	stop_timer();
}

static void extract_moby(fs::path input_path, fs::path output_path, const char* game) {
	auto moby_bin = read_file(input_path);
	MobyClassData moby_class;
	if(strcmp(game, "rac1") == 0) {
		moby_class = read_moby_class(moby_bin, Game::RAC1);
	} else if(strcmp(game, "rac2") == 0) {
		moby_class = read_moby_class(moby_bin, Game::RAC2);
	} else if(strcmp(game, "rac3") == 0) {
		moby_class = read_moby_class(moby_bin, Game::RAC3);
	} else if(strcmp(game, "dl") == 0) {
		moby_class = read_moby_class(moby_bin, Game::DL);
	} else {
		fprintf(stderr, "error: Invalid game.\n");
		exit(1);
	}
	write_file("/", output_path, write_collada(lift_moby_model(moby_class, -1, 0)));
}
