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

# /*
# 	CLI tool to parse VIF chains.
# */

#include <iostream>

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
	
	// TODO
	//file_stream src(src_path);
	//std::vector<vif_packet> chain = parse_vif_chain(&src, offset, SIZE_MAX);
	//while(chain.size() > 0) {
	//	vif_packet& packet = chain.front();
	//	if(packet.error == "") {
	//		std::cout << std::hex << packet.address << " " << packet.code.to_string() << std::endl;
	//		chain.erase(chain.begin());
	//	} else {
	//		std::cout << std::hex << packet.address << " " << packet.error << std::endl;
	//		if(packet.address > end_offset) {
	//			break;
	//		} else {
	//			chain = parse_vif_chain(&src, packet.address, SIZE_MAX);
	//			continue;
	//		}
	//	}
	//}
}
