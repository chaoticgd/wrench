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
# 	Match the contents of a table of contents with loose files.
# */

#include <stdio.h>

#include "../util.h"
#include "../fs_includes.h"
#include "../command_line.h"
#include "../formats/toc.h"

int main(int argc, char** argv) {
	cxxopts::Options options("Match the contents of a table of contents with loose files.");
	options.add_options()
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("d,dir", "The directory of files to correlate ToC entries with.",
			cxxopts::value<std::string>())
		("o,offset", "The offset in the input file where the table of contents begins.",
			cxxopts::value<std::string>()->default_value("0x1f4800"));

	options.parse_positional({
		"src", "dir", "offset"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string src_path = cli_get(args, "src");
	std::string match_dir = cli_get(args, "dir");
	std::size_t toc_base = parse_number(cli_get_or(args, "offset", "0x1f4800"));

	file_stream iso(src_path);
	
	array_stream toc_stream;
	iso.seek(toc_base);
	stream::copy_n(toc_stream, iso, TOC_MAX_SIZE);
	
	table_of_contents toc = read_toc(iso, toc_base);
	
	std::vector<uint8_t> loose_file_buffer;
	
	// Match tables to files.
	for(auto file_name : fs::directory_iterator(match_dir)) {
		if(!file_name.is_regular_file()) {
			continue;
		}
		
		file_stream loose_file(file_name.path());
		uint32_t size = loose_file.read<uint32_t>();
		if(size > 0xffff) {
			continue;
		}
		loose_file.seek(loose_file.tell() + 4);
		loose_file_buffer.resize(size - 8);
		loose_file.read_v(loose_file_buffer);
		for(std::size_t i = 0; i < toc.tables.size(); i++) {
			toc_table& table = toc.tables[i];
			if(loose_file_buffer.size() != table.data.size()) {
				continue;
			}
			
			if(std::memcmp(loose_file_buffer.data(), table.data.data(), loose_file_buffer.size()) != 0) {
				continue;
			}
			
			printf("Matched table %ld at toc+0x%04x with file %s\n", i, table.offset_in_toc, file_name.path().string().c_str());
		}
	}
	
	loose_file_buffer.resize(SECTOR_SIZE);
	
	// Match levels to files.
	for(auto file_name : fs::directory_iterator(match_dir)) {
		if(!file_name.is_regular_file()) {
			continue;
		}
		file_stream loose_file(file_name.path());
		if(loose_file.size() < SECTOR_SIZE) {
			continue;
		}
		loose_file.read_v(loose_file_buffer);
		
		auto toc_compare_sector = [&](sector32 sect) {
			toc_stream.write<uint32_t>(sect.bytes() - toc_base + 4, 0); // Zero out the base offset (it's zero in the loose files).
			return std::memcmp(loose_file_buffer.data(), &toc_stream.buffer[sect.bytes() - toc_base], SECTOR_SIZE) == 0;
		};
		
		for(std::size_t i = 0; i < toc.levels.size(); i++) {
			toc_level& lvl = toc.levels[i];
			if(toc_compare_sector(lvl.main_part)) {
				printf("Matched main part at toc+0x%04lx of level %ld with file %s.\n",
					lvl.main_part.bytes() - toc_base, i, file_name.path().string().c_str());
			}
			if(lvl.audio_part.sectors != 0 && toc_compare_sector(lvl.audio_part)) {
				printf("Matched audio part at toc+0x%04lx of level %ld with file %s.\n",
					lvl.audio_part.bytes() - toc_base, i, file_name.path().string().c_str());
			}
			if(lvl.scene_part.sectors != 0 && toc_compare_sector(lvl.scene_part)) {
				printf("Matched scene part at toc+0x%04lx of level %ld with file %s.\n",
					lvl.scene_part.bytes() - toc_base, i, file_name.path().string().c_str());
			}
		}
	}
}
