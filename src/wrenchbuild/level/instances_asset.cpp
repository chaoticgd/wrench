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

#include <toolwads/wads.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

static void unpack_instances_asset(
	InstancesAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void unpack_help_messages(LevelWadAsset& dest, const HelpMessages& src, BuildConfig config);
static void pack_instances_asset(
	OutputStream& dest, const InstancesAsset& src, BuildConfig config, const char* hint);
static void pack_help_messages(HelpMessages& dest, const LevelWadAsset& src, BuildConfig config);
static bool test_instances_asset(
	std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);
static const std::vector<GameplayBlockDescription>* get_gameplay_block_descriptions(Game game, const char* hint);

on_load(Instances, []() {
	InstancesAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	InstancesAsset::funcs.unpack_dl = wrap_hint_unpacker_func<InstancesAsset>(unpack_instances_asset);
	
	InstancesAsset::funcs.pack_rac1 = wrap_hint_packer_func<InstancesAsset>(pack_instances_asset);
	InstancesAsset::funcs.pack_rac2 = wrap_hint_packer_func<InstancesAsset>(pack_instances_asset);
	InstancesAsset::funcs.pack_rac3 = wrap_hint_packer_func<InstancesAsset>(pack_instances_asset);
	InstancesAsset::funcs.pack_dl = wrap_hint_packer_func<InstancesAsset>(pack_instances_asset);
	
	InstancesAsset::funcs.test_rac = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_gc = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_uya = new AssetTestFunc(test_instances_asset);
	InstancesAsset::funcs.test_dl = new AssetTestFunc(test_instances_asset);
})

static void unpack_instances_asset(
	InstancesAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	unpack_instances(dest, nullptr, buffer, nullptr, config, hint);
}

s32 unpack_instances(
	InstancesAsset& dest, LevelWadAsset* help_occl_dest, const std::vector<u8>& main, const std::vector<u8>* art, BuildConfig config, const char* hint)
{
	std::vector<u8> main_decompressed;
	verify(decompress_wad(main_decompressed, main), "Failed to decompress instances.");
	
	std::string type = next_hint(&hint);
	
	s32 core_moby_count = 0;
	if (type == "mission") {
		core_moby_count = atoi(next_hint(&hint));
	}
	
	Gameplay gameplay;
	gameplay.core_moby_count = core_moby_count;
	read_gameplay(gameplay, main_decompressed, config.game(), *get_gameplay_block_descriptions(config.game(), type.c_str()));
	
	Instances instances;
	HelpMessages help;
	std::vector<CppType> pvar_types;
	move_gameplay_to_instances(instances, &help, nullptr, &pvar_types, gameplay, config.game());
	
	std::vector<u8> occlusion_mappings;
	
	if (art) {
		std::vector<u8> art_decompressed;
		verify(decompress_wad(art_decompressed, *art), "Failed to decompress art instances.");
		
		Gameplay art_instances;
		read_gameplay(art_instances, art_decompressed, config.game(), DL_ART_INSTANCE_BLOCKS);
		instances.dir_lights = std::move(opt_iterator(art_instances.dir_lights));
		instances.tie_instances = std::move(opt_iterator(art_instances.tie_instances));
		instances.tie_groups = std::move(opt_iterator(art_instances.tie_groups));
		instances.shrub_instances = std::move(opt_iterator(art_instances.shrub_instances));
		instances.shrub_groups = std::move(opt_iterator(art_instances.shrub_groups));
		occlusion_mappings = std::move(opt_iterator(art_instances.occlusion));
	} else {
		occlusion_mappings = std::move(opt_iterator(gameplay.occlusion));
	}
	
	const char* application_version;
	if (strlen(wadinfo.build.version_string) != 0) {
		application_version = wadinfo.build.version_string;
	} else {
		application_version = wadinfo.build.commit_string;
	}
	std::string text = write_instances(instances, "Wrench Build Tool", application_version);
	FileReference ref = dest.file().write_text_file(stringf("%s.instances", type.c_str()), text.c_str());
	dest.set_src(ref);
	
	if (help_occl_dest) {
		unpack_help_messages(*help_occl_dest, help, config);
		if (!occlusion_mappings.empty()) {
			OcclusionAsset& occl = help_occl_dest->occlusion();
			auto [stream, ref] = occl.file().open_binary_file_for_writing(fs::path(std::string("occlusion_mappings.bin")));
			stream->write_n(occlusion_mappings.data(), occlusion_mappings.size());
			occl.set_mappings(ref);
		}
	}
	
	// Merge types.
	std::map<std::string, CppType>& types_dest = dest.forest().types();
	for (CppType& type : pvar_types) {
		auto iter = types_dest.find(type.name);
		if (iter != types_dest.end()) {
			destructively_merge_cpp_structs(iter->second, type);
		} else {
			types_dest.emplace(type.name, std::move(type));
		}
	}
	
	return instances.moby_instances.size();
}

static void unpack_help_messages(LevelWadAsset& dest, const HelpMessages& src, BuildConfig config)
{
	if (src.us_english.has_value()) {
		MemoryInputStream us_english_stream(*src.us_english);
		unpack_asset_impl(dest.help_messages_us_english(), us_english_stream, nullptr, config);
	}
	if (src.uk_english.has_value()) {
		MemoryInputStream uk_english_stream(*src.uk_english);
		unpack_asset_impl(dest.help_messages_uk_english(), uk_english_stream, nullptr, config);
	}
	if (src.french.has_value()) {
		MemoryInputStream french_stream(*src.french);
		unpack_asset_impl(dest.help_messages_french(), french_stream, nullptr, config);
	}
	if (src.german.has_value()) {
		MemoryInputStream german_stream(*src.german);
		unpack_asset_impl(dest.help_messages_german(), german_stream, nullptr, config);
	}
	if (src.spanish.has_value()) {
		MemoryInputStream spanish_stream(*src.spanish);
		unpack_asset_impl(dest.help_messages_spanish(), spanish_stream, nullptr, config);
	}
	if (src.italian.has_value()) {
		MemoryInputStream italian_stream(*src.italian);
		unpack_asset_impl(dest.help_messages_italian(), italian_stream, nullptr, config);
	}
	if (src.japanese.has_value()) {
		MemoryInputStream japanese_stream(*src.japanese);
		unpack_asset_impl(dest.help_messages_japanese(), japanese_stream, nullptr, config);
	}
	if (src.korean.has_value()) {
		MemoryInputStream korean_stream(*src.korean);
		unpack_asset_impl(dest.help_messages_korean(), korean_stream, nullptr, config);
	}
}

Gameplay load_gameplay(
	const Asset& src,
	const LevelWadAsset* help_occl_src,
	const std::map<std::string, CppType>& types_src,
	const BuildConfig& config,
	const char* hint)
{
	if (g_asset_packer_dry_run) {
		return {};
	}
	
	std::vector<u8> gameplay_buffer;
	if (const InstancesAsset* asset = src.maybe_as<InstancesAsset>()) {
		std::string instances_wtf = asset->src().read_text_file();
		Instances instances = read_instances(instances_wtf);
		Opt<HelpMessages> help;
		if (help_occl_src) {
			pack_help_messages(help.emplace(), *help_occl_src, config);
		}
		Gameplay gameplay;
		move_instances_to_gameplay(gameplay, instances, &(*help), nullptr, types_src);
		return gameplay;
	} else if (const BinaryAsset* asset = src.maybe_as<BinaryAsset>()) {
		std::unique_ptr<InputStream> gameplay_stream = asset->src().open_binary_file_for_reading();
		std::vector<u8> buffer = gameplay_stream->read_multiple<u8>(gameplay_stream->size());
		Gameplay gameplay;
		read_gameplay(gameplay, buffer, config.game(), *get_gameplay_block_descriptions(config.game(), hint));
		return gameplay;
	}
	verify_not_reached("Instances asset is of an invalid type.");
}

static void pack_instances_asset(
	OutputStream& dest,
	const InstancesAsset& src,
	BuildConfig config,
	const char* hint)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	std::string type = next_hint(&hint);
	
	std::string instances_str = src.src().read_text_file();
	Instances instances = read_instances(instances_str);
	
	// If we're packing a mission instances file, we also read the gameplay core
	// to determine ID to index mappings for moby instances. TODO: Cache this.
	Opt<Instances> core;
	if (type == "mission") {
		const InstancesAsset& gameplay_core = src.get_core();
		std::string gameplay_core_str = gameplay_core.src().read_text_file();
		core = read_instances(gameplay_core_str);
		instances.core = &(*core);
	}
	
	Gameplay gameplay;
	gameplay.core_moby_count = core.has_value() ? core->moby_instances.size() : 0;
	move_instances_to_gameplay(gameplay, instances, nullptr, nullptr, src.forest().types());
	std::vector<u8> buffer = write_gameplay(gameplay, config.game(), *get_gameplay_block_descriptions(config.game(), type.c_str()));
	dest.write_v(buffer);
}

static void pack_help_messages(HelpMessages& dest, const LevelWadAsset& src, BuildConfig config)
{
	if (src.has_help_messages_us_english()) {
		MemoryOutputStream stream(dest.us_english.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_us_english(), config);
	}
	if (src.has_help_messages_uk_english()) {
		MemoryOutputStream stream(dest.uk_english.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_uk_english(), config);
	}
	if (src.has_help_messages_french()) {
		MemoryOutputStream stream(dest.french.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_french(), config);
	}
	if (src.has_help_messages_german()) {
		MemoryOutputStream stream(dest.german.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_german(), config);
	}
	if (src.has_help_messages_spanish()) {
		MemoryOutputStream stream(dest.spanish.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_spanish(), config);
	}
	if (src.has_help_messages_italian()) {
		MemoryOutputStream stream(dest.italian.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_italian(), config);
	}
	if (src.has_help_messages_japanese()) {
		MemoryOutputStream stream(dest.japanese.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_japanese(), config);
	}
	if (src.has_help_messages_korean()) {
		MemoryOutputStream stream(dest.korean.emplace());
		pack_asset_impl(stream, nullptr, nullptr, src.get_help_messages_korean(), config);
	}
}

static bool test_instances_asset(
	std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode)
{
	const std::vector<GameplayBlockDescription>* blocks = get_gameplay_block_descriptions(config.game(), hint);
	
	// Parse C++ types from the overlay asset bank.
	AssetForest type_forest;
	type_forest.mount<LooseAssetBank>("data/overlay", false);
	type_forest.read_source_files(config.game());
	
	// Parse gameplay file.
	Gameplay gameplay_in;
	read_gameplay(gameplay_in, src, config.game(), *blocks);
	
	// Separate out the different parts of the file.
	Instances instances_in;
	HelpMessages help_messages;
	std::vector<u8> occlusion;
	std::vector<CppType> pvar_types;
	move_gameplay_to_instances(instances_in, &help_messages, &occlusion, &pvar_types, gameplay_in, config.game());
	
	// Add recovered type information to the parsed map of pvar types.
	for (CppType& pvar_type : pvar_types) {
		type_forest.types().emplace(pvar_type.name, std::move(pvar_type));
	}
	
	// Write out instances file and read it back.
	const char* application_version;
	if (strlen(wadinfo.build.version_string) != 0) {
		application_version = wadinfo.build.version_string;
	} else {
		application_version = wadinfo.build.commit_string;
	}
	std::string instances_text = write_instances(instances_in, "Wrench Build Tool (Test)", application_version);
	write_file("/tmp/instances.txt", instances_text);
	Instances instances_out = read_instances(instances_text);
	
	// Write out new gameplay file.
	Gameplay gameplay_out;
	move_instances_to_gameplay(gameplay_out, instances_out, &help_messages, &occlusion, type_forest.types());
	std::vector<u8> dest = write_gameplay(gameplay_out, config.game(), *blocks);
		
	// Compare the new file against the original.
	strip_trailing_padding_from_lhs(src, dest);
	bool headers_equal = diff_buffers(src, dest, 0, 0x100, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	bool data_equal = diff_buffers(src, dest, 0x100, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	return headers_equal && data_equal;
}

static const std::vector<GameplayBlockDescription>* get_gameplay_block_descriptions(
	Game game, const char* hint)
{
	const std::vector<GameplayBlockDescription>* blocks = nullptr;
	switch (game) {
		case Game::RAC:
			blocks = &RAC_GAMEPLAY_BLOCKS;
			break;
		case Game::GC:
		case Game::UYA:
			blocks = &GC_UYA_GAMEPLAY_BLOCKS;
			break;
		case Game::DL:
			if (strcmp(hint, FMT_INSTANCES_GAMEPLAY) == 0) {
				blocks = &DL_GAMEPLAY_CORE_BLOCKS;
			} else if (strcmp(hint, FMT_INSTANCES_ART) == 0) {
				blocks = &DL_ART_INSTANCE_BLOCKS;
			} else if (strcmp(hint, FMT_INSTANCES_MISSION) == 0) {
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
