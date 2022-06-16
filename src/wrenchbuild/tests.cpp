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
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

enum TestMode {
	RUN_ALL_TESTS,
	PRINT_DIFF_ON_FAIL
};

static void run_round_trip_asset_packing_tests(const fs::path& input_path, const std::string& asset_ref, s32 min_percentage, s32 max_percentage);
static void enumerate_binaries(std::vector<BinaryAsset*>& dest, Asset& src);
static void run_round_trip_asset_packing_test(AssetForest& forest, BinaryAsset& binary, AssetType type, s32 percentage, TestMode mode);
static void strip_trailing_padding_from_src(std::vector<u8>& src, std::vector<u8>& dest);

static s32 pass_count = 0;
static s32 fail_count = 0;

void run_tests(fs::path input_path, const std::string& asset_ref) {
	run_round_trip_asset_packing_tests(input_path, asset_ref, 0, 100);
	
	if(fail_count == 0) {
		printf("\nALL TESTS HAPPY\n");
	} else {
		printf("%d passed, %d failed", pass_count, fail_count);
	}
}

static void run_round_trip_asset_packing_tests(const fs::path& input_path, const std::string& asset_ref, s32 min_percentage, s32 max_percentage) {
	// Disable printing when an asset is packed/unpacked.
	g_asset_unpacker.quiet = true;
	g_asset_packer_quiet = true;
	
	AssetForest forest;
	AssetBank& bank = forest.mount<LooseAssetBank>(input_path, false);
	Asset* root = bank.root();
	verify(root, "Tried to run test on directory with no asset files!");
	
	std::vector<BinaryAsset*> binaries;
	
	if(asset_ref.empty()) {
		enumerate_binaries(binaries, *root);
	} else {
		AssetReference ref = parse_asset_reference(asset_ref.c_str());
		Asset& asset = forest.lookup_asset(ref, bank.root());
		verify(asset.type() == BinaryAsset::ASSET_TYPE, "Specified asset is not a binary.");
		binaries.emplace_back(&asset.as<BinaryAsset>());
	}
	
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
			TestMode mode = asset_ref.empty() ? RUN_ALL_TESTS : PRINT_DIFF_ON_FAIL;
			run_round_trip_asset_packing_test(forest, binary, type, (s32) percentage, mode);
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

static void run_round_trip_asset_packing_test(AssetForest& forest, BinaryAsset& binary, AssetType type, s32 percentage, TestMode mode) {
	auto src_file = binary.file().open_binary_file_for_reading(binary.src());
	std::vector<u8> src = src_file->read_multiple<u8>(src_file->size());
	MemoryInputStream src_stream(src);
	
	const char* type_name = asset_type_to_string(type);
	std::string ref = asset_reference_to_string(binary.reference());
	
	if(mode == PRINT_DIFF_ON_FAIL) {
		printf("[%3d%%] \033[34mRunning test with %s asset %s\033[0m\n", percentage, type_name, ref.c_str());
	}
	
	std::string hint = binary.format_hint();
	BuildConfig config(binary.game(), binary.region());
	
	AssetDispatchTable* dispatch = nullptr;
	
	AssetBank* temp = nullptr;
	
	std::vector<u8> dest;
	if(type == MobyClassAsset::ASSET_TYPE) {
		MobyClassData moby = read_moby_class(src, config.game());
		write_moby_class(dest, moby, config.game());
		
		dispatch = &MobyClassAsset::funcs;
	} else {
		temp = &forest.mount<MemoryAssetBank>();
		AssetFile& file = temp->asset_file("test.asset");
		Asset& asset = file.root().physical_child(type, "test");
		unpack_asset_impl(asset, src_stream, nullptr, config, hint.c_str());
		
		MemoryOutputStream dest_stream(dest);
		pack_asset_impl(dest_stream, nullptr, nullptr, asset, config, hint.c_str());
		
		dispatch = &asset.funcs;
	}
	
	if(!dispatch->test || !(*dispatch->test)(src, dest, config, hint.c_str())) {
		strip_trailing_padding_from_src(src, dest);
		if(diff_buffers(src, dest, 0, 0, mode == PRINT_DIFF_ON_FAIL)) {
			if(mode == RUN_ALL_TESTS) {
				printf("\033[32m [PASS] %s\033[0m\n", ref.c_str());
			}
			pass_count++;
		} else {
			if(mode == RUN_ALL_TESTS) {
				printf("\033[31m [FAIL] %s\033[0m\n", ref.c_str());
			}
			fail_count++;
		}
	}
	
	if(temp) {
		forest.unmount_last();
	}
}

static void strip_trailing_padding_from_src(std::vector<u8>& src, std::vector<u8>& dest) {
	if(dest.size() > 0 && src.size() > dest.size() && src.size() <= dest.size() + SECTOR_SIZE) {
		bool is_padding = true;
		for(s64 i = dest.size(); i < src.size(); i++) {
			is_padding &= src[i] == '\0';
		}
		if(is_padding) {
			src.resize(dest.size());
		}
	}
}
