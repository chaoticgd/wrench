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

#ifndef GAME_DB_H
#define GAME_DB_H

#include <map>
#include <string>
#include <vector>

#include "better-enums/enum.h"

# /*
# 	Lookup table of different game releases loaded from gamedb.txt.
# */

BETTER_ENUM(gamedb_region, char,
	EUROPE, NORTH_AMERICA, JAPAN
)

BETTER_ENUM(gamedb_edition, char,
	BLACK_LABEL, GREATEST_HITS	
)

BETTER_ENUM(gamedb_file_type, char,
	TEXTURES, ARMOR, LEVEL
)

struct gamedb_file {
	gamedb_file(gamedb_file_type type_) : type(type_) {}
	
	gamedb_file_type type;
	std::size_t offset;
	std::size_t size;
	std::string name;
};

struct gamedb_release {
	std::string elf_id; // e.g. "SCES_516.07".
	std::string title;
	gamedb_edition edition = gamedb_edition::BLACK_LABEL; // obviously
	gamedb_region region = gamedb_region::EUROPE;
	std::vector<gamedb_file> files;
};

std::map<std::string, gamedb_release> gamedb_parse_file();

#endif
