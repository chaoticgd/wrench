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

#ifndef SPANNER_ASSET_PACKER_H
#define SPANNER_ASSET_PACKER_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>

enum class AssetFormatHint {
	NO_HINT,
	TEXTURE_PIF_IDTEX8,
	TEXTURE_RGBA
};

// Packs asset into a binary and writes it out to dest, using hint to determine
// details of the expected output format if necessary.
void pack_asset_impl(OutputStream& dest, Asset& asset, Game game, AssetFormatHint hint = AssetFormatHint::NO_HINT);

template <typename Range>
Range pack_asset(OutputStream& dest, Asset& wad, Game game, s64 base, AssetFormatHint hint = AssetFormatHint::NO_HINT) {
	s64 begin = dest.tell();
	pack_asset_impl(dest, wad, game, hint);
	s64 end = dest.tell();
	return Range::from_bytes(begin - base, end - begin);
}

#endif
