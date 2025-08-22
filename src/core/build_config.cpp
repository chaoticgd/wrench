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

#include "build_config.h"

BuildConfig::BuildConfig(Game game, Region region, bool is_testing)
	: m_game(game), m_region(region), m_is_testing(is_testing) {}

BuildConfig::BuildConfig(const std::string& game, const std::string& region, bool is_testing)
	: BuildConfig(game_from_string(game), region_from_string(region), is_testing) {}

Game BuildConfig::game() const
{
	return m_game;
}

Region BuildConfig::region() const
{
	return m_region;
}

bool BuildConfig::is_testing() const
{
	return m_is_testing;
}

bool BuildConfig::is_ntsc() const
{
	return region() != Region::EU;
}

float BuildConfig::framerate()
{
	if (is_ntsc()) {
		return NTSC_FRAMERATE;
	} else {
		return PAL_FRAMERATE;
	}
}

float BuildConfig::half_framerate()
{
	if (is_ntsc()) {
		return HALF_NTSC_FRAMERATE;
	} else {
		return HALF_PAL_FRAMERATE;
	}
}

Game game_from_string(const std::string& game)
{
	if (game == "rac") return Game::RAC;
	if (game == "gc") return Game::GC;
	if (game == "uya") return Game::UYA;
	if (game == "dl") return Game::DL;
	return Game::UNKNOWN;
}

std::string game_to_string(Game game)
{
	switch (game) {
		case Game::RAC: return "rac";
		case Game::GC: return "gc";
		case Game::UYA: return "uya";
		case Game::DL: return "dl";
		default: return "unknown";
	}
}

Region region_from_string(const std::string& region)
{
	if (region == "us") return Region::US;
	if (region == "eu") return Region::EU;
	if (region == "japan") return Region::JAPAN;
	return Region::UNKNOWN;
}

std::string region_to_string(Region region)
{
	switch (region) {
		case Region::US: return "us";
		case Region::EU: return "eu";
		case Region::JAPAN: return "japan";
		default: return "unknown";
	}
}
