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

#include "util.h"

#include <sstream>
#include <stdio.h>

std::string int_to_hex(std::size_t x) {
	std::stringstream ss;
	ss << std::hex << x;
	return ss.str();
}

std::size_t hex_to_int(std::string x) {
	std::stringstream ss;
	ss << std::hex << x;
	std::size_t result;
	ss >> result;
	return result;
}

std::size_t parse_number(std::string x) {
	std::stringstream ss;
	if(x.size() >= 2 && x[0] == '0' && x[1] == 'x') {
		ss << std::hex << x.substr(2);
	} else {
		ss << x;
	}
	std::size_t result;
	ss >> result;
	return result;
}

std::vector<std::string> to_hex_dump(uint32_t* data, std::size_t align, std::size_t size_in_u32s) {
	std::vector<std::string> result;
	std::size_t column = align % 16;
	std::string data_str(column * 3, ' ');
	for(std::size_t i = 0; i < size_in_u32s; i++) {
		uint32_t word = data[i];
		for(int byte = 0; byte < 4; byte++) {
			std::string num = int_to_hex(((uint8_t*) &word)[byte]);
			while(num.size() < 2) num = "0" + num;
			data_str += num + " ";
			if(data_str.size() > 47) {
				result.push_back(data_str);
				data_str = "";
			}
		}
	}
	if(data_str.size() > 0) {
		result.push_back(data_str);
	}
	return result;
}

int execute_command(std::string executable, std::vector<std::string> arguments) {
#ifdef DECENT_OS
	//
#else
	printf("*** USING INSECURE execute_command IMPLEMENTATION ***\n");
	
	std::string command = executable;
	for(std::string& arg : arguments) {
		command += " \"" + arg + "\"";
	}
	printf("command: %s\n", command.c_str());
	return system(command.c_str());
#endif
}
