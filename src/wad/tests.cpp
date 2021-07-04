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

#include "tests.h"

static void run_gameplay_tests(fs::path input_path, Game game);
struct GameplayTestArgs {
	std::string wad_file_path;
	FILE* file;
	SectorRange lump = {0, 0};
	const char* name;
	const std::vector<GameplayBlockDescription>* blocks;
	bool compressed;
	Game game;
};
static void run_gameplay_lump_test(GameplayTestArgs args);


void run_tests(fs::path input_path, Game game) {
	run_gameplay_tests(input_path, game);
	
	printf("\nALL TESTS PASSED\n");
}

static void run_gameplay_tests(fs::path input_path, Game game) {
	for(fs::path wad_file_path : fs::directory_iterator(input_path)) {
		FILE* file = fopen(wad_file_path.string().c_str(), "rb");
		verify(file, "Failed to open input file.");
		
		const std::vector<u8> header = read_header(file);
		const WadFileDescription file_desc = match_wad(file, header);
		
		std::unique_ptr<Wad> wad = file_desc.create();
		assert(wad.get());
		
		auto build_args = [&](s32 header_offset, const char* name, const std::vector<GameplayBlockDescription>& blocks, bool compressed, Game game) {
			GameplayTestArgs args;
			args.wad_file_path = wad_file_path.string();
			args.file = file;
			args.lump = Buffer(header).read<SectorRange>(header_offset, "WAD header");
			args.name = name;
			args.blocks = &blocks;
			args.compressed = compressed;
			args.game = game;
			return args;
		};
		
		switch(file_desc.header_size) {
			case 0x60: { // GC/UYA
				verify(game == Game::RAC2 || game == Game::RAC3, "R&C2/R&C3 detected but other game specified.");
				auto gameplay_core = find_lump(file_desc, "gameplay_core");
				assert(gameplay_core.has_value());
				run_gameplay_lump_test(build_args(gameplay_core->offset, gameplay_core->name, RAC23_GAMEPLAY_BLOCKS, true, game));
				break;
			}
			case 0xc68: { // Deadlocked
				verify(game == Game::DL, "Deadlocked detected but other game specified.");
				auto gameplay_core = find_lump(file_desc, "gameplay_core");
				assert(gameplay_core.has_value());
				run_gameplay_lump_test(build_args(gameplay_core->offset, gameplay_core->name, DL_GAMEPLAY_CORE_BLOCKS, true, game));
				
				auto art_instances = find_lump(file_desc, "art_instances");
				assert(art_instances.has_value());
				run_gameplay_lump_test(build_args(art_instances->offset, art_instances->name, DL_ART_INSTANCE_BLOCKS, true, game));
				
				auto missions_instances = find_lump(file_desc, "gameplay_mission_instances");
				assert(missions_instances.has_value());
				for(s32 i = 0; i < missions_instances->count; i++) {
					std::string name = std::string(missions_instances->name) + " " + std::to_string(i);
					run_gameplay_lump_test(build_args(missions_instances->offset + i * 8, name.c_str(), DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS, false, game));
				}
				break;
			}
		}
	}
}

static void run_gameplay_lump_test(GameplayTestArgs args) {
	if(args.lump.offset.sectors == 0) {
		return;
	}
	std::vector<u8> raw = read_lump(args.file, args.lump.offset, args.lump.size);
	std::vector<u8> src;
	if(args.compressed) {
		verify(decompress_wad(src, raw), "Decompressing %s file failed.", args.name);
	} else {
		src = std::move(raw);
	}
	if(args.blocks == &DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS && src.size() >= 4 && *(s32*) src.data() == 0x90) {
		printf("warning: Skipping %s %s. Weird format.\n", args.wad_file_path.c_str(), args.name);
		return;
	}
	
	// Test the binary reading/writing functions.
	Gameplay gameplay;
	read_gameplay(gameplay, src, args.game, *args.blocks);
	std::vector<u8> dest = write_gameplay(gameplay, args.game, *args.blocks);
	
	// The input buffer may or may not already be padded to the sector size.
	OutBuffer(dest).pad(SECTOR_SIZE, 0);
	OutBuffer(src).pad(SECTOR_SIZE, 0);
	
	Buffer dest_buf(dest);
	Buffer src_buf(src);
	
	bool good = true;
	
	std::string prefix_str = args.wad_file_path + " " + args.name;
	std::string gameplay_header_str = prefix_str + " header";
	std::string gameplay_data_str = prefix_str + " data";
	good &= diff_buffers(src_buf.subbuf(0, 0xa0), dest_buf.subbuf(0, 0xa0), 0, gameplay_header_str.c_str());
	good &= diff_buffers(src_buf.subbuf(0xa0), dest_buf.subbuf(0xa0), 0xa0, gameplay_data_str.c_str());
	
	if(!good) {
		FILE* gameplay_file = fopen("/tmp/gameplay.bin", "wb");
		verify(gameplay_file, "Failed to open /tmp/gameplay.bin for writing.");
		fwrite(src.data(), src.size(), 1, gameplay_file);
		fclose(gameplay_file);
		exit(1);
	}
	
	// Test the JSON reading/writing functions.
	Json gameplay_json = write_gameplay_json(gameplay);
	Json help_messages_json = write_help_messages(gameplay);
	Gameplay test_gameplay;
	read_gameplay_json(test_gameplay, gameplay_json);
	read_help_messages(test_gameplay, help_messages_json);
	std::vector<u8> test_dest = write_gameplay(test_gameplay, args.game, *args.blocks);
	OutBuffer(test_dest).pad(SECTOR_SIZE, 0);
	if(test_dest == dest) {
		fprintf(stderr, "%s JSON matches.\n", prefix_str.c_str());
	} else {
		fprintf(stderr, "File read from JSON doesn't match original.\n");
		write_file("/tmp/gameplay_orig.bin", dest);
		write_file("/tmp/gameplay_test.bin", test_dest);
		exit(1);
	}
}
