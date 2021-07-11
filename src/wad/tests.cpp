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

static void run_gameplay_tests(fs::path input_path);
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

void run_tests(fs::path input_path) {
	run_gameplay_tests(input_path);
	
	printf("\nALL TESTS PASSED\n");
}

static void run_gameplay_tests(fs::path input_path) {
	for(fs::path wad_file_path : fs::directory_iterator(input_path)) {
		FILE* file = fopen(wad_file_path.string().c_str(), "rb");
		verify(file, "Failed to open input file.");
		
		auto build_args = [&](SectorRange lump, const char* name, const std::vector<GameplayBlockDescription>& blocks, bool compressed, Game game) {
			GameplayTestArgs args;
			args.wad_file_path = wad_file_path.string();
			args.file = file;
			args.lump = lump;
			args.name = name;
			args.blocks = &blocks;
			args.compressed = compressed;
			args.game = game;
			return args;
		};
		
		s32 header_size;
		verify(fread(&header_size, 4, 1, file) == 1, "Failed to read WAD header.");
		
		switch(header_size) {
			case sizeof(Rac23LevelWadHeader): {
				auto header = read_header<Rac23LevelWadHeader>(file);
				std::vector<u8> primary = read_lump(file, header.primary, "primary");
				Game game = detect_game_rac23(primary);
				run_gameplay_lump_test(build_args(header.gameplay, "gameplay", RAC23_GAMEPLAY_BLOCKS, true, game));
				break;
			}
			case sizeof(DeadlockedLevelWadHeader): {
				auto header = read_header<DeadlockedLevelWadHeader>(file);
				std::vector<u8> primary = read_lump(file, header.primary, "primary");
				
				run_gameplay_lump_test(build_args(header.gameplay_core, "gameplay core", DL_GAMEPLAY_CORE_BLOCKS, true, Game::DL));
				run_gameplay_lump_test(build_args(header.art_instances, "art instances", DL_ART_INSTANCE_BLOCKS, true, Game::DL));
				
				for(s32 i = 0; i < 128; i++) {
					std::string name = "mission instances " + std::to_string(i);
					run_gameplay_lump_test(build_args(header.gameplay_mission_instances[i], name.c_str(), DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS, false, Game::DL));
				}
				break;
			}
			default:
				verify_not_reached("Unable to identify '%s'.", wad_file_path.string().c_str());
		}
	}
}

static void run_gameplay_lump_test(GameplayTestArgs args) {
	if(args.lump.offset.sectors == 0) {
		return;
	}
	std::vector<u8> raw = read_lump(args.file, args.lump, args.name);
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
	LevelWad wad;
	Gameplay gameplay;
	read_gameplay(wad, gameplay, src, args.game, *args.blocks);
	std::vector<u8> dest = write_gameplay(wad, gameplay, args.game, *args.blocks);
	
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
	HelpMessages help_messages;
	help_messages.swap(gameplay); // Move the help messages into the help_messages object.
	Json help_messages_json = write_help_messages_json(help_messages);
	Gameplay test_gameplay;
	from_json(test_gameplay, gameplay_json);
	help_messages.swap(test_gameplay); // Move the help messages into the test_gameplay object.
	std::vector<u8> test_dest = write_gameplay(wad, test_gameplay, args.game, *args.blocks);
	OutBuffer(test_dest).pad(SECTOR_SIZE, 0);
	if(test_dest == dest) {
		fprintf(stderr, "%s JSON matches.\n", prefix_str.c_str());
	} else {
		fprintf(stderr, "File read from JSON doesn't match original.\n");
		write_file("/tmp/", "gameplay_orig.bin", dest);
		write_file("/tmp/", "gameplay_test.bin", test_dest);
		exit(1);
	}
}
