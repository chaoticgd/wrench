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

#ifndef WRENCHGUI_BUILD_SETTINGS_H
#define WRENCHGUI_BUILD_SETTINGS_H

#include <core/util.h>
#include <gui/commands.h>

namespace gui {

void build_settings(
	PackerParams& params,
	const std::vector<std::string>* game_builds,
	const std::vector<std::string>& mod_builds,
	bool launcher = true);

}

#endif
