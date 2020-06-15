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

# /*
# 	List of supported games loaded from gamedb.txt.
# */

struct gamedb_game {
	std::string name;
	std::map<std::size_t, std::string> tables;
	std::map<std::size_t, std::string> levels;
};

std::vector<gamedb_game> gamedb_read();

#endif
