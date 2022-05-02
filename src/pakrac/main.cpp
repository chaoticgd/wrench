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
#include <iso/wad_identifier.h>
#include <pakrac/tests.h>
#include <pakrac/wad_file.h>
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

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
static void unpack(const fs::path& input_path, const fs::path& output_path);
static void pack(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path);
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
			g_asset_unpacker.dump_wads = false;
			g_asset_unpacker.dump_binaries = false;
			
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack(args.input_paths[0], args.output_path);
			return 0;
		} else if(continuation == "_wads") {
			g_asset_unpacker.dump_wads = true;
			g_asset_unpacker.dump_global_wads = true;
			g_asset_unpacker.dump_level_wads = true;
			g_asset_unpacker.dump_binaries = false;
			
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack(args.input_paths[0], args.output_path);
			return 0;
		} else if(continuation == "_global_wads") {
			g_asset_unpacker.dump_wads = true;
			g_asset_unpacker.dump_global_wads = true;
			g_asset_unpacker.dump_level_wads = false;
			g_asset_unpacker.dump_binaries = false;
			
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack(args.input_paths[0], args.output_path);
			return 0;
		} else if(continuation == "_level_wads") {
			g_asset_unpacker.dump_wads = true;
			g_asset_unpacker.dump_global_wads = false;
			g_asset_unpacker.dump_level_wads = true;
			g_asset_unpacker.dump_binaries = false;
			
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack(args.input_paths[0], args.output_path);
			return 0;
		} else if(continuation == "_binaries") {
			g_asset_unpacker.dump_wads = false;
			g_asset_unpacker.dump_binaries = true;
			
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack(args.input_paths[0], args.output_path);
			return 0;
		}
	}
	
	if(mode.starts_with("pack")) {
		std::string continuation = mode.substr(4);
		if(continuation.empty()) {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATHS | ARG_ASSET | ARG_OUTPUT_PATH);
			pack(args.input_paths, args.asset, args.output_path);
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

static void unpack(const fs::path& input_path, const fs::path& output_path) {
	AssetForest forest;
	
	AssetBank& pack = forest.mount<LooseAssetBank>("unpacked", output_path, true);
	pack.game_info.type = AssetPackType::UNPACKED;
	
	FileInputStream stream;
	verify(stream.open(input_path), "Failed to open input file '%s' for reading.", input_path.string().c_str());
	
	// Check if it's an ISO file.
	if(stream.size() > 16 * SECTOR_SIZE + 6) {
		stream.seek(16 * SECTOR_SIZE + 1);
		std::vector<char> identifier = stream.read_multiple<char>(5);
		
		if(memcmp(identifier.data(), "CD001", 5) == 0) {
			BuildAsset& build = pack.asset_file("build.asset").root().child<BuildAsset>("base_game");
			pack.game_info.builds = {build.reference()};
			
			g_asset_unpacker.input_file = &stream;
			g_asset_unpacker.current_file_offset = 0;
			g_asset_unpacker.total_file_size = stream.size();
			
			unpack_asset_impl(build, stream, Game::UNKNOWN);
			
			printf("[100%%] Done!\n");
			
			pack.write();
			return;
		}
	}
	
	// Check if it's a WAD.
	s32 header_size = static_cast<InputStream&>(stream).read<s32>(0);
	if(header_size < 0x10000) {
		stream.seek(0);
		std::vector<u8> header = stream.read_multiple<u8>(header_size);
		auto [game, type, name] = identify_wad(header);
		
		if(type != WadType::UNKNOWN) {
			Asset& root = pack.asset_file("wad.asset").root();
			
			Asset* wad = nullptr;
			switch(type) {
				case WadType::ARMOR: wad = &root.child<ArmorWadAsset>("wad"); break;
				case WadType::AUDIO: wad = &root.child<AudioWadAsset>("wad"); break;
				case WadType::BONUS: wad = &root.child<BonusWadAsset>("wad"); break;
				case WadType::GADGET: wad = &root.child<GadgetWadAsset>("wad"); break;
				case WadType::HUD: wad = &root.child<HudWadAsset>("wad"); break;
				case WadType::MISC: wad = &root.child<MiscWadAsset>("wad"); break;
				case WadType::MPEG: wad = &root.child<MpegWadAsset>("wad"); break;
				case WadType::ONLINE: wad = &root.child<OnlineWadAsset>("wad"); break;
				case WadType::SCENE: wad = &root.child<SceneWadAsset>("wad"); break;
				case WadType::SPACE: wad = &root.child<SpaceWadAsset>("wad"); break;
				case WadType::LEVEL: wad = &root.child<LevelWadAsset>("wad"); break;
				case WadType::LEVEL_AUDIO: wad = &root.child<LevelAudioWadAsset>("wad"); break;
				case WadType::LEVEL_SCENE: wad = &root.child<LevelSceneWadAsset>("wad"); break;
			}
			
			g_asset_unpacker.input_file = &stream;
			g_asset_unpacker.current_file_offset = 0;
			g_asset_unpacker.total_file_size = stream.size();
			
			unpack_asset_impl(*wad, stream, game);
			
			printf("[100%%] Done!\n");
			
			pack.write();
			return;
		}
	}
	
	verify_not_reached("Unable to detect type of input file '%s'!", input_path.string().c_str());
}

static void pack(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path) {
	printf("[  0%%] Mounting asset bank\n");
	
	AssetForest forest;
	
	for(const fs::path& input_path : input_paths) {
		forest.mount<LooseAssetBank>("src", input_path, false);
	}
	
	Asset& wad = forest.lookup_asset(parse_asset_reference(asset.c_str()), nullptr);
	
	printf("[  0%%] Scanning dependencies of %s\n", asset.c_str());
	
	// Find the number of assets we need to pack. This is used for estimating
	// the completion percentage.
	BlackHoleOutputStream dummy;
	g_asset_packer_max_assets_processed = 0;
	g_asset_packer_num_assets_processed = 0;
	g_asset_packer_dry_run = true;
	pack_asset_impl(dummy, nullptr, nullptr, wad, Game::DL);
	g_asset_packer_max_assets_processed = g_asset_packer_num_assets_processed;
	g_asset_packer_num_assets_processed = 0;
	g_asset_packer_dry_run = false;
	
	FileOutputStream iso;
	verify(iso.open(output_path), "Failed to open '%s' for writing.\n", output_path.string().c_str());
	
	pack_asset_impl(iso, nullptr, nullptr, wad, Game::DL);
	
	printf("[100%%] Done!\n");
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
	puts("PakRac, part of Wrench -- https://github.com/chaoticgd/wrench");
	puts("");
	puts(" An asset packer/unpacker for the Ratchet & Clank PS2 games intended for modding.");
	puts("");
	puts("USER SUBCOMMANDS");
	puts("");
	puts(" unpack <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of source files.");
	puts("");
	puts(" pack <input dirs> -a <asset> -o <output iso>");
	puts("   Pack an asset (e.g. base_game) to produce a built file (e.g. an ISO file).");
	puts("");
	puts("DEVELOPER SUBCOMMANDS");
	puts("");
	puts(" unpack_wad <input files> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of WAD files.");
	puts("");
	puts(" unpack_global_wad <input files> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of global WAD files.");
	puts("");
	puts(" unpack_level_wad <input files> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of level WAD files.");
	puts("");
	puts(" unpack_binaries <input files> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of binaries.");
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
