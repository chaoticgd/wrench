/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef WRENCHBUILD_LEVEL_INSTANCES_ASSET_H
#define WRENCHBUILD_LEVEL_INSTANCES_ASSET_H

#include <instancemgr/gameplay.h>
#include <assetmgr/asset_types.h>

std::pair<Gameplay, PvarTypes> load_instances(const Asset& src, const BuildConfig& config, const char* hint);

#endif
