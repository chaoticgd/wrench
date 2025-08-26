/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

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

#include <catch2/catch_amalgamated.hpp>
#include <core/elf.h>
#include <core/filesystem.h>

TEST_CASE("ELF file preserved", "[elf]")
{
	const char* path = "test/data/test.elf";
	if (!fs::exists(path)) {
		return;
	}
	std::vector<u8> input = read_file(path);
	ElfFile elf = read_elf_file(input);
	std::vector<u8> output;
	write_elf_file(output, elf);
	REQUIRE(diff_buffers(input, output, 0, DIFF_REST_OF_BUFFER, true));
}
