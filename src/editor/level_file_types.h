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

#ifndef LEVEL_FILE_TYPES_H
#define LEVEL_FILE_TYPES_H

#include <map>
#include <stdint.h>

enum class level_file_type {
	LEVEL, AUDIO, SCENE
};

// Can represent states where we don't yet know exactly which game we're working
// with e.g. GAME_RAC2 | GAME_RAC3 = could be a R&C2 file or R&C3 file.
// The idea here is that if we bitwise and these together for a set of files,
// we'll either get the game we want, or we'll know something is amiss.
enum which_game {
	GAME_RAC1 = 1,
	GAME_RAC2 = 2,
	GAME_RAC3 = 4,
	GAME_RAC4 = 8,
	GAME_RAC2_OTHER = 16,
	GAME_ANY = GAME_RAC1 | GAME_RAC2 | GAME_RAC3 | GAME_RAC4 | GAME_RAC2_OTHER
};

// R&C2 and R&C3 levels have the same magic identifier, but the level table
struct level_file_identifier {
	uint32_t magic;
	int level_table_position;
};

struct level_file_info {
	level_file_type type;
	const char* prefix;
	which_game game;
};

// A table of the different types of file headers that level-specific files
// will have depending on the game. The key of the map is the magic identifier
// stored at 0x0 in these files.
static const std::map<uint32_t, level_file_info> LEVEL_FILE_TYPES = {
	{0x0060, level_file_info {level_file_type::LEVEL, "level", (which_game) (GAME_RAC2 | GAME_RAC3)}},
	{0x1018, level_file_info {level_file_type::AUDIO, "audio", GAME_RAC2}},
	{0x137c, level_file_info {level_file_type::SCENE, "scene", GAME_RAC2}},
	
	{0x1818, level_file_info {level_file_type::AUDIO, "audio", GAME_RAC3}},
	{0x26f0, level_file_info {level_file_type::SCENE, "scene", (which_game) (GAME_RAC3 | GAME_RAC4)}},
	
	{0x0c68, level_file_info {level_file_type::LEVEL, "level", GAME_RAC4}},
	{0x02a0, level_file_info {level_file_type::AUDIO, "audio", GAME_RAC4}},
	
	{0x0068, level_file_info {level_file_type::LEVEL, "level", GAME_RAC2_OTHER}},
	{0x1000, level_file_info {level_file_type::AUDIO, "audio", GAME_RAC2_OTHER}},
	{0x2420, level_file_info {level_file_type::SCENE, "scene", GAME_RAC2_OTHER}}
};

#endif
