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

#include "game_db.h"

#include <cctype>
#include <fstream>
#include <stdexcept>

#include "util.h"

std::string read_token(std::string str, std::size_t& i);
std::string read_rest_of_line(std::string str, std::size_t& i);
void skip_whitespace(std::string str, std::size_t& i);

std::map<std::string, gamedb_release> gamedb_parse_file() {
	std::ifstream file("data/gamedb.txt");
	std::map<std::string, gamedb_release> result;
	
	gamedb_release game;
	std::string line;
	while(std::getline(file, line)) {
		std::size_t i = 0;
		std::string type = read_token(line, i);
		if(type == "" || type[0] == '#') {
			continue;
		}
		
		if(type == "game") {
			game.elf_id = read_token(line, i);
		} else if(type == "end") {
			result.insert({ game.elf_id, game });
			game = gamedb_release();
		} else if(type == "title") {
			game.title = read_rest_of_line(line, i);
		} else if(type == "edition") {
			game.edition = gamedb_edition::_from_string(read_token(line, i).c_str());
		} else if(type == "region") {
			game.region = gamedb_region::_from_string(read_token(line, i).c_str());
		} else if(type == "file") {
			gamedb_file file(gamedb_file_type::_from_string(read_token(line, i).c_str()));
			file.offset = parse_number(read_token(line, i));
			file.size = parse_number(read_token(line, i));
			file.name = read_rest_of_line(line, i);
			game.files.push_back(file);
		} else {
			throw std::runtime_error("Error parsing gamedb: Invalid line type.");
		}
	}
	
	return result;
}

std::string read_token(std::string str, std::size_t& i) {
	skip_whitespace(str, i);
	std::size_t begin = i;
	for(; !isspace(str[i]) && str[i] != '\0'; i++);
	return str.substr(begin, i - begin);
}

std::string read_rest_of_line(std::string str, std::size_t& i) {
	skip_whitespace(str, i);
	return str.substr(i);
}

void skip_whitespace(std::string str, std::size_t& i) {
	for(; isspace(str[i]) && str[i] != '\0'; i++);
}
