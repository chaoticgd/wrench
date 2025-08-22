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

static std::vector<u8> extract_file(std::vector<u8>& file);

int main(int argc, const char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s <input file> <output file>\n", (argc > 0) ? argv[0] : "unpackbin");
		return 1;
	}
	
	fs::path input_path = argv[1];
	fs::path output_path = argv[2];
	
	std::vector<u8> input = read_file(input_path);
	std::vector<u8> decompressed = extract_file(input);
	ElfFile elf = read_ratchet_executable(decompressed);
	printf("%d sections\n", (s32) elf.sections.size());
	bool success = false;
	if (elf.sections.size() == DONOR_UYA_BOOT_ELF_HEADERS.sections.size()) {
		success = fill_in_elf_headers(elf, DONOR_UYA_BOOT_ELF_HEADERS);
	} else if (elf.sections.size() == DONOR_DL_BOOT_ELF_HEADERS.sections.size()) {
		success = fill_in_elf_headers(elf, DONOR_DL_BOOT_ELF_HEADERS);
	} else if (elf.sections.size() == DONOR_RAC_GC_UYA_LEVEL_ELF_HEADERS.sections.size()) {
		success = fill_in_elf_headers(elf, DONOR_RAC_GC_UYA_LEVEL_ELF_HEADERS);
	} else if (elf.sections.size() == DONOR_DL_LEVEL_ELF_NOBITS_HEADERS.sections.size()) {
		if (elf.sections[2].header.type == SHT_NOBITS) {
			success = fill_in_elf_headers(elf, DONOR_DL_LEVEL_ELF_NOBITS_HEADERS);
		} else {
			success = fill_in_elf_headers(elf, DONOR_DL_LEVEL_ELF_PROGBITS_HEADERS);
		}
	}
	if (!success) {
		fprintf(stderr, "warning: Failed to recover section information!\n");
	}
	std::vector<u8> output;
	write_elf_file(output, elf);
	write_file(output_path, output);
	return 0;
}

static std::vector<u8> extract_file(std::vector<u8>& file)
{
	s64 wad_ofs = -1;
	for (s64 i = 0; i < file.size() - 3; i++) {
		if (memcmp(&file.data()[i], "WAD", 3) == 0) {
			wad_ofs = i;
			break;
		}
	}
	if (wad_ofs > -1) {
		std::vector<u8> decompressed;
		decompress_wad(decompressed, WadBuffer{file.data() + wad_ofs, file.data() + file.size()});
		file.clear();
		return decompressed;
	} else {
		return std::move(file);
	}
}
