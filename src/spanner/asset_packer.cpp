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

#include "asset_packer.h"

void pack_asset_impl(OutputStream& dest, Asset& asset, Game game, AssetFormatHint hint) {
	switch(asset.type().id) {
		case BinaryAsset::ASSET_TYPE.id: {
			BinaryAsset& binary = static_cast<BinaryAsset&>(asset);
			auto src = binary.file().open_binary_file_for_reading(binary.src());
			Stream::copy(dest, *src, src->size());
			break;
		}
	}
}
