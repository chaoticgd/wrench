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

#include <sstream>
#include <iostream>
#include <boost/filesystem.hpp>

#include "../command_line.h"
#include "../formats/racpak.h"

# /*
#	CLI tool to inspect, unpack and repack .WAD archives (racpaks).
# */

std::string hex_string(uint32_t x);

int main(int argc, char** argv) {
	std::string command;
	std::string src_path;
	std::string dest_path;

	po::options_description desc("Read a game archive file");
	desc.add_options()
		("command,c", po::value<std::string>(&command)->required(),
			"The operation to perform. Available commands are: ls, extract.")
		("src,s", po::value<std::string>(&src_path)->required(),
			"The input file of directory.")
		("dest,d", po::value<std::string>(&dest_path),
			"The output file or directory (if applicable).");

	po::positional_options_description pd;
	pd.add("command", 1);
	pd.add("src", 1);
	pd.add("dest", 1);

	if(!parse_command_line_args(argc, argv, desc, pd)) {
		return 0;
	}
	
	file_stream src_file(src_path);
	racpak archive(&src_file);
	uint32_t num_entries = archive.num_entries();

	if(command == "ls") {
		std::cout << "Index\tOffset\tSize\n";
		for(uint32_t i = 0; i < num_entries; i++) {
			auto entry = archive.entry(i);
			std::cout << std::dec;
			std::cout << i << "\t";
			std::cout << std::hex;
			std::cout << entry.offset << "\t";
			std::cout << entry.size << "\n";
		}
	} else if(command == "extract") {
		if(dest_path == "") {
			std::cerr << "Must specify destination.\n";
			return 0;
		}
		for(uint32_t i = 0; i < num_entries; i++) {
			boost::filesystem::create_directory(dest_path);
			
			auto entry = archive.entry(i);
			std::string dest_name = std::to_string(i) + "_" + hex_string(entry.offset);
			file_stream dest(dest_path + "/" + dest_name, std::ios::in | std::ios::out | std::ios::trunc);
			
			stream* src = archive.open(entry);
			src->seek(0);
			stream::copy_n(dest, *src, src->size());
		}
	} else {
		std::cerr << "Invalid command.\n";
	}
}

std::string hex_string(uint32_t x) {
	std::stringstream ss;
	ss << std::hex << x;
	return ss.str();
}
