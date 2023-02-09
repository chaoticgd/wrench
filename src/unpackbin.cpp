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

#include <core/elf.h>
#include <core/filesystem.h>

int main(int argc, const char** argv) {
	if(argc < 3) {
		if(argc > 0) {
			printf("usage: %s <input file> <output file>\n", argv[0]);
		}
		return 1;
	}
	
	fs::path input_path = argv[1];
	fs::path output_path = argv[2];
	
	std::vector<u8> input = read_file(input_path);
	ElfFile elf = read_ratchet_executable(input);
	printf("%d sections\n", (s32) elf.sections.size());
	recover_deadlocked_section_info(elf);
	std::vector<u8> output;
	write_elf_file(output, elf);
	write_file(output_path, output);
	return 0;
}
