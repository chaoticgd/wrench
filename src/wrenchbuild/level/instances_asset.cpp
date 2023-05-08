/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include <engine/gameplay.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

static void unpack_instances_asset(InstancesAsset& dest, InputStream& src, BuildConfig config);
static bool test_instances_asset(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);
static const std::vector<GameplayBlockDescription>* get_gameplay_block_descriptions(Game game, const char* hint);

on_load(Instances, []() {
	InstancesAsset::funcs.unpack_rac1 = wrap_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_rac2 = wrap_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_rac3 = wrap_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_dl = wrap_unpacker_func<InstancesAsset>(unpack_instances_asset);
	
	InstancesAsset::funcs.test_rac = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_gc = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_uya = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_dl = new AssetTestFunc(test_instances_asset);
})

static void unpack_instances_asset(InstancesAsset& dest, InputStream& src, BuildConfig config) {
	
}

static bool test_instances_asset(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode) {
	const std::vector<GameplayBlockDescription>* blocks = get_gameplay_block_descriptions(config.game(), hint);
	
	Gameplay gameplay;
	PvarTypes types;
	read_gameplay(gameplay, types, src, config.game(), *blocks);
	std::vector<u8> dest = write_gameplay(gameplay, types, config.game(), *blocks);
	
	strip_trailing_padding_from_lhs(src, dest);
	return diff_buffers(src, dest, 0, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
}

static const std::vector<GameplayBlockDescription>* get_gameplay_block_descriptions(Game game, const char* hint) {
	const std::vector<GameplayBlockDescription>* blocks = nullptr;
	switch(game) {
		case Game::RAC:
			blocks = &RAC_GAMEPLAY_BLOCKS;
			break;
		case Game::GC:
		case Game::UYA:
			blocks = &GC_UYA_GAMEPLAY_BLOCKS;
			break;
		case Game::DL:
			if(strcmp(hint, "gameplay") == 0) {
				blocks = &DL_GAMEPLAY_CORE_BLOCKS;
			} if(strcmp(hint, "art") == 0) {
				blocks = &DL_ART_INSTANCE_BLOCKS;
			} else if(strcmp(hint, "mission") == 0) {
				blocks = &DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS;
			} else {
				verify_not_reached("Invalid hint. Must be 'gameplay' or 'art' or 'mission'.");
			}
			break;
		default: verify_fatal(0);
	}
	return blocks;
}
