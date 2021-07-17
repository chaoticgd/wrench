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
#include <sstream>
#include <functional>

#include "util.h"

struct gamedb_parser {
	std::vector<std::string> tokens;
	std::vector<bool> is_eol;
	std::size_t pos = 0;
};

gamedb_parser gamedb_lex(std::string input);
std::map<std::size_t, std::string> gamedb_read_subsection(gamedb_parser& parser);
std::string gamedb_read_token(gamedb_parser& parser);
std::string gamedb_read_until_newline(gamedb_parser& parser);

std::vector<gamedb_game> gamedb_read() {
	std::ifstream file("data/gamedb.txt");
	if(!file) {
		return {};
	}
	
	std::stringstream ss;
	ss << file.rdbuf();
	
	std::string content = ss.str();
	gamedb_parser parser = gamedb_lex(content);
	
	std::vector<gamedb_game> result;
	while(parser.pos < parser.tokens.size()) {
		gamedb_game game;
		if(gamedb_read_token(parser) != "game") {
			throw std::runtime_error("gamedb: Expected 'game'.");
		}
		if(gamedb_read_token(parser) != "{") {
			throw std::runtime_error("gamedb: Expected '{'.");
		}
		std::string next;
		while(next = gamedb_read_token(parser), next != "}") {
			if(next == "name") {
				game.name = gamedb_read_until_newline(parser);
			} else if(next == "tables") {
				game.tables = gamedb_read_subsection(parser);
			} else if(next == "levels") {
				game.levels = gamedb_read_subsection(parser);
			} else {
				throw std::runtime_error("gamedb: Expected 'name', 'tables', 'levels', or '}'.");
			}
		}
		result.push_back(game);
	}
	
	return result;
}

gamedb_parser gamedb_lex(std::string input) {
	gamedb_parser parser;
	std::string current;
	for(char c : input) {
		if(isspace(c)) {
			if(current.size() > 0) {
				parser.tokens.push_back(current);
				parser.is_eol.push_back(false);
				current = "";
			}
			if(c == '\n' && parser.is_eol.size() > 0) {
				parser.is_eol.back() = true;
			}
		} else {
			current += c;
		}
	}
	if(current.size() > 0) {
		parser.tokens.push_back(current);
		parser.is_eol.push_back(true);
	}
	return parser;
}

std::map<std::size_t, std::string> gamedb_read_subsection(gamedb_parser& parser) {
	if(gamedb_read_token(parser) != "{") {
		throw std::runtime_error("gamedb: Expected '{'.");
	}
	std::map<std::size_t, std::string> result;
	std::string next;
	while(next = gamedb_read_token(parser), next != "}") {
		std::size_t key = std::stoi(next);
		std::string value = gamedb_read_until_newline(parser);
		result.emplace(key, value);
	}
	return result;
}

std::string gamedb_read_token(gamedb_parser& parser) {
	if(parser.pos >= parser.tokens.size()) {
		throw std::runtime_error("gamedb: Unexpected end of file.");
	}
	return parser.tokens[parser.pos++];
}

std::string gamedb_read_until_newline(gamedb_parser& parser) {
	std::string result;
	while(parser.pos < parser.tokens.size() && !parser.is_eol[parser.pos]) {
		result += gamedb_read_token(parser) + " ";
	}
	result += gamedb_read_token(parser);
	return result;
}
