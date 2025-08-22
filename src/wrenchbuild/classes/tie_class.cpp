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

#include <engine/tie.h>
#include <assetmgr/material_asset.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_tie_class(TieClassAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_tie_class(OutputStream& dest, const TieClassAsset& src, BuildConfig config, const char* hint);
static bool test_tie_class(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(TieClass, []() {
	TieClassAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<TieClassAsset>(unpack_tie_class);
	TieClassAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<TieClassAsset>(unpack_tie_class);
	TieClassAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<TieClassAsset>(unpack_tie_class);
	TieClassAsset::funcs.unpack_dl = wrap_hint_unpacker_func<TieClassAsset>(unpack_tie_class);
	
	TieClassAsset::funcs.pack_rac1 = wrap_hint_packer_func<TieClassAsset>(pack_tie_class);
	TieClassAsset::funcs.pack_rac2 = wrap_hint_packer_func<TieClassAsset>(pack_tie_class);
	TieClassAsset::funcs.pack_rac3 = wrap_hint_packer_func<TieClassAsset>(pack_tie_class);
	TieClassAsset::funcs.pack_dl = wrap_hint_packer_func<TieClassAsset>(pack_tie_class);
	
	TieClassCoreAsset::funcs.test_rac = new AssetTestFunc(test_tie_class);
	TieClassCoreAsset::funcs.test_gc  = new AssetTestFunc(test_tie_class);
	TieClassCoreAsset::funcs.test_uya = new AssetTestFunc(test_tie_class);
	TieClassCoreAsset::funcs.test_dl  = new AssetTestFunc(test_tie_class);
})

static void unpack_tie_class(TieClassAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	if (g_asset_unpacker.dump_binaries) {
		if (!dest.has_core()) {
			unpack_asset_impl(dest.core<TieClassCoreAsset>(), src, nullptr, config);
		}
		return;
	}
	
	unpack_asset_impl(dest.core<BinaryAsset>(), src, nullptr, config);
	
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	TieClass tie = read_tie_class(buffer, config.game());
	ColladaScene scene = recover_tie_class(tie);
	
	std::vector<u8> xml = write_collada(scene);
	auto ref = dest.file().write_text_file("mesh.dae", (char*) xml.data());
	
	MeshAsset& editor_mesh = dest.editor_mesh();
	editor_mesh.set_name("mesh");
	editor_mesh.set_src(ref);
}

static void pack_tie_class(OutputStream& dest, const TieClassAsset& src, BuildConfig config, const char* hint) {
	if (g_asset_packer_dry_run) {
		return;
	}
	
	if (src.get_core().logical_type() == BinaryAsset::ASSET_TYPE) {
		pack_asset_impl(dest, nullptr, nullptr, src.get_core(), config, nullptr);
		return;
	} else {
		verify_not_reached_fatal("Not yet implemented.");
	}
}

static bool test_tie_class(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode) {
	TieClass tie = read_tie_class(src, config.game());
	
	std::vector<u8> dest;
	write_tie_class(dest, tie);
	
	return diff_buffers(src, dest, 0, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
}
