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

#ifndef ASSETMGR_ASSET_PATH_GEN_H
#define ASSETMGR_ASSET_PATH_GEN_H

#include <assetmgr/asset_types.h>

// Generate output paths for different types of asset based on the name and
// category attributes pulled from the underlay and possibly mods.
std::string generate_level_asset_path(s32 tag, const Asset& parent);
std::string generate_moby_class_asset_path(s32 tag, const Asset& parent);
std::string generate_tie_class_asset_path(s32 tag, const Asset& parent);
std::string generate_shrub_class_asset_path(s32 tag, const Asset& parent);

#endif
