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

#ifndef WAD_GAMEPLAY_H
#define WAD_GAMEPLAY_H

#include "../core/util.h"
#include "../core/level.h"

using GameplayBlockReadFunc = std::function<void(LevelWad& wad, Gameplay& gameplay, Buffer src, Game game)>;
using GameplayBlockWriteFunc = std::function<bool(OutBuffer dest, const LevelWad& wad, const Gameplay& gameplay, Game game)>;

struct GameplayBlockFuncs {
	GameplayBlockReadFunc read;
	GameplayBlockWriteFunc write;
};

struct GameplayBlockDescription {
	s32 header_pointer_offset;
	GameplayBlockFuncs funcs;
	const char* name;
};

extern const std::vector<GameplayBlockDescription> RAC1_GAMEPLAY_BLOCKS;
extern const std::vector<GameplayBlockDescription> RAC23_GAMEPLAY_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS;

void read_gameplay(LevelWad& wad, Gameplay& gameplay, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks);
std::vector<u8> write_gameplay(const LevelWad& wad, const Gameplay& gameplay_arg, Game game, const std::vector<GameplayBlockDescription>& blocks);
std::vector<u8> write_occlusion(const Gameplay& gameplay, Game game);

#endif
