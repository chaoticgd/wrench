/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#ifndef WRENCHCORE_BUILD_CONFIG_H
#define WRENCHCORE_BUILD_CONFIG_H

#include <core/util.h>

enum class Game : u8
{
	UNKNOWN = 0, RAC = 1, GC = 2, UYA = 3, DL = 4
};

enum
{
	MAX_GAME = 5
};

enum class Region : u8
{
	UNKNOWN = 0, US = 1, EU = 2, JAPAN = 3
};

class BuildConfig
{
public:
	BuildConfig(Game game, Region region, bool is_testing = false);
	BuildConfig(const std::string& game, const std::string& region, bool is_testing = false);
	
	Game game() const;
	Region region() const;
	bool is_testing() const;
	
	bool is_ntsc() const;
	
	float framerate();
	float half_framerate();
	
private:
	Game m_game;
	Region m_region;
	bool m_is_testing;
};

Game game_from_string(const std::string& game);
std::string game_to_string(Game game);
Region region_from_string(const std::string& region);
std::string region_to_string(Region region);

#define NTSC_FRAMERATE 59.94005994005994f
#define PAL_FRAMERATE 50.f
#define HALF_NTSC_FRAMERATE 29.97002997002997f
#define HALF_PAL_FRAMERATE 25.f

#endif
