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

#ifndef WRENCHBUILD_TESTS_H
#define WRENCHBUILD_TESTS_H

#include <core/filesystem.h>
#include <assetmgr/asset.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

void run_tests(fs::path input_path, const std::string& asset_ref, const std::string& filter);
void strip_trailing_padding_from_lhs(
	std::vector<u8>& lhs, std::vector<u8>& rhs, s32 max_padding_size = SECTOR_SIZE);

template <typename TestFunc>
AssetTestFunc* wrap_diff_test_func(TestFunc func)
{
	return new AssetTestFunc([func](std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode) -> bool {
		MemoryInputStream src_stream(src);
		
		AssetForest forest;
		AssetBank& temp = forest.mount<MemoryAssetBank>();
		AssetFile& file = temp.asset_file("test.asset");
		Asset& asset = file.root().physical_child(type, "test");
		unpack_asset_impl(asset, src_stream, nullptr, config, hint);
		
		std::vector<u8> dest;
		MemoryOutputStream dest_stream(dest);
		pack_asset_impl(dest_stream, nullptr, nullptr, asset, config, hint);
		
		strip_trailing_padding_from_lhs(src, dest);
		
		return func(src, dest, config, hint, mode);
	});
}

#endif
