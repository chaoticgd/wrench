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

static void extract(fs::path input_path, fs::path output_path);
static void build(fs::path input_path, fs::path output_path);

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
	FILE* wad_file = fopen(input_path.string().c_str(), "rb");
	verify(wad_file, "Failed to open input file.");
	std::unique_ptr<Wad> wad = read_wad(wad_file);
	write_wad_json(output_path, wad.get());
	fclose(wad_file);
}

static void build(fs::path input_path, fs::path output_path) {
	std::unique_ptr<Wad> wad = read_wad_json(input_path);
	FILE* wad_file = fopen(output_path.string().c_str(), "wb");
	verify(wad_file, "Failed to open output file for writing.");
	write_wad(wad_file, wad.get());
	fclose(wad_file);
}
