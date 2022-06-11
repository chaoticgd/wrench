/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#ifndef ASSETMGR_GAME_INFO_H
#define ASSETMGR_GAME_INFO_H

#include <core/util.h>

enum class AssetBankType {
	UNPACKED, // Assets unpacked from a base game ISO.
	MOD, // A mod.
	TEST // A set of binary assets to be used for running tests on.
};

struct GameInfo {
	std::string name;
	std::string author;
	std::string description;
	std::string version;
	std::vector<std::string> images;
	s32 format_version;
	AssetBankType type;
	std::vector<std::string> builds; // List of builds included with this asset pack.
};

GameInfo read_game_info(char* input);
void write_game_info(std::string& dest, const GameInfo& info);

#endif
