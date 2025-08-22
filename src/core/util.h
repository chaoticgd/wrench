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

#ifndef UTIL_H
#define UTIL_H

#include <core/util/basic_util.h>
#include <core/util/binary_util.h>
#include <core/util/error_util.h>

enum class WadType
{
	UNKNOWN = 0,
	GLOBAL, // R&C1
	ARMOR,
	AUDIO,
	BONUS,
	GADGET,
	HUD,
	MISC,
	MPEG,
	ONLINE,
	SCENE,
	SPACE,
	LEVEL,
	LEVEL_AUDIO,
	LEVEL_SCENE
};

#endif
