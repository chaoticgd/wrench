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
	
	for(std::size_t i = 0; i < toc.tables.size(); i++) {
		toc_file_type type_enum = static_cast<toc_file_type>(toc.tables[i].header.size);
		const char* type = toc_file_type_to_string(type_enum);
		printf("Table %ld at toc+0x%04x with 0x%03lx entries of type %s.\n",
			i, toc.tables[i].offset_in_toc, toc.tables[i].entries.size(), type);
	}
	
	for(toc_level level : toc.levels) {
		printf("Level %02d with main part at 0x%08x, audio part at 0x%08x, scene part at 0x%08x.\n",
			level.main_part.level_number, level.main_part.base_offset, level.audio_part.bytes(), level.scene_part.bytes());
	}
}
