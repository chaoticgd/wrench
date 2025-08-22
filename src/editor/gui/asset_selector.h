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

#include <assetmgr/asset.h>

struct AssetSelector
{
	s32 required_type_count = 0;
	AssetType required_types[10];
	Opt<AssetType> omit_type;
	bool no_recurse = true;
	std::string filter;
	std::string preview;
	Asset* selected = nullptr;
};

Asset* asset_selector(
	const char* label, const char* default_preview, AssetSelector& state, AssetForest& forest);
