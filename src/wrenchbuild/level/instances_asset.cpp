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

#include "instances_asset.h"

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

static void unpack_instances_asset(InstancesAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static bool test_instances_asset(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);
static const std::vector<GameplayBlockDescription>* get_gameplay_block_descriptions(Game game, const char* hint);

on_load(Instances, []() {
	InstancesAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_dl = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	
	InstancesAsset::funcs.test_rac = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_gc = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_uya = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_dl = new AssetTestFunc(test_instances_asset);
})

static void unpack_instances_asset(InstancesAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	
	Gameplay gameplay;
	read_gameplay(gameplay, buffer, config.game(), *get_gameplay_block_descriptions(config.game(), hint)); 
	
	Instances instances;
	move_gameplay_to_instances(instances, nullptr, nullptr, gameplay);
	
	std::string text = write_instances(instances);
	
	FileReference ref = dest.file().write_text_file(stringf("%s.instances", hint), text.c_str());
	dest.set_src(ref);
}

Gameplay load_instances(const Asset& src, const BuildConfig& config, const char* hint) {
	std::vector <u8> gameplay_buffer;
	if(const InstancesAsset* asset = src.maybe_as<InstancesAsset>()) {
		std::unique_ptr<InputStream> gameplay_stream = asset->file().open_binary_file_for_reading(asset->src());
		gameplay_buffer = gameplay_stream->read_multiple<u8>(gameplay_stream->size());
	} else if(const BinaryAsset* asset = src.maybe_as<BinaryAsset>()) {
		std::unique_ptr<InputStream> gameplay_stream = asset->file().open_binary_file_for_reading(asset->src());
		gameplay_buffer = gameplay_stream->read_multiple<u8>(gameplay_stream->size());
	}
	Gameplay gameplay;
	read_gameplay(gameplay, gameplay_buffer, config.game(), *gameplay_block_descriptions_from_game(config.game()));
	return gameplay;
}

static bool test_instances_asset(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode) {
	const std::vector<GameplayBlockDescription>* blocks = get_gameplay_block_descriptions(config.game(), hint);
	
	// Parse gameplay file.
	Gameplay gameplay_in;
	read_gameplay(gameplay_in, src, config.game(), *blocks);
	
	// Separate out the different parts of the file.
	Instances instances_in;
	HelpMessages help_messages;
	OcclusionMappings occlusion;
	move_gameplay_to_instances(instances_in, &help_messages, &occlusion, gameplay_in);
	
	// Write out instances file and read it back.
	std::string instances_text = write_instances(instances_in);
	write_file("/tmp/instances.txt", instances_text);
	Instances instances_out = read_instances(instances_text);
	instances_out.global_pvar = std::move(instances_in.global_pvar);
	
	// Write out new gameplay file.
	Gameplay gameplay_out;
	move_instances_to_gameplay(gameplay_out, instances_out, &help_messages, &occlusion);
	gameplay_out.pvar_table = gameplay_in.pvar_table;
	gameplay_out.pvar_scratchpad = gameplay_in.pvar_scratchpad;
	gameplay_out.pvar_relatives = gameplay_in.pvar_relatives;
	gameplay_out.global_pvar = gameplay_in.global_pvar;
	gameplay_out.global_pvar_table = gameplay_in.global_pvar_table;
	std::vector<u8> dest = write_gameplay(gameplay_out, config.game(), *blocks);
		
	// Compare the new file against the original.
	strip_trailing_padding_from_lhs(src, dest);
	bool headers_equal = diff_buffers(src, dest, 0, 0x100, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	bool data_equal = diff_buffers(src, dest, 0x100, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	return headers_equal && data_equal;
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
			if(strcmp(hint, FMT_INSTANCES_GAMEPLAY) == 0) {
				blocks = &DL_GAMEPLAY_CORE_BLOCKS;
			} else if(strcmp(hint, FMT_INSTANCES_ART) == 0) {
				blocks = &DL_ART_INSTANCE_BLOCKS;
			} else if(strcmp(hint, FMT_INSTANCES_MISSION) == 0) {
				blocks = &DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS;
			} else {
				verify_not_reached("Invalid hint. Must be '%s', '%s' or '%s'.",
					FMT_INSTANCES_GAMEPLAY, FMT_INSTANCES_ART, FMT_INSTANCES_MISSION);
			}
			break;
		default: verify_fatal(0);
	}
	return blocks;
}
