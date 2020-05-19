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
# 	CLI tool to parse VIF chains.
# */

#include <iostream>

#include "../util.h"
#include "../command_line.h"
#include "../formats/vif.h"

int main(int argc, char** argv) {
	cxxopts::Options options("vif", "Parse PS2 VIF chains until an invalid VIF code is encountered.");
	options.add_options()
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("o,offset", "The offset in the input file where the VIF chain begins.",
			cxxopts::value<std::string>()->default_value("0"));

	options.parse_positional({
		"src"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string src_path = cli_get(args, "src");
	std::size_t offset = parse_number(cli_get_or(args, "offset", "0"));

	file_stream src(src_path);
	std::vector<vif_packet> chain = parse_vif_chain(&src, offset, SIZE_MAX);
	for(vif_packet& packet : chain) {
		if(packet.error != "") {
			std::cout << packet.error << std::endl;
			break;
		}
		
		std::cout << std::hex << packet.address << " " << packet.code.to_string() << std::endl;
	}
}
