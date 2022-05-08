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

#include "tests.h"

#include <md5.h>

#include <engine/moby.h>
#include <engine/texture.h>
#include <engine/collision.h>
#include <pakrac/assets.h>
#include <pakrac/primary.h>
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static void run_round_trip_asset_packing_tests(const fs::path& input_path, s32 min_percentage, s32 max_percentage);
static void enumerate_binaries(std::vector<BinaryAsset*>& dest, Asset& src);
static void run_round_trip_asset_packing_test(AssetForest& forest, BinaryAsset& binary, AssetType type, s32 percentage);

void run_tests(fs::path input_path) {
	run_round_trip_asset_packing_tests(input_path, 0, 100);
	
	printf("\nALL TESTS HAPPY\n");
}

static void run_round_trip_asset_packing_tests(const fs::path& input_path, s32 min_percentage, s32 max_percentage) {
	AssetForest forest;
	AssetBank& bank = forest.mount<LooseAssetBank>("test", input_path, false);
	Asset* root = bank.root();
	verify(root, "Tried to run test on directory with no asset files!");
	
	std::vector<BinaryAsset*> binaries;
	enumerate_binaries(binaries, *root);
	
	for(size_t i = 0; i < binaries.size(); i++) {
		BinaryAsset& binary = *binaries[i];
		std::string asset_type;
		try {
			asset_type = binary.asset_type();
		} catch(MissingAssetAttribute&) {
			continue;
		}
		AssetType type = asset_string_to_type(asset_type.c_str());
		if(type != NULL_ASSET_TYPE) {
			f32 percentage = lerp(min_percentage, max_percentage, i / (f32) binaries.size());
			run_round_trip_asset_packing_test(forest, binary, type, (s32) percentage);
		}
	}
}

static void enumerate_binaries(std::vector<BinaryAsset*>& dest, Asset& src) {
	if(src.type() == BinaryAsset::ASSET_TYPE) {
		dest.emplace_back(&src.as<BinaryAsset>());
	}
	
	src.for_each_logical_child([&](Asset& child) {
		enumerate_binaries(dest, child);
	});
}

static void run_round_trip_asset_packing_test(AssetForest& forest, BinaryAsset& binary, AssetType type, s32 percentage) {
	auto src_file = binary.file().open_binary_file_for_reading(binary.src());
	std::vector<u8> src = src_file->read_multiple<u8>(src_file->size());
	MemoryInputStream src_stream(src);
	
	const char* type_name = asset_type_to_string(type);
	std::string ref = asset_reference_to_string(binary.reference());
	printf("[%3d%%] \033[34mRunning test with %s asset %s\033[0m\n", percentage, type_name, ref.c_str());
	
	AssetFormatHint hint = (AssetFormatHint) binary.format_hint();
	Game game = (Game) binary.game();
	
	AssetDispatchTable* dispatch = nullptr;
	
	std::vector<u8> dest;
	if(type == MobyClassAsset::ASSET_TYPE) {
		MobyClassData moby = read_moby_class(src, game);
		write_moby_class(dest, moby, game);
		
		dispatch = &MobyClassAsset::funcs;
	} else {
		AssetBank& temp = forest.mount<MemoryAssetBank>("test");
		AssetFile& file = temp.asset_file("test.asset");
		Asset& asset = file.root().physical_child(type, "test");
		unpack_asset_impl(asset, src_stream, game, hint);
		
		MemoryOutputStream dest_stream(dest);
		pack_asset_impl(dest_stream, nullptr, nullptr, asset, game, hint);
		
		dispatch = &asset.funcs;
	}
	
	if(!dispatch->test || !(*dispatch->test)(src, dest, game, hint)) {
		if(!diff_buffers(src, dest, 0, "", 0)) {
			exit(1);
		}
	}
}
