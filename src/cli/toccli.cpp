/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

# /*
# 	Read the game's table of contents.
# */

#include <stdio.h>

#include "../util.h"
#include "../command_line.h"
#include "../formats/toc.h"

int main(int argc, char** argv) {
	cxxopts::Options options("Read the game's table of contents");
	options.add_options()
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("o,offset", "The offset in the input file where the table of contents begins.",
			cxxopts::value<std::string>()->default_value("0x1f4800"));

	options.parse_positional({
		"src", "offset"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string src_path = cli_get(args, "src");
	std::size_t offset = parse_number(cli_get_or(args, "offset", "0x1f4800"));

	file_stream src(src_path);
	table_of_contents toc = read_toc(src, offset);
	
	printf("+-[Non-level Sections]--+-------------+-------------+\n");
	printf("| Index | Offset in ToC | Size in ToC | Data Offset |\n");
	printf("| ----- | ------------- | ----------- | ----------- |\n");
	for(size_t i = 0; i < toc.tables.size(); i++) {
		toc_table& table = toc.tables[i];
		size_t table_size = sizeof(toc_table_header) + table.data.size();
		size_t base_offset = table.header.base_offset.bytes();
	printf("| %02ld    | %08x      | %08lx    | %08lx    |\n",
		i, table.offset_in_toc, table_size, base_offset);
	}
	printf("+-------+---------------+-------------+-------------+\n");
	
	printf("+-[Level Table]------------------+------------------------+------------------------+\n");
	printf("|       | LEVELn.WAD             | AUDIOn.WAD             | SCENEn.WAD             |\n");
	printf("|       | ----------             | ----------             | ----------             |\n");
	printf("| Index | Offset      Size       | Offset      Size       | Offset      Size       |\n");
	printf("| ----- | ------      ----       | ------      ----       | ------      ----       |\n");
	for(size_t i = 0; i < toc.levels.size(); i++) {
		toc_level& lvl = toc.levels[i];
		size_t main_part_base = src.read<sector32>(lvl.main_part.bytes() + 4).bytes();
		printf("| %02ld    | %010lx  %010lx |",
			i, main_part_base, lvl.main_part_size.bytes());
		if(lvl.audio_part.sectors != 0) {
			size_t audio_part_base = src.read<sector32>(lvl.audio_part.bytes() + 4).bytes();
			printf(" %010lx  %010lx |", audio_part_base, lvl.audio_part_size.bytes());
		} else {
			printf(" N/A         N/A        |");
		}
		if(lvl.scene_part.sectors != 0) {
			size_t scene_part_base = src.read<sector32>(lvl.scene_part.bytes() + 4).bytes();
			printf(" %010lx  %010lx |", scene_part_base, lvl.scene_part_size.bytes());
		} else {
			printf(" N/A         N/A        |");
		}
		printf("\n");
	}
	printf("+-------+------------------------+------------------------+------------------------+\n");
}
