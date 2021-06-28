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

int main(int argc, char** argv) {
	verify(argc == 3 || argc == 4, "Wrong number of arguments.");
	
	std::string mode = argv[1];
	fs::path input_path = argv[2];
	
	if(mode == "extract") {
		run_extractor(input_path, argc == 4 ? argv[3] : "wad_extracted");
	} else if(mode == "test") {
		run_tests(input_path);
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
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		for(s32 i = 0; i < lump_desc.count; i++) {
			auto& [offset, size] = Buffer(header).read<SectorRange>(lump_desc.offset + i * 8, "WAD header");
			if(size.sectors != 0) {
				std::vector<u8> src = read_lump(file, offset, size);
				verify(lump_desc.types.read(lump_desc, *wad.get(), src), "Failed to convert lump.");
			}
		}
	}
	
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
		write_file(path.c_str(), Buffer((uint8_t*) &(*str.begin()), (uint8_t*) &(*str.end())));
	}
	
	fclose(file);
}
