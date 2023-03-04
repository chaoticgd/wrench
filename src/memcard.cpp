/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include <core/filesystem.h>
#include <core/memory_card.h>

static void print_usage(int argc, char** argv);

int main(int argc, char** argv) {
	if(argc != 4) {
		print_usage(argc, argv);
		return 1;
	}
	const char* command = argv[1];
	fs::path input_path = std::string(argv[2]);
	fs::path output_path = std::string(argv[3]);
	std::vector<u8> input = read_file(input_path);
	if(strcmp(command, "ls") == 0) {
		memory_card::SaveGame save = memory_card::read(input);
		printf("game:\n");
		for(size_t i = 0; i < save.game.size(); i++) {
			printf("\tsection of type %d: %d bytes\n", save.game[i].type, (s32) save.game[i].data.size());
		}
		for(s32 i = 0; i < (s32) save.levels.size(); i++) {
			printf("level %d:\n", i);
			for(size_t j = 0; j < save.levels[i].size(); j++) {
				printf("\tsection of type %d: %d bytes\n", save.levels[i][j].type, (s32) save.levels[i][j].data.size());
			}
		}
	} else {
		print_usage(argc, argv);
		return 1;
	}
}

static void print_usage(int argc, char** argv) {
	fprintf(stderr, "usage: %s ls|dump|prepare <input path> <output path>\n", (argc > 0) ? argv[0] : "memcard");
}
