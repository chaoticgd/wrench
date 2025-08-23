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

#include <string>
#include <md5.h>
#include <engine/moby_low.h>
#include <engine/shrub.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void run_round_trip_asset_packing_tests(
	const fs::path& input_path,
	const std::string& asset_ref,
	s32 min_percentage,
	s32 max_percentage,
	const std::string& filter);
static void enumerate_binaries(std::vector<BinaryAsset*>& dest, Asset& src);
static void run_round_trip_asset_packing_test(
	AssetForest& forest,
	BinaryAsset& binary,
	AssetType type,
	s32 percentage,
	AssetTestMode mode,
	const std::string& filter);

static s32 pass_count = 0;
static s32 fail_count = 0;

void run_tests(fs::path input_path, const std::string& asset_ref, const std::string& filter)
{
	run_round_trip_asset_packing_tests(input_path, asset_ref, 0, 100, filter);
	
	if (fail_count == 0) {
		printf("\nALL TESTS HAPPY\n");
	} else {
		printf("%d passed, %d failed\n", pass_count, fail_count);
	}
}

static void run_round_trip_asset_packing_tests(
	const fs::path& input_path,
	const std::string& asset_ref,
	s32 min_percentage,
	s32 max_percentage,
	const std::string& filter)
{
	// Disable printing when an asset is packed/unpacked.
	g_asset_unpacker.quiet = true;
	g_asset_packer_quiet = true;
	
	AssetForest forest;
	AssetBank& bank = forest.mount<LooseAssetBank>(input_path, false);
	Asset* root = bank.root();
	verify(root, "Tried to run test on directory with no asset files!");
	
	std::vector<BinaryAsset*> binaries;
	
	if (asset_ref.empty()) {
		enumerate_binaries(binaries, *root);
	} else {
		AssetLink link;
		link.set(asset_ref.c_str());
		Asset& asset = forest.lookup_asset(link, bank.root());
		verify(asset.physical_type() == BinaryAsset::ASSET_TYPE, "Specified asset is not a binary.");
		binaries.emplace_back(&asset.as<BinaryAsset>());
	}
	
	for (size_t i = 0; i < binaries.size(); i++) {
		BinaryAsset& binary = *binaries[i];
		std::string asset_type;
		try {
			asset_type = binary.asset_type();
		} catch (RuntimeError&) {
			continue;
		}
		AssetType type = asset_string_to_type(asset_type.c_str());
		if (type != NULL_ASSET_TYPE) {
			f32 percentage = lerp(min_percentage, max_percentage, i / (f32) binaries.size());
			AssetTestMode mode = asset_ref.empty() ? AssetTestMode::RUN_ALL_TESTS : AssetTestMode::PRINT_DIFF_ON_FAIL;
			run_round_trip_asset_packing_test(forest, binary, type, (s32) percentage, mode, filter);
		}
	}
}

static void enumerate_binaries(std::vector<BinaryAsset*>& dest, Asset& src)
{
	if (src.logical_type() == BinaryAsset::ASSET_TYPE) {
		dest.emplace_back(&src.as<BinaryAsset>());
	}
	
	src.for_each_logical_child([&](Asset& child) {
		enumerate_binaries(dest, child);
	});
}

static void run_round_trip_asset_packing_test(
	AssetForest& forest,
	BinaryAsset& binary,
	AssetType type,
	s32 percentage,
	AssetTestMode mode,
	const std::string& filter)
{
	const char* type_name = asset_type_to_string(type);
	std::string ref = binary.absolute_link().to_string();
	
	if (ref.find(filter) == std::string::npos) {
		return;
	}
	
	
	if (mode == AssetTestMode::PRINT_DIFF_ON_FAIL) {
		printf("[%3d%%] \033[34mRunning test with %s asset %s\033[0m\n", percentage, type_name, ref.c_str());
	}
	
	auto src_file = binary.src().open_binary_file_for_reading();
	std::vector<u8> src = src_file->read_multiple<u8>(src_file->size());
	MemoryInputStream src_stream(src);
	
	std::string hint = binary.format_hint();
	BuildConfig config(binary.game(), binary.region(), true);
	
	AssetDispatchTable* dispatch = dispatch_table_from_asset_type(type);
	verify_fatal(dispatch);
	
	AssetTestFunc* test_func;
	switch (config.game()) {
		case Game::RAC: test_func = dispatch->test_rac; break;
		case Game::GC: test_func = dispatch->test_gc; break;
		case Game::UYA: test_func = dispatch->test_uya; break;
		case Game::DL: test_func = dispatch->test_dl; break;
		default: return;
	}
	
	if (test_func) {
		if ((*test_func)(src, type, config, hint.c_str(), mode)) {
			if (mode == AssetTestMode::RUN_ALL_TESTS) {
				printf("\033[32m[PASS] %s\033[0m\n", ref.c_str());
			}
			pass_count++;
		} else {
			if (mode == AssetTestMode::RUN_ALL_TESTS) {
				printf("\033[31m[FAIL] %s\033[0m\n", ref.c_str());
			}
			fail_count++;
		}
	}
}

void strip_trailing_padding_from_lhs(std::vector<u8>& lhs, std::vector<u8>& rhs, s32 max_padding_size)
{
	if (rhs.size() > 0 && lhs.size() > rhs.size() && (max_padding_size == -1 || lhs.size() <= rhs.size() + max_padding_size)) {
		bool is_padding = true;
		for (s64 i = rhs.size(); i < lhs.size(); i++) {
			is_padding &= lhs[i] == '\0';
		}
		if (is_padding) {
			lhs.resize(rhs.size());
		}
	}
}
