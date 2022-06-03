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
#include <toolwads/wads.h>
#include <wrenchbuild/tests.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

enum ArgFlags : u32 {
	ARG_INPUT_PATH = 1 << 0,
	ARG_INPUT_PATHS = 1 << 1,
	ARG_ASSET = 1 << 2,
	ARG_OUTPUT_PATH = 1 << 3,
	ARG_OFFSET = 1 << 4,
	ARG_GAME = 1 << 5
};

struct ParsedArgs {
	std::vector<fs::path> input_paths;
	std::string asset;
	fs::path output_path;
	s64 offset = -1;
	Game game = Game::RAC1;
};

static ParsedArgs parse_args(int argc, char** argv, u32 flags);
static void unpack(const fs::path& input_path, const fs::path& output_path);
static void pack(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path, Game game);
static void decompress(const fs::path& input_path, const fs::path& output_path, s64 offset);
static void compress(const fs::path& input_path, const fs::path& output_path);
static void extract(fs::path input_path, fs::path output_path);
static void build(fs::path input_path, fs::path output_path);
static void extract_collision(fs::path input_path, fs::path output_path);
static void build_collision(fs::path input_path, fs::path output_path);
static void extract_moby(const char* input_path, const char* output_path);
static void build_moby(const char* input_path, const char* output_path);
static void print_usage();
static void print_version();

#define require_args(arg_count) verify(argc == arg_count, "Incorrect number of arguments.");

int main(int argc, char** argv) {
	if(argc < 2) {
		print_usage();
		return 1;
	}
	
	std::string mode = argv[1];
	
	if(mode.starts_with("unpack")) {
		std::string continuation = mode.substr(6);
		if(continuation == "_globals") {
			g_asset_unpacker.skip_levels = true;
		} else if(continuation == "_levels") {
			g_asset_unpacker.skip_globals = true;
		} else if(continuation == "_wads") {
			g_asset_unpacker.dump_wads = true;
		} else if(continuation == "_global_wads") {
			g_asset_unpacker.skip_levels = true;
			g_asset_unpacker.dump_wads = true;
		} else if(continuation == "_level_wads") {
			g_asset_unpacker.skip_globals = true;
			g_asset_unpacker.dump_wads = true;
		} else if(continuation == "_binaries") {
			g_asset_unpacker.dump_binaries = true;
		} else if(continuation == "_flat") {
			g_asset_unpacker.dump_flat = true;
		} else if(!continuation.empty()) {
			print_usage();
			return 1;
		}
		
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
		unpack(args.input_paths[0], args.output_path);
		report_memory_statistics();
		return 0;
	}
	
	if(mode == "pack") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATHS | ARG_ASSET | ARG_OUTPUT_PATH | ARG_GAME);
		pack(args.input_paths, args.asset, args.output_path, args.game);
		report_memory_statistics();
		return 0;
	}
	
	if(mode == "help" || mode == "-h" || mode == "--help") {
		print_usage();
		return 0;
	}
	
	if(mode == "version" || mode == "-v" || mode == "--version") {
		print_version();
		return 0;
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
	
	if(mode == "parse_pcsx2_cdvd_log") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH);
		parse_pcsx2_cdvd_log(args.input_paths[0]);
		return 0;
	}
	
	if(mode == "profile_memory_usage") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH);
		{
			AssetForest forest;
			for(const fs::path& input_path : args.input_paths) {
				forest.mount<LooseAssetBank>(input_path, false);
			}
		}
		report_memory_statistics();
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
		
		if((flags & ARG_GAME) && strcmp(argv[i], "-g") == 0) {
			verify(i + 1 < argc, "Expected game argument.");
			const char* game = argv[++i];
			if(strcmp(game, "rac") == 0) {
				args.game = Game::RAC1;
			} else if(strcmp(game, "gc") == 0) {
				args.game = Game::RAC2;
			} else if (strcmp(game, "uya") == 0) {
				args.game = Game::RAC3;
			} else if(strcmp(game, "dl") == 0) {
				args.game = Game::DL;
			} else {
				verify_not_reached("Invalid game argument. Options are: rac, gc, uya, dl.");
			}
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
	
	AssetBank& bank = forest.mount<LooseAssetBank>(output_path, true);
	
	if(g_asset_unpacker.dump_binaries) {
		bank.game_info.type = AssetBankType::TEST;
	} else {
		bank.game_info.type = AssetBankType::UNPACKED;
	}
	
	FileInputStream stream;
	verify(stream.open(input_path), "Failed to open input file '%s' for reading.", input_path.string().c_str());
	
	// Check if it's an ISO file.
	if(stream.size() > 16 * SECTOR_SIZE + 6) {
		stream.seek(16 * SECTOR_SIZE + 1);
		std::vector<char> identifier = stream.read_multiple<char>(5);
		
		if(memcmp(identifier.data(), "CD001", 5) == 0) {
			BuildAsset& build = bank.asset_file("build.asset").root().child<BuildAsset>("base_game");
			
			g_asset_unpacker.input_file = &stream;
			g_asset_unpacker.current_file_offset = 0;
			g_asset_unpacker.total_file_size = stream.size();
			
			unpack_asset_impl(build, stream, Game::UNKNOWN);
			
			bank.game_info.name = build.game();
			bank.game_info.format_version = ASSET_FORMAT_VERSION;
			bank.game_info.builds = {build.reference()};
			
			printf("[100%%] Done!\n");
			
			bank.write();
			return;
		}
	}
	
	// Check if it's a WAD.
	s32 header_size = stream.read<s32>(0);
	if(header_size < 0x10000) {
		stream.seek(0);
		std::vector<u8> header = stream.read_multiple<u8>(header_size);
		auto [game, type, name] = identify_wad(header);
		
		if(type != WadType::UNKNOWN) {
			Asset& root = bank.asset_file("wad.asset").root();
			
			BuildAsset& build = root.child<BuildAsset>("build");
			
			Asset* wad = nullptr;
			switch(type) {
				case WadType::ARMOR: wad = &build.armor<ArmorWadAsset>(); break;
				case WadType::AUDIO: wad = &build.audio<AudioWadAsset>(); break;
				case WadType::BONUS: wad = &build.bonus<BonusWadAsset>(); break;
				case WadType::GADGET: wad = &build.gadget<GadgetWadAsset>(); break;
				case WadType::HUD: wad = &build.hud<HudWadAsset>(); break;
				case WadType::MISC: wad = &build.misc<MiscWadAsset>(); break;
				case WadType::MPEG: wad = &build.mpeg<MpegWadAsset>(); break;
				case WadType::ONLINE: wad = &build.online<OnlineWadAsset>(); break;
				case WadType::SCENE: wad = &build.scene<SceneWadAsset>(); break;
				case WadType::SPACE: wad = &build.space<SpaceWadAsset>(); break;
				case WadType::LEVEL: wad = &build.levels().child<LevelAsset>("level").level<LevelWadAsset>(); break;
				case WadType::LEVEL_AUDIO: wad = &build.levels().child<LevelAsset>("level").audio<LevelWadAsset>(); break;
				case WadType::LEVEL_SCENE: wad = &build.levels().child<LevelAsset>("level").scene<LevelWadAsset>(); break;
			}
			
			g_asset_unpacker.input_file = &stream;
			g_asset_unpacker.current_file_offset = 0;
			g_asset_unpacker.total_file_size = stream.size();
			
			unpack_asset_impl(*wad, stream, game);
			
			bank.game_info.format_version = ASSET_FORMAT_VERSION;
			bank.game_info.builds = {build.reference()};
			
			printf("[100%%] Done!\n");
			
			bank.write();
			return;
		}
	}
	
	verify_not_reached("Unable to detect type of input file '%s'!", input_path.string().c_str());
}

static void pack(const std::vector<fs::path>& input_paths, const std::string& asset, const fs::path& output_path, Game game) {
	printf("[  0%%] Mounting asset banks\n");
	
	AssetForest forest;
	
	for(const fs::path& input_path : input_paths) {
		forest.mount<LooseAssetBank>(input_path, false);
	}
	
	Asset& wad = forest.lookup_asset(parse_asset_reference(asset.c_str()), nullptr);
	
	printf("[  0%%] Scanning dependencies of %s\n", asset.c_str());
	
	// Find the number of assets we need to pack. This is used for estimating
	// the completion percentage.
	BlackHoleOutputStream dummy;
	g_asset_packer_max_assets_processed = 0;
	g_asset_packer_num_assets_processed = 0;
	g_asset_packer_dry_run = true;
	pack_asset_impl(dummy, nullptr, nullptr, wad, game);
	g_asset_packer_max_assets_processed = g_asset_packer_num_assets_processed;
	g_asset_packer_num_assets_processed = 0;
	g_asset_packer_dry_run = false;
	
	FileOutputStream iso;
	verify(iso.open(output_path), "Failed to open '%s' for writing.\n", output_path.string().c_str());
	
	pack_asset_impl(iso, nullptr, nullptr, wad, game);
	
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
	compress_wad(compressed_bytes, bytes, nullptr, 8);
	
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
	puts("Wrench Build Tool -- https://github.com/chaoticgd/wrench");
	puts("");
	puts(" An asset packer/unpacker for the Ratchet & Clank PS2 games intended for modding.");
	puts("");
	puts("USER SUBCOMMANDS");
	puts("");
	puts(" unpack <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of source files.");
	puts("");
	puts(" pack <input asset banks> -a <asset> -o <output iso> -g <game>");
	puts("   Pack an asset (e.g. base_game) to produce a built file (e.g. an ISO file).");
	puts("   If <asset> is not a build, the game must be specified (rac, gc, uya or dl).");
	puts("");
	puts(" help | -h | --help");
	puts("    Print out this usage text.");
	puts("");
	puts(" version | -v | --version");
	puts("    Print out version information.");
	puts("");
	puts("DEVELOPER SUBCOMMANDS");
	puts("");
	puts(" unpack_globals <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of source global files.");
	puts("");
	puts(" unpack_levels <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of source level files.");
	puts("");
	puts(" unpack_wads <input files> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of WAD files.");
	puts("");
	puts(" unpack_global_wads <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of global WAD files.");
	puts("");
	puts(" unpack_level_wads <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of level WAD files.");
	puts("");
	puts(" unpack_binaries <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of binaries.");
	puts("");
	puts(" unpack_flat <input file> -o <output dir>");
	puts("   Unpack an ISO or WAD file to produce an asset bank of FlatWad assets.");
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
	puts(" profile_memory_usage <input asset banks>");
	puts("   Record statistics about the memory used by mounting asset banks.");
	puts("");
	puts(" test <asset bank>");
	puts("   Unpack and repack each binary in an asset bank, and diff against the original.");
}

extern unsigned char WAD_INFO[];

static void print_version() {
	BuildWadHeader* build = &((ToolWadInfo*) WAD_INFO)->build;
	if(build->version_major > -1 && build->version_major > -1) {
		printf("Wrench Build Tool v%hd.%hd\n", build->version_major, build->version_minor);
	} else {
		printf("Wrench Build Tool (Development Version)\n");
	}
	printf("Built from git commit ");
	for(s32 i = 0; i < ARRAY_SIZE(build->commit); i++) {
		printf("%hhx", build->commit[i]);
	}
	printf("\n");
}
