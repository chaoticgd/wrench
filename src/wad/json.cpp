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

#include "json.h"

static const char* HEX_DIGITS = "0123456789abcdef";

std::string encode_json_string(const std::string& input) {
	std::string output;
	for(char c : input) {
		output += HEX_DIGITS[(c & 0xff) >> 4];
		output += HEX_DIGITS[(c & 0xff) & 0xf];
	}
	return output;
}

std::string decode_json_string(const std::string& input) {
	std::string output;
	verify(input.size() % 2 == 0, "Invalid string.");
	for(size_t i = 0; i < input.size(); i += 2) {
		u32 c;
		sscanf(&input[i], "%02x", &c);
		output += (char) (c & 0xff);
	}
	return output;
}

Json buffer_to_json_hexdump(const std::vector<u8>& buffer) {
	Json json;
	for(size_t i = 0; i < buffer.size(); i += 0x10) {
		std::string line;
		for(size_t j = 0; j < 0x10 && i + j < buffer.size(); j++) {
			u8 byte = buffer[i + j];
			line += HEX_DIGITS[byte >> 4];
			line += HEX_DIGITS[byte & 0xf];
		}
		json.emplace_back(line);
	}
	return json;
}

std::vector<u8> buffer_from_json_hexdump(const Json& json) {
	verify(json.is_array(), "Expected JSON array for hexdump.");
	std::vector<u8> result;
	for(const Json& line : json) {
		verify(line.is_string(), "Expected JSON string.");
		std::string line_str = decode_json_string(line.get<std::string>());
		for(char c : line_str) {
			result.push_back(c);
		}
	}
	return result;
}
