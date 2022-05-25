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
	FMT_BINARY_WAD,
	FMT_BINARY_PSS,
	FMT_TEXTURE_RGBA,
	FMT_TEXTURE_PIF4,
	FMT_TEXTURE_PIF4_SWIZZLED,
	FMT_TEXTURE_PIF8,
	FMT_TEXTURE_PIF8_SWIZZLED,
	FMT_MOBY_CLASS_LEVEL,
	FMT_MOBY_CLASS_ARMOR,
	FMT_MOBY_CLASS_ARMOR_MP,
	FMT_COLLECTION_PIF8
};

// *****************************************************************************

using AssetUnpackerFunc = std::function<void(Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset)>;

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_unpacker_func(UnpackerFunc func) {
	return new AssetUnpackerFunc([func](Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset) {
		func(static_cast<ThisAsset&>(dest), src, game);
	});
}

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_hint_unpacker_func(UnpackerFunc func) {
	return new AssetUnpackerFunc([func](Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset) {
		func(static_cast<ThisAsset&>(dest), src, game, hint);
	});
}

template <typename ThisAsset, typename WadHeader, typename UnpackerFunc>
AssetUnpackerFunc* wrap_wad_unpacker_func(UnpackerFunc func) {
	return new AssetUnpackerFunc([func](Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset) {
		WadHeader header;
		if(game == Game::RAC1) {
			// The packed R&C1 headers don't have a header_size and sector field
			// but we want to read them using a version of the header that
			// starts with those fields so we can write them out like that, so
			// that all of the WAD files can be identified from their header.
			header = src.read<WadHeader>(header_offset - 8);
		} else {
			header = src.read<WadHeader>(header_offset);
		}
		func(static_cast<ThisAsset&>(dest).switch_files(), header, src, game);
	});
}

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_iso_unpacker_func(UnpackerFunc func, AssetUnpackerFunc unpack) {
	return new AssetUnpackerFunc([func, unpack](Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset) {
		func(static_cast<ThisAsset&>(dest), src, unpack);
	});
}

// *****************************************************************************

using AssetPackerFunc = std::function<void((OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, s32 hint))>;

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, s32 hint) {
		func(dest, static_cast<ThisAsset&>(src), game);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_hint_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, s32 hint) {
		func(dest, static_cast<ThisAsset&>(src), game, hint);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename WadHeader, typename PackerFunc>
AssetPackerFunc* wrap_wad_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, s32 hint) {
		WadHeader header = {0};
		header.header_size = sizeof(WadHeader);
		dest.write(header);
		dest.pad(SECTOR_SIZE, 0);
		func(dest, header, static_cast<ThisAsset&>(src), game);
		dest.write(0, header);
		if(header_dest) {
			OutBuffer(*header_dest).write(0, header);
		}
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_bin_packer_func(PackerFunc func) {
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, s32 hint) {
		func(dest, header_dest, time_dest, static_cast<ThisAsset&>(src));
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_iso_packer_func(PackerFunc func, AssetPackerFunc pack) {
	return new AssetPackerFunc([func, pack](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, s32 hint) {
		func(dest, static_cast<ThisAsset&>(src), game, pack);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

// *****************************************************************************

using AssetTestFunc = std::function<bool(std::vector<u8>& original, std::vector<u8>& repacked, Game game, s32 hint)>;

// *****************************************************************************

struct AssetDispatchTable {
	AssetUnpackerFunc* unpack_rac1;
	AssetUnpackerFunc* unpack_rac2;
	AssetUnpackerFunc* unpack_rac3;
	AssetUnpackerFunc* unpack_dl;
	
	AssetPackerFunc* pack_rac1;
	AssetPackerFunc* pack_rac2;
	AssetPackerFunc* pack_rac3;
	AssetPackerFunc* pack_dl;
	
	AssetTestFunc* test;
};

#endif
