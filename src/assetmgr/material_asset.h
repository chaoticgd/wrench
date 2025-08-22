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

#ifndef WRENCHBUILD_MATERIAL_ASSET_H
#define WRENCHBUILD_MATERIAL_ASSET_H

#include <core/material.h>
#include <assetmgr/asset_types.h>

struct MaterialSet
{
	std::vector<Material> materials;
	std::vector<FileReference> textures;
};

// Reads a collection of material assets and deduplicates textures referenced
// multiple times in the set of input materials.
MaterialSet read_material_assets(const CollectionAsset& src);

#endif
