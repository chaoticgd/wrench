/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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
#include "../formats/dma.h"

int main(int argc, char** argv) {
	std::string src_path;
	std::string offset_str;

	po::options_description desc("Parse PS2 VIF chains until an invalid VIF code is encountered");
	desc.add_options()
		("src,s",    po::value<std::string>(&src_path)->required(),
			"The input file.")
		("offset,o", po::value<std::string>(&offset_str)->default_value("0"),
			"The offset in the input file where the VIF chain begins.");

	po::positional_options_description pd;
	pd.add("src", 1);

	if(!parse_command_line_args(argc, argv, desc, pd)) {
		return 0;
	}
	
	std::size_t offset = parse_number(offset_str);

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
