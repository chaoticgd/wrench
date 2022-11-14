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

#include <assetmgr/material_asset.h>
#include <engine/shrub.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_tfrags(TfragsAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_tfrags(OutputStream& dest, const TfragsAsset& src, BuildConfig config, const char* hint);
static AssetTestResult test_tfrags(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(ShrubClass, []() {
	TfragsAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<TfragsAsset>(unpack_tfrags);
	TfragsAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<TfragsAsset>(unpack_tfrags);
	TfragsAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<TfragsAsset>(unpack_tfrags);
	TfragsAsset::funcs.unpack_dl = wrap_hint_unpacker_func<TfragsAsset>(unpack_tfrags);
	
	TfragsAsset::funcs.pack_rac1 = wrap_hint_packer_func<TfragsAsset>(pack_tfrags);
	TfragsAsset::funcs.pack_rac2 = wrap_hint_packer_func<TfragsAsset>(pack_tfrags);
	TfragsAsset::funcs.pack_rac3 = wrap_hint_packer_func<TfragsAsset>(pack_tfrags);
	TfragsAsset::funcs.pack_dl = wrap_hint_packer_func<TfragsAsset>(pack_tfrags);
	
	TfragsAsset::funcs.test_rac = new AssetTestFunc(test_tfrags);
	TfragsAsset::funcs.test_gc  = new AssetTestFunc(test_tfrags);
	TfragsAsset::funcs.test_uya = new AssetTestFunc(test_tfrags);
	TfragsAsset::funcs.test_dl  = new AssetTestFunc(test_tfrags);
})

static void unpack_tfrags(TfragsAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	if(g_asset_unpacker.dump_binaries) {
		if(!dest.has_core()) {
			unpack_asset_impl(dest.core<TfragsCoreAsset>(), src, nullptr, config);
		}
		return;
	}
}

static void pack_tfrags(OutputStream& dest, const TfragsAsset& src, BuildConfig config, const char* hint) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	if(src.get_core().logical_type() == BinaryAsset::ASSET_TYPE) {
		pack_asset_impl(dest, nullptr, nullptr, src.get_core(), config, nullptr);
		return;
	}
}

static AssetTestResult test_tfrags(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode) {
	return AssetTestResult::NOT_RUN;
}
