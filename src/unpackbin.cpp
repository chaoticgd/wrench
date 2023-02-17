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
#include <engine/compression.h>

static std::vector<u8> decompress_file(const std::vector<u8>& file);

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
	std::vector<u8> decompressed = decompress_file(input);
	ElfFile elf = read_ratchet_executable(decompressed);
	printf("%d sections\n", (s32) elf.sections.size());
	if(!fill_in_elf_headers(elf, EXPECTED_DEADLOCKED_BOOT_ELF_HEADERS)) {
		fprintf(stderr, "warning: Failed to recover section information!\n");
	}
	std::vector<u8> output;
	write_elf_file(output, elf);
	write_file(output_path, output);
	return 0;
}

static std::vector<u8> decompress_file(const std::vector<u8>& file) {
	s64 wad_ofs = -1;
	for(s64 i = 0; i < file.size() - 3; i++) {
		if(memcmp(&file.data()[i], "WAD", 3) == 0) {
			wad_ofs = i;
			break;
		}
	}
	verify(wad_ofs > -1, "Cannot find 'WAD' magic bytes (LZ compression header).");
	std::vector<u8> decompressed;
	decompress_wad(decompressed, WadBuffer{file.data() + wad_ofs, file.data() + file.size()});
	return decompressed;
}
