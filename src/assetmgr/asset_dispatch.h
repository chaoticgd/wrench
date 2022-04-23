/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#ifndef ASSETMGR_ASSET_DISPATCH_H
#define ASSETMGR_ASSET_DISPATCH_H

#include <assetmgr/asset_util.h>

class Asset;

enum AssetFormatHint {
	FMT_NO_HINT = 0,
	FMT_TEXTURE_PIF_IDTEX8,
	FMT_TEXTURE_RGBA
};

// *****************************************************************************

using AssetUnpackerFunc = std::function<void(Asset& dest, InputStream& src, Game game, AssetFormatHint hint)>;

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_unpacker_func(UnpackerFunc func) {
	return new AssetUnpackerFunc([func](Asset& dest, InputStream& src, Game game, AssetFormatHint hint) {
		func(static_cast<ThisAsset&>(dest), src, game);
	});
}

// *****************************************************************************

using AssetTransformerFunc = std::function<void(Asset& dest, Asset& src, Game game)>;

// *****************************************************************************

using AssetPackerFunc = std::function<void((OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, AssetFormatHint hint))>;

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, AssetFormatHint hint) {
		func(dest, static_cast<ThisAsset&>(src), game);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_wad_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, AssetFormatHint hint) {
		func(dest, header_dest, static_cast<ThisAsset&>(src), game);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_bin_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, AssetFormatHint hint) {
		func(dest, header_dest, time_dest, static_cast<ThisAsset&>(src));
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_iso_packer_func(PackerFunc func, AssetPackerFunc pack) {
	return new AssetPackerFunc([func, pack](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, AssetFormatHint hint) {
		func(dest, static_cast<ThisAsset&>(src), game, pack);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

// *****************************************************************************

struct AssetDispatchTable {
	AssetUnpackerFunc* unpack_rac1;
	AssetUnpackerFunc* unpack_rac2;
	AssetUnpackerFunc* unpack_rac3;
	AssetUnpackerFunc* unpack_dl;
	
	AssetTransformerFunc* transform_rac1;
	AssetTransformerFunc* transform_rac2;
	AssetTransformerFunc* transform_rac3;
	AssetTransformerFunc* transform_dl;
	
	AssetPackerFunc* pack_rac1;
	AssetPackerFunc* pack_rac2;
	AssetPackerFunc* pack_rac3;
	AssetPackerFunc* pack_dl;
};

#endif
