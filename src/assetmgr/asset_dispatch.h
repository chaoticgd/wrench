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

#include <core/build_config.h>
#include <assetmgr/asset_util.h>

class Asset;

// Common hint strings to be passed to the asset packers/unpackers.
#define FMT_NO_HINT ""
#define FMT_BINARY_WAD "ext,wad"
#define FMT_BINARY_PSS "ext,pss"
#define FMT_BINARY_VAG "ext,vag"
#define FMT_BUILD_RELEASE "release"
#define FMT_BUILD_TESTLF_ALL_LEVELS_MPEGS "testlf,,"
#define FMT_BUILD_TESTLF_ALL_LEVELS_NOMPEGS "testlf,,nompegs"
#define FMT_TEXTURE_RGBA "rgba"
#define FMT_TEXTURE_RGBA_512_416 "rawrgba,512,416"
#define FMT_TEXTURE_RGBA_512_448 "rawrgba,512,448"
#define FMT_TEXTURE_PIF4 "pif,4,unswizzled"
#define FMT_TEXTURE_PIF4_SWIZZLED "pif,4,1,swizzled"
#define FMT_TEXTURE_PIF8 "pif,8,unswizzled"
#define FMT_TEXTURE_PIF8_SWIZZLED "pif,8,1,swizzled"
#define FMT_MOBY_CLASS_PHAT "phat"
#define FMT_MOBY_CLASS_MESH_ONLY_ARMOR "gadget"
#define FMT_MOBY_CLASS_MESH_ONLY_WRENCH "mission"
#define FMT_COLLECTION_PIF8 "texlist,pif,8,1,unswizzled"
#define FMT_COLLECTION_PIF8_4MIPS "texlist,pif,8,4,unswizzled"
#define FMT_COLLECTION_MATLIST_PIF8 "matlist,pif,8,1,unswizzled"
#define FMT_COLLECTION_MATLIST_PIF8_4MIPS "matlist,pif,8,4,unswizzled"
#define FMT_COLLECTION_SUBTITLES "subtitles"
#define FMT_COLLECTION_MISSION_CLASSES "missionclasses"
#define FMT_GLOBALWAD_NOMPEGS "nompegs"
#define FMT_MPEGWAD_NOMPEGS "nompegs"
#define FMT_ELFFILE_PACKED "packed"
#define FMT_ELFFILE_RATCHET_EXECUTABLE "ratchetexecutable"
#define FMT_INSTANCES_GAMEPLAY "gameplay"
#define FMT_INSTANCES_ART "art"
#define FMT_INSTANCES_MISSION "mission"

// *****************************************************************************

using AssetUnpackerFunc = std::function<void(Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint)>;

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_unpacker_func(UnpackerFunc func)
{
	return new AssetUnpackerFunc([func](Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint) {
		func(static_cast<ThisAsset&>(dest), src, config);
	});
}

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_hint_unpacker_func(UnpackerFunc func)
{
	return new AssetUnpackerFunc([func](Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint) {
		func(static_cast<ThisAsset&>(dest), src, config, hint);
	});
}

template <typename ThisAsset, typename WadHeader, typename UnpackerFunc>
AssetUnpackerFunc* wrap_wad_unpacker_func(UnpackerFunc func, bool error_fatal = true)
{
	return new AssetUnpackerFunc([func, error_fatal](Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint) {
		verify(header_src, "No header passed to wad unpacker.");
		if(!error_fatal && Buffer(*header_src).read<s32>(0, "wad header") != sizeof(WadHeader)) {
			return;
		}
		WadHeader header = Buffer(*header_src).read<WadHeader>(0, "wad header");
		func(static_cast<ThisAsset&>(dest), header, src, config);
	});
}

template <typename ThisAsset, typename UnpackerFunc>
AssetUnpackerFunc* wrap_iso_unpacker_func(UnpackerFunc func, AssetUnpackerFunc unpack)
{
	return new AssetUnpackerFunc([func, unpack](Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint) {
		func(static_cast<ThisAsset&>(dest), src, config, unpack);
	});
}

// *****************************************************************************

using AssetPackerFunc = std::function<void(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint)>;

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_packer_func(PackerFunc func)
{
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint) {
		func(dest, static_cast<const ThisAsset&>(src), config);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_hint_packer_func(PackerFunc func)
{
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint) {
		func(dest, static_cast<const ThisAsset&>(src), config, hint);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename WadHeader, typename PackerFunc>
AssetPackerFunc* wrap_wad_packer_func(PackerFunc func)
{
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint) {
		WadHeader header = {0};
		header.header_size = sizeof(WadHeader);
		dest.write(header);
		dest.pad(SECTOR_SIZE, 0);
		func(dest, header, static_cast<const ThisAsset&>(src), config);
		dest.write(0, header);
		if(header_dest) {
			OutBuffer(*header_dest).write(0, header);
		}
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

template <typename ThisAsset, typename WadHeader, typename PackerFunc>
AssetPackerFunc* wrap_wad_hint_packer_func(PackerFunc func)
{
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint) {
		WadHeader header = {0};
		header.header_size = sizeof(WadHeader);
		dest.write(header);
		dest.pad(SECTOR_SIZE, 0);
		func(dest, header, static_cast<const ThisAsset&>(src), config, hint);
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
AssetPackerFunc* wrap_bin_packer_func(PackerFunc func)
{
	return new AssetPackerFunc([func](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint) {
		func(dest, header_dest, time_dest, static_cast<const ThisAsset&>(src));
	});
}

template <typename ThisAsset, typename PackerFunc>
AssetPackerFunc* wrap_iso_packer_func(PackerFunc func, AssetPackerFunc pack)
{
	return new AssetPackerFunc([func, pack](OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint) {
		func(dest, static_cast<const ThisAsset&>(src), config, hint, pack);
		if(time_dest) {
			*time_dest = fs::file_time_type::clock::now();
		}
	});
}

// *****************************************************************************

enum class AssetTestMode
{
	RUN_ALL_TESTS,
	PRINT_DIFF_ON_FAIL
};

using AssetTestFunc = std::function<bool(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode)>;

// *****************************************************************************

struct AssetDispatchTable
{
	AssetUnpackerFunc* unpack_rac1;
	AssetUnpackerFunc* unpack_rac2;
	AssetUnpackerFunc* unpack_rac3;
	AssetUnpackerFunc* unpack_dl;
	
	AssetPackerFunc* pack_rac1;
	AssetPackerFunc* pack_rac2;
	AssetPackerFunc* pack_rac3;
	AssetPackerFunc* pack_dl;
	
	AssetTestFunc* test_rac;
	AssetTestFunc* test_gc;
	AssetTestFunc* test_uya;
	AssetTestFunc* test_dl;
};

#endif
