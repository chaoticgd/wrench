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

// Tool to read VIF command lists.

#include <iostream>

#include <core/vif.h>
#include <core/filesystem.h>
#include <editor/util.h>
#include <editor/command_line.h>

int main(int argc, char** argv) {
	cxxopts::Options options("vif", "Parse PS2 VIF chains until an invalid VIF code is encountered.");
	options.add_options()
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("o,offset", "The offset in the input file where the VIF chain begins.",
			cxxopts::value<std::string>()->default_value("0"))
		("e,end", "The minimum offset where if we fail to parse a VIF code, we can stop searching.",
			cxxopts::value<std::string>()->default_value("0"));

	options.parse_positional({
		"src"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string src_path = cli_get(args, "src");
	std::size_t offset = parse_number(cli_get_or(args, "offset", "0"));
	std::size_t end_offset = parse_number(cli_get_or(args, "end", "0"));
	
	std::vector<u8> data = read_file(src_path);
	std::vector<VifPacket> command_list = read_vif_command_list(Buffer(data).subbuf(offset));
	while(command_list.size() > 0) {
		VifPacket& packet = command_list.front();
		if(packet.error == "") {
			std::string code_string = packet.code.to_string();
			printf("%08x %s\n", (s32) offset + packet.offset, code_string.c_str());
			command_list.erase(command_list.begin());
		} else {
			printf("%08x %s\n", (s32) offset + packet.offset, packet.error.c_str());
			if(offset + packet.offset > end_offset) {
				break;
			} else {
				command_list = read_vif_command_list(Buffer(data).subbuf(offset + packet.offset + 4, SIZE_MAX));
			}
		}
	}
}
