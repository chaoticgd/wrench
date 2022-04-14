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

#include <fstream>

#include <core/util.h>
#include <core/timer.h>
#include <core/stream.h>
#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/moby.h>
#include <engine/collision.h>
#include <iso/iso_packer.h>
#include <iso/iso_unpacker.h>
#include <iso/iso_tools.h>
#include <spanner/tests.h>
#include <spanner/wad_file.h>
#include <spanner/global_wads.h>
#include <spanner/level/level_wad.h>
#include <spanner/level/level_audio_wad.h>
#include <spanner/level/level_scene_wad.h>
#include <spanner/asset_packer.h>

enum ArgFlags : u32 {
	ARG_INPUT_PATH = 1 << 0,
	ARG_INPUT_PATHS = 1 << 1,
	ARG_ASSET = 1 << 2,
	ARG_OUTPUT_PATH = 1 << 3,
	ARG_OFFSET = 1 << 4
};

struct ParsedArgs {
	std::vector<fs::path> input_paths;
	std::string asset;
	fs::path output_path;
	s64 offset = -1;
};

static ParsedArgs parse_args(int argc, char** argv, u32 flags);
static void unpack_wads(const fs::path& input_path, const fs::path& output_path, bool unpack_globals, bool unpack_levels);
static void unpack_level_wads(AssetPack& dest_pack, BuildAsset& dest_build, BuildAsset& src_build);
static void unpack_bins(const fs::path& input_path, const fs::path& output_path);
static void pack(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path);
static void pack_bin(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path);
static void pack_wad_asset(OutputStream& dest, std::vector<u8>* wad_header_dest, fs::file_time_type* modified_time_dest, Asset& asset);
static void decompress(const fs::path& input_path, const fs::path& output_path, s64 offset);
static void compress(const fs::path& input_path, const fs::path& output_path);
static void extract(fs::path input_path, fs::path output_path);
static void build(fs::path input_path, fs::path output_path);
static void extract_collision(fs::path input_path, fs::path output_path);
static void build_collision(fs::path input_path, fs::path output_path);
static void extract_moby(const char* input_path, const char* output_path);
static void build_moby(const char* input_path, const char* output_path);
static void print_usage();

#define require_args(arg_count) verify(argc == arg_count, "Incorrect number of arguments.");

int main(int argc, char** argv) {
	if(argc < 2) {
		print_usage();
		return 1;
	}
	
	std::string mode = argv[1];
	
	if(mode.starts_with("unpack")) {
		std::string continuation = mode.substr(6);
		if(continuation.empty()) {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_iso(args.input_paths[0], args.output_path/"wad");
			unpack_wads(args.output_path/"wad", args.output_path/"bin", true, true);
			unpack_bins(args.output_path/"bin", args.output_path/"unpacked");
			return 0;
		} else if(continuation == "_iso") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_iso(args.input_paths[0], args.output_path);
			return 0;
		} else if(continuation == "_wads") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_wads(args.input_paths[0], args.output_path, true, true);
			return 0;
		} else if(continuation == "_global_wads") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_wads(args.input_paths[0], args.output_path, true, false);
			return 0;
		} else if(continuation == "_level_wads") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_wads(args.input_paths[0], args.output_path, false, true);
			return 0;
		} else if(continuation == "_bin") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_bins(args.input_paths[0], args.output_path);
			return 0;
		}
	}
	
	if(mode.starts_with("pack")) {
		std::string continuation = mode.substr(4);
		if(continuation.empty()) {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATHS | ARG_ASSET | ARG_OUTPUT_PATH);
			pack(args.input_paths, args.asset, args.output_path);
			return 0;
		} else if(continuation == "_bin") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATHS | ARG_ASSET | ARG_OUTPUT_PATH);
			pack_bin(args.input_paths, args.asset, args.output_path);
			return 0;
		}
	}
	
	if(mode == "decompress") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH | ARG_OFFSET);
		decompress(args.input_paths[0], args.output_path, args.offset);
		return 0;
	}
	
	if(mode == "compress") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
		compress(args.input_paths[0], args.output_path);
		return 0;
	}
	
	if(mode == "inspect_iso") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH);
		inspect_iso(args.input_paths[0]);
		return 0;
	}
	
	if(mode == "extract_collision") {
		require_args(4);
		extract_collision(argv[2], argv[3]);
	} else if(mode == "build_collision") {
		require_args(4);
		build_collision(argv[2], argv[3]);
	} else if(mode == "extract_moby") {
		require_args(4);
		extract_moby(argv[2], argv[3]);
	} else if(mode == "build_moby") {
		require_args(4);
		build_moby(argv[2], argv[3]);
	} else if(mode == "test") {
		require_args(3);
		run_tests(argv[2]);
	} else {
		print_usage();
		return 1;
	}
	return 0;
}

static ParsedArgs parse_args(int argc, char** argv, u32 flags) {
	ParsedArgs args;
	
	for(int i = 2; i < argc; i++) {
		if((flags & ARG_ASSET) && strcmp(argv[i], "-a") == 0) {
			verify(i + 1 < argc, "Expected asset reference argument.");
			args.asset = argv[++i];
			continue;
		}
		
		if((flags & ARG_OUTPUT_PATH) && strcmp(argv[i], "-o") == 0) {
			verify(i + 1 < argc, "Expected output path argument.");
			args.output_path = argv[++i];
			continue;
		}
		
		if((flags & ARG_OFFSET) && strcmp(argv[i], "-x") == 0) {
			verify(i + 1 < argc, "Expected offset argument.");
			args.offset = parse_number(argv[++i]);
			continue;
		}
		
		if((flags & ARG_INPUT_PATH) || (flags & ARG_INPUT_PATHS)) {
			args.input_paths.emplace_back(argv[i]);
		}
	}
	
	if(flags & ARG_INPUT_PATH) {
		verify(!args.input_paths.empty(), "Input path not specified.");
		verify(args.input_paths.size() <= 1, "Multiple input paths specified.");
	} else if(flags & ARG_INPUT_PATHS) {
		verify(!args.input_paths.empty(), "Input paths not specified.");
	} else {
		verify(args.input_paths.empty(), "Unknown argument.");
	}
	verify(((flags & ARG_ASSET) == 0) || !args.asset.empty(), "Asset reference (-a) not specified.");
	verify(((flags & ARG_OUTPUT_PATH) == 0) || !args.output_path.empty(), "Output path (-o) not specified.");
	verify(((flags & ARG_OFFSET) == 0) || (args.offset != -1), "Offset (-x) not specified.");
	
	return args;
}

static void unpack_wads(const fs::path& input_path, const fs::path& output_path, bool unpack_globals, bool unpack_levels) {
	AssetForest src_forest;
	
	AssetPack& src_pack = src_forest.mount<LooseAssetPack>("wads", input_path, false);
	verify(src_pack.game_info.type == AssetPackType::WADS,
		"unpack_wads can only be run on an WAD asset pack.");
	
	auto& builds = src_pack.game_info.builds;
	verify(builds.size() == 1, "WAD asset pack must have exactly one build.");
	BuildAsset* src_build = dynamic_cast<BuildAsset*>(src_forest.lookup_asset(builds[0]));
	verify(src_build, "Invalid build asset.");
	
	AssetForest dest_forest;
	
	AssetPack& dest_pack = dest_forest.mount<LooseAssetPack>("bins", output_path, true);
	dest_pack.game_info.type = AssetPackType::BINS;
	
	BuildAsset& dest_build = dest_pack.asset_file("build.asset").root().child<BuildAsset>("base_game");
	dest_pack.game_info.builds = {dest_build.absolute_reference()};
	
	if(unpack_globals) {
		unpack_global_wads(dest_pack, dest_build, *src_build);
	}
	
	if(unpack_levels) {
		unpack_level_wads(dest_pack, dest_build, *src_build);
	}
	
	dest_pack.write();
}

static void unpack_level_wads(AssetPack& dest_pack, BuildAsset& dest_build, BuildAsset& src_build) {
	src_build.get_levels().for_each_logical_child_of_type<LevelAsset>([&](LevelAsset& src_level) {
		AssetFile& dest_file = dest_pack.asset_file(stringf("level/%s/level%s.asset", src_level.tag().c_str(), src_level.tag().c_str()));
		CollectionAsset& levels_collection = dest_file.root().child<CollectionAsset>("levels");
		LevelAsset& dest_level = levels_collection.child<LevelAsset>(src_level.tag().c_str());
		
		if(src_level.has_level()) {
			unpack_level_wad(dest_level.level<LevelWadAsset>().switch_files(), src_level.get_level().as<BinaryAsset>());
		}
		
		if(src_level.has_audio()) {
			unpack_level_audio_wad(dest_level.audio<LevelAudioWadAsset>().switch_files(), src_level.get_audio().as<BinaryAsset>());
		}
		
		if(src_level.has_scene()) {
			unpack_level_scene_wad(dest_level.scene<LevelSceneWadAsset>().switch_files(), src_level.get_scene().as<BinaryAsset>());
		}
		
		dest_build.levels().child<ReferenceAsset>(src_level.tag().c_str()).set_asset(stringf("/levels/%s", src_level.tag().c_str()));
	});
}

static void unpack_bins(const fs::path& input_path, const fs::path& output_path) {
	
}

static void pack(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path) {
	FileOutputStream iso;
	verify(iso.open(output_path), "Failed to open '%s' for writing.\n", output_path.string().c_str());
	
	printf("[  ?%] \033[32mMounting asset directories\033[0m\n");
	
	AssetForest forest;
	
	for(const fs::path& input_path : input_paths) {
		AssetPack& src_pack = forest.mount<LooseAssetPack>("src", input_path, false);
	}
	
	Asset* wad = forest.lookup_asset(parse_asset_reference(asset.c_str()));
	verify(wad, "Invalid asset path.");
	pack_asset_impl(iso, nullptr, nullptr, *wad, Game::DL);
}

static void pack_bin(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path) {
	
}

static void decompress(const fs::path& input_path, const fs::path& output_path, s64 offset) {
	FILE* file = fopen(input_path.string().c_str(), "rb");
	verify(file, "Failed to open file for reading.");
	
	std::vector<u8> header = read_file(file, offset, 0x10);
	verify(header[0] == 'W' && header[1] == 'A' && header[2] == 'D',
		"Invalid WAD header (magic bytes aren't correct).");
	s32 compressed_size = Buffer(header).read<s32>(3, "compressed size");
	std::vector<u8> compressed_bytes = read_file(file, offset, compressed_size);
	
	std::vector<u8> decompressed_bytes;
	decompress_wad(decompressed_bytes, compressed_bytes);
	
	write_file(output_path, decompressed_bytes);
}

static void compress(const fs::path& input_path, const fs::path& output_path) {
	std::vector<u8> bytes = read_file(input_path);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, 8);
	
	write_file(output_path, compressed_bytes);
}

static void extract_collision(fs::path input_path, fs::path output_path) {
	auto collision = read_file(input_path);
	write_file("/", output_path, write_collada(read_collision(collision)), "w");
}

static void build_collision(fs::path input_path, fs::path output_path) {
	auto collision = read_file(input_path, "r");
	std::vector<u8> bin;
	write_collision(bin, read_collada(collision));
	write_file("/", output_path, bin);
}

static void extract_moby(const char* input_path, const char* output_path) {
	auto bin = read_file(input_path);
	MobyClassData moby = read_moby_class(bin, Game::RAC2);
	ColladaScene scene = recover_moby_class(moby, 0, 0);
	auto xml = write_collada(scene);
	write_file("/", output_path, xml, "w");
}

static void build_moby(const char* input_path, const char* output_path) {
	auto xml = read_file(input_path, "r");
	ColladaScene scene = read_collada(xml);
	MobyClassData moby = build_moby_class(scene);
	std::vector<u8> buffer;
	write_moby_class(buffer, moby, Game::RAC2);
	write_file("/", output_path, buffer);
}

static void print_usage() {
	puts("Spanner, part of Wrench -- https://github.com/chaoticgd/wrench");
	puts("");
	puts(" An asset packer/unpacker for the Ratchet & Clank PS2 games intended for modding.");
	puts("");
	puts("SUBCOMMANDS");
	puts("");
	puts(" unpack <input iso> -o <output dir>");
	puts("   Run unpack_iso, unpack_wads and unpack_bins in sequence.");
	puts("");
	puts("   unpack_iso <input iso> -o <output dir>");
	puts("     Unpack the ISO file and produce an asset directory of WAD files.");
	puts("");
	puts("   unpack_wads <input dir> -o <output dir>");
	puts("     Unpack an asset directory of WAD files to a new asset directory of loose");
	puts("     binary files.");
	puts("");
	puts("     unpack_global_wads <input dir> -o <output dir>");
	puts("       Unpack only global WAD files.");
	puts("");
	puts("     unpack_global_wads <input dir> -o <output dir>");
	puts("       Unpack only level WAD files.");
	puts("");
	puts("   unpack_bin <input dir> -o <output dir>");
	puts("     Unpack an asset directory of loose binary files to a new asset directory of"); // max line length
	puts("     source assets.");
	puts("");
	puts("     unpack_collision <input bin> -o <output dae>");
	puts("       Unpack a loose collision file and produce a COLLADA mesh.");
	puts("");
	puts("");
	puts(" pack <input dirs> -a <asset> -o <output iso>");
	puts("   Pack an asset directory to produce an ISO file.");
	puts("");
	puts("   pack_bin <input dirs> -a <asset> -o <output iso>");
	puts("     Pack an asset directory to produce an ISO file.");
	puts("");
	puts("   pack_collision <input dae> -o <output bin>");
	puts("     Pack a COLLADA mesh into a collision file.");
	puts("");
	puts("");
	puts(" decompress <input file> -o <output file> -x <offset>");
	puts("   Decompress a file stored using the game's custom LZ compression scheme.");
	puts("");
	puts(" compress <input file> -o <output file>");
	puts("   Compress a file using the game's custom LZ compression scheme.");
	puts("");
	puts(" inspect_iso <input iso>");
	puts("   Print out a summary of where assets are in the provided ISO file.");
	puts("");
	puts(" parse_pcsx2_cdvd_log <input iso>");
	puts("   Interpret the output of PCSX2's disc block access log (from stdin) and print");
	puts("   out file accesses as they occur.");
	puts("");
	puts(" test <level wads dir>");
	puts("   Run tests.");
}
