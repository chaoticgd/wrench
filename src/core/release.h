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

#ifndef CORE_RELEASE_H
#define CORE_RELEASE_H

#include <core/build_config.h>
#include <core/util.h>

struct Release
{
	std::string elf_name;
	Game game = Game::UNKNOWN;
	Region region = Region::US;
	const char* name;
};

Release identify_release(const std::string& elf_name);
Release identify_release_fuzzy(const std::string& game_id);

#endif
