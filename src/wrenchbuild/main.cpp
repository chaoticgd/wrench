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

#include <cstdio>

#include <core/util.h>
#include <core/timer.h>
#include <core/stream.h>
#include <core/stdout_thread.h>
#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <assetmgr/zipped_asset_bank.h>
#include <instancemgr/instance.h>
#include <engine/tfrag_high.h>
#include <engine/moby_low.h>
#include <engine/tie.h>
#include <engine/shrub.h>
#include <engine/collision.h>
#include <iso/iso_packer.h>
#include <iso/iso_unpacker.h>
#include <iso/iso_tools.h>
#include <iso/iso_filesystem.h>
#include <iso/wad_identifier.h>
#include <toolwads/wads.h>
#include <wrenchbuild/tests.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/release.h>

enum ArgFlags : u32 {
	ARG_INPUT_PATH = 1 << 0,
	ARG_INPUT_PATHS = 1 << 1,
	ARG_ASSET = 1 << 2,
	ARG_OUTPUT_PATH = 1 << 3,
	ARG_OFFSET = 1 << 4,
	ARG_GAME = 1 << 5,
	ARG_REGION = 1 << 6,
	ARG_HINT = 1 << 7,
	ARG_SUBDIRECTORY = 1 << 8,
	ARG_DEVELOPER = 1 << 9,
	ARG_ASSET_OPTIONAL = 1 << 10,
	ARG_FILTER = 1 << 11
};

struct ParsedArgs {
	std::vector<fs::path> input_paths;
	std::string asset;
	fs::path output_path;
	s64 offset = 0;
	Game game = Game::UNKNOWN;
	Region region = Region::UNKNOWN;
	std::string hint;
	bool generate_output_subdirectory = false;
	bool print_developer_output = false;
	std::string filter;
};

static int wrenchbuild(int argc, char** argv);
static ParsedArgs parse_args(int argc, char** argv, u32 flags);
static void unpack(
	const fs::path& input_path,
	const fs::path& output_path,
	Game game,
	Region region,
	bool generate_output_subdirectory,
	const char* underlay_path);
static void pack(
	const std::vector<fs::path>& input_paths,
	const std::string& asset,
	const fs::path& output_path,
	BuildConfig config,
	const std::string& hint,
	const char* underlay_path);
static void decompress(const fs::path& input_path, const fs::path& output_path, s64 offset);
static void compress(const fs::path& input_path, const fs::path& output_path);
static void extract_tfrags(const fs::path& input_path, const fs::path& output_path, Game game);
static void extract_moby(const fs::path& input_path, const fs::path& output_path, Game game);
static void extract_mesh_only_moby(const fs::path& input_path, const fs::path& output_path, Game game);
static void extract_tie(const fs::path& input_path, const fs::path& output_path, Game game);
static void extract_shrub(const fs::path& input_path, const fs::path& output_path);
static void unpack_collision(const fs::path& input_path, const fs::path& output_path);
static void print_usage(bool developer_subcommands);
static void print_version();

#define require_args(arg_count) verify(argc == arg_count, "Incorrect number of arguments.");

int main(int argc, char** argv)
{
	try {
		int exit_code = wrenchbuild(argc, argv);
		stop_stdout_flusher_thread();
		return exit_code;
	} catch(RuntimeError& e) {
		e.print();
		stop_stdout_flusher_thread();
		return 1;
	} catch(...) {
		stop_stdout_flusher_thread();
		throw;
	}
}

static int wrenchbuild(int argc, char** argv)
{
	if(argc < 2) {
		print_usage(false);
		return 1;
	}
	
	WadPaths wads = find_wads(argv[0]);
	
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
		} else if(continuation == "_collision") {
			ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
			unpack_collision(args.input_paths[0], args.output_path);
			return 0;
		} else if(!continuation.empty()) {
			print_usage(false);
			return 1;
		}
		
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH | ARG_GAME | ARG_REGION | ARG_SUBDIRECTORY);
		unpack(args.input_paths[0], args.output_path, args.game, args.region, args.generate_output_subdirectory, wads.underlay.c_str());
		report_memory_statistics();
		return 0;
	}
	
	if(mode == "pack") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATHS | ARG_ASSET | ARG_OUTPUT_PATH | ARG_GAME | ARG_REGION | ARG_HINT);
		pack(args.input_paths, args.asset, args.output_path, BuildConfig(args.game, args.region), args.hint, wads.underlay.c_str());
		report_memory_statistics();
		return 0;
	}
	
	if(mode == "help" || mode == "-h" || mode == "--help") {
		ParsedArgs args = parse_args(argc, argv, ARG_DEVELOPER);
		print_usage(args.print_developer_output);
		return 0;
	}
	
	if(mode == "test") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_ASSET_OPTIONAL | ARG_FILTER);
		run_tests(args.input_paths[0], args.asset, args.filter);
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
		inspect_iso(args.input_paths[0].string());
		return 0;
	}
	
	if(mode == "parse_pcsx2_cdvd_log") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH);
		parse_pcsx2_cdvd_log(args.input_paths[0].string());
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
	
	if(mode == "extract_moby") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH | ARG_GAME);
		extract_moby(args.input_paths[0], args.output_path, args.game);
		return 0;
	}
	
	if(mode == "extract_mesh_only_moby") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH | ARG_GAME);
		extract_mesh_only_moby(args.input_paths[0], args.output_path, args.game);
		return 0;
	}
	
	if(mode == "extract_tie") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH | ARG_GAME);
		extract_tie(args.input_paths[0], args.output_path, args.game);
		return 0;
	}
	
	if(mode == "extract_shrub") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH);
		extract_shrub(args.input_paths[0], args.output_path);
		return 0;
	}
	
	if(mode == "extract_tfrags") {
		ParsedArgs args = parse_args(argc, argv, ARG_INPUT_PATH | ARG_OUTPUT_PATH | ARG_GAME);
		extract_tfrags(args.input_paths[0], args.output_path, args.game);
		return 0;
	}
	
	print_usage(false);
	return 1;
}

static ParsedArgs parse_args(int argc, char** argv, u32 flags)
{
	ParsedArgs args;
	
	for(int i = 2; i < argc; i++) {
		if((flags & (ARG_ASSET | ARG_ASSET_OPTIONAL)) && strcmp(argv[i], "-a") == 0) {
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
			std::string game = argv[++i];
			args.game = game_from_string(game);
			continue;
		}
		
		if((flags & ARG_REGION) && strcmp(argv[i], "-r") == 0) {
			verify(i + 1 < argc, "Expected region argument.");
			std::string region = argv[++i];
			args.region = region_from_string(region);
			continue;
		}
		
		if((flags & ARG_HINT) && strcmp(argv[i], "-h") == 0) {
			verify(i + 1 < argc, "Expected hint argument.");
			args.hint = argv[++i];
			continue;
		}
		
		if((flags & ARG_SUBDIRECTORY) && strcmp(argv[i], "-s") == 0) {
			args.generate_output_subdirectory = true;
			continue;
		}
		
		if((flags & ARG_DEVELOPER) && strcmp(argv[i], "-d") == 0) {
			args.print_developer_output = true;
			continue;
		}
		
		if((flags & ARG_FILTER) && strcmp(argv[i], "-f") == 0) {
			verify(i + 1 < argc, "Expected filter argument.");
			args.filter = argv[++i];
			continue;
		}
		
		if(strcmp(argv[i], "--flusher-thread-hack") == 0) {
			start_stdout_flusher_thread();
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
	verify(((flags & ARG_ASSET_OPTIONAL) != 0) || ((flags & ARG_ASSET) == 0) || !args.asset.empty(), "Asset reference (-a) not specified.");
	verify(((flags & ARG_OUTPUT_PATH) == 0) || !args.output_path.empty(), "Output path (-o) not specified.");
	
	return args;
}

static void unpack(
	const fs::path& input_path,
	const fs::path& output_path,
	Game game, Region region,
	bool generate_output_subdirectory,
	const char* underlay_path)
{
	AssetForest forest;
	
	FileInputStream stream;
	verify(stream.open(input_path), "Failed to open input file '%s' for reading (%s).",
		input_path.string().c_str(), stream.last_error.c_str());
	
	// Check if it's an ISO file.
	if(stream.size() > 16 * SECTOR_SIZE + 6) {
		stream.seek(16 * SECTOR_SIZE + 1);
		std::vector<char> identifier = stream.read_multiple<char>(5);
		
		if(memcmp(identifier.data(), "CD001", 5) == 0) {
			IsoFilesystem fs = read_iso_filesystem(stream);
			Release release = identify_release(fs.root, stream);
			
			std::string game_str = game_to_string(release.game);
			std::string region_str = region_to_string(release.region);
			
			// If -s is passed we create a new subdirectory based on the elf
			// name for the output files.
			fs::path new_output_path = output_path;
			if(generate_output_subdirectory) {
				std::string name = game_str + "_" + release.elf_name;
				for(char& c : name) {
					if(c == '.') {
						c = '_';
					}
				}
				new_output_path = new_output_path/fs::path(name);
			}
			
			// Mount the underlay, which contains metadata to be used to name
			// files and directories while unpacking.
			forest.mount<LooseAssetBank>(underlay_path, false);
			
			AssetBank& bank = forest.mount<LooseAssetBank>(new_output_path, true);
			bank.game_info.type = g_asset_unpacker.dump_binaries ? AssetBankType::TEST : AssetBankType::GAME;
			bank.game_info.game.game = release.game;
			
			BuildAsset& build = bank.asset_file("build.asset").root().child<BuildAsset>(game_str.c_str());
			
			g_asset_unpacker.input_file = &stream;
			g_asset_unpacker.current_file_offset = 0;
			g_asset_unpacker.total_file_size = stream.size();
			
			BuildConfig config(release.game, release.region);
			unpack_asset_impl(build, stream, nullptr, config);
			
			bank.game_info.name = release.name;
			bank.game_info.format_version = ASSET_FORMAT_VERSION;
			bank.game_info.builds = {build.absolute_link().to_string()};
			
			printf("[100%%] Done!\n");
			
			bank.write();
			forest.write_source_files(bank, bank.game_info.game.game);
			
			return;
		}
	}
	
	// Check if it's a WAD.
	s32 header_size = stream.read<s32>(0);
	if(header_size < 0x10000) {
		stream.seek(0);
		std::vector<u8> header = stream.read_multiple<u8>(header_size);
		auto [detected_game, type, name] = identify_wad(header);
		
		if(game == Game::UNKNOWN) {
			game = detected_game;
		}
		
		if(type != WadType::UNKNOWN) {
			// Mount the underlay, which contains metadata to be used to name
			// files and directories while unpacking.
			forest.mount<LooseAssetBank>(underlay_path, false);
			
			AssetBank& bank = forest.mount<LooseAssetBank>(output_path, true);
			bank.game_info.type = g_asset_unpacker.dump_binaries ? AssetBankType::TEST : AssetBankType::GAME;
			bank.game_info.game.game = game;
			
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
				default: verify_fatal("Invalid WAD type.");
			}
			
			g_asset_unpacker.input_file = &stream;
			g_asset_unpacker.current_file_offset = 0;
			g_asset_unpacker.total_file_size = stream.size();
			
			unpack_asset_impl(*wad, stream, &header, BuildConfig(game, region));
			
			bank.game_info.format_version = ASSET_FORMAT_VERSION;
			bank.game_info.builds = {build.absolute_link().to_string()};
			
			printf("[100%%] Done!\n");
			
			bank.write();
			forest.write_source_files(bank, bank.game_info.game.game);
			
			return;
		}
	}
	
	verify_not_reached("Unable to detect type of input file '%s'!", input_path.string().c_str());
}

static void pack(
	const std::vector<fs::path>& input_paths,
	const std::string& asset,
	const fs::path& output_path,
	BuildConfig config,
	const std::string& hint,
	const char* underlay_path)
{
	printf("[  0%%] Mounting asset banks\n");
	
	AssetForest forest;
	
	// Load the underlay, and mark all underlay assets as weakly deleted so they
	// don't show up if the asset isn't actually present.
	forest.mount<LooseAssetBank>(underlay_path, false);
	forest.any_root()->for_each_logical_descendant([&](Asset& asset) {
		// If the asset has strongly_deleted set to false, interpret that to
		// mean the asset should shouldn't be weakly deleted.
		if((asset.flags & ASSET_HAS_STRONGLY_DELETED_FLAG) == 0 || (asset.flags & ASSET_IS_STRONGLY_DELETED) != 0) {
			asset.flags |= ASSET_IS_WEAKLY_DELETED;
		}
	});
	
	for(const fs::path& input_path : input_paths) {
		if(fs::is_directory(input_path)) {
			forest.mount<LooseAssetBank>(input_path, false);
		} else if(input_path.extension() == ".zip") {
			forest.mount<ZippedAssetBank>(input_path.string().c_str(), fs::path());
		} else {
			verify_not_reached("An input path points to neither a directory nor a zip file.");
		}
	}
	
	AssetLink link;
	link.set(asset.c_str());
	Asset& wad = forest.lookup_asset(link, nullptr);
	
	Game game;
	if(BuildAsset* build = wad.maybe_as<BuildAsset>()) {
		game = game_from_string(build->game());
	} else {
		verify(config.game() != Game::UNKNOWN, "Must specify -g on the command line.");
		game = config.game();
	}
	
	// Parse pvar types.
	forest.read_source_files(game);
	
	printf("[  0%%] Scanning dependencies of %s\n", asset.c_str());
	
	// Find the number of assets we need to pack. This is used for estimating
	// the completion percentage.
	BlackHoleOutputStream dummy;
	g_asset_packer_max_assets_processed = 0;
	g_asset_packer_num_assets_processed = 0;
	g_asset_packer_dry_run = true;
	pack_asset_impl(dummy, nullptr, nullptr, wad, config, hint.c_str());
	g_asset_packer_max_assets_processed = g_asset_packer_num_assets_processed;
	g_asset_packer_num_assets_processed = 0;
	g_asset_packer_dry_run = false;
	
	FileOutputStream iso;
	verify(iso.open(output_path), "Failed to open '%s' for writing.\n", output_path.string().c_str());
	
	pack_asset_impl(iso, nullptr, nullptr, wad, config, hint.c_str());
	
	printf("[100%%] Done!\n");
}

static void decompress(const fs::path& input_path, const fs::path& output_path, s64 offset)
{
	WrenchFileHandle* file = file_open(input_path.string().c_str(), WRENCH_FILE_MODE_READ);
	verify(file, "Failed to open file '%s' for reading (%s).", input_path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
	
	std::vector<u8> header = read_file(file, offset, 0x10);
	verify(header[0] == 'W' && header[1] == 'A' && header[2] == 'D',
		"Invalid WAD header (magic bytes aren't correct).");
	s32 compressed_size = Buffer(header).read<s32>(3, "compressed size");
	std::vector<u8> compressed_bytes = read_file(file, offset, compressed_size);
	
	std::vector<u8> decompressed_bytes;
	decompress_wad(decompressed_bytes, compressed_bytes);
	
	write_file(output_path, decompressed_bytes);
}

static void compress(const fs::path& input_path, const fs::path& output_path)
{
	std::vector<u8> bytes = read_file(input_path);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, nullptr, 8);
	
	write_file(output_path, compressed_bytes);
}

static void extract_tfrags(const fs::path& input_path, const fs::path& output_path, Game game)
{
	auto bin = read_file(input_path.string().c_str());
	Tfrags tfrags = read_tfrags(bin, game);
	ColladaScene scene = recover_tfrags(tfrags, TFRAG_NO_FLAGS);
	auto xml = write_collada(scene);
	write_file(output_path, xml, true);
}

static void extract_moby(const fs::path& input_path, const fs::path& output_path, Game game)
{
	auto bin = read_file(input_path.string().c_str());
	MOBY::MobyClassData moby = MOBY::read_class(bin, game);
	
	std::vector<GLTF::Mesh> packets = MOBY::recover_packets(moby.mesh.high_lod, -1, moby.scale, moby.animation.joints.size() > 0);
	GLTF::Mesh mesh = MOBY::merge_packets(packets, "high_lod");
	
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	
	for(s32 i = 0; i < 16; i++) {
		gltf.materials.emplace_back();
	}
	
	scene->nodes.emplace_back((s32) gltf.nodes.size());
	GLTF::Node& node = gltf.nodes.emplace_back();
	
	node.mesh = (s32) gltf.meshes.size();
	gltf.meshes.emplace_back(std::move(mesh));
	
	auto glb = GLTF::write_glb(gltf);
	write_file(output_path, glb, false);
}

static void extract_mesh_only_moby(const fs::path& input_path, const fs::path& output_path, Game game)
{
	auto bin = read_file(input_path.string().c_str());
	MOBY::MobyMeshSection moby = MOBY::read_mesh_only_class(bin, game);
	
	std::vector<GLTF::Mesh> packets = MOBY::recover_packets(moby.high_lod, -1, 1.f, true);
	GLTF::Mesh mesh = MOBY::merge_packets(packets, "high_lod");
	
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	
	for(s32 i = 0; i < 16; i++) {
		gltf.materials.emplace_back();
	}
	
	scene->nodes.emplace_back((s32) gltf.nodes.size());
	GLTF::Node& node = gltf.nodes.emplace_back();
	
	node.mesh = (s32) gltf.meshes.size();
	gltf.meshes.emplace_back(std::move(mesh));
	
	auto glb = GLTF::write_glb(gltf);
	write_file(output_path, glb, false);
}

static void extract_tie(const fs::path& input_path, const fs::path& output_path, Game game)
{
	auto bin = read_file(input_path.string().c_str());
	TieClass tie = read_tie_class(bin, game);
	ColladaScene scene = recover_tie_class(tie);
	auto xml = write_collada(scene);
	write_file(output_path, xml, true);
}

static void extract_shrub(const fs::path& input_path, const fs::path& output_path)
{
	auto bin = read_file(input_path.string().c_str());
	ShrubClass shrub = read_shrub_class(bin);
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	scene->nodes.emplace_back((s32) gltf.nodes.size());
	GLTF::Node& node = gltf.nodes.emplace_back();
	node.mesh = (s32) gltf.meshes.size();
	gltf.meshes.emplace_back(recover_shrub_class(shrub));
	auto glb = GLTF::write_glb(gltf);
	write_file(output_path, glb, false);
}

static void unpack_collision(const fs::path& input_path, const fs::path& output_path)
{
	AssetForest forest;
	AssetBank& bank = forest.mount<LooseAssetBank>(output_path, true);
	CollisionAsset& collision = bank.asset_file("collision.asset").root().child<CollisionAsset>("collision");
	
	FileInputStream stream;
	verify(stream.open(input_path), "Cannot open input file.");
	
	BuildConfig config(Game::RAC, Region::US); // Don't care.
	unpack_asset_impl(collision, stream, nullptr, config);
	
	bank.write();
}

static void print_usage(bool developer_subcommands)
{
	puts("Wrench Build Tool -- https://github.com/chaoticgd/wrench");
	puts("");
	puts(" An asset packer/unpacker for the Ratchet & Clank PS2 games intended for modding.");
	puts("");
	puts("User Subcommands");
	puts("");
	puts(" unpack <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
	puts("   Unpack an ISO or WAD file to produce an asset bank of source files.");
	puts("   If the file to be unpacked is a WAD, the game (rac, gc, uya or dl) should be");
	puts("   specified and the region (us, eu or japan) must be specified.");
	puts("   Optionally, files unpacked from an ISO can be placed in a subdirectory with a");
	puts("   name based on the identified release of said ISO by passing -s.");
	puts("");
	puts(" pack <input asset banks> -a <asset link> -o <output iso> [-h <hint>] [-g <game>] [-r <region>]");
	puts("   Pack an asset (e.g. a build) to produce a built file (e.g. an ISO file).");
	puts("   If <asset link> is not a build, the game (rac, gc, uya or dl) and the region");
	puts("   (us, eu or japan) must be specified.");
	puts("   A hint string used to specify the format of the asset can be set by");
	puts("   passing -h, which is required for certain types of assets such as builds.");
	puts("   These strings are documented in the asset reference.");
	puts("");
	puts(" help | -h | --help [-d]");
	puts("   Print out this usage text. Pass -d to list developer subcommands.");
	puts("");
	puts(" version | -v | --version");
	puts("   Print out version information.");
	if(developer_subcommands) {
		puts("");
		puts("Developer Subcommands");
		puts("");
		puts(" unpack_globals <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of source global files.");
		puts("");
		puts(" unpack_levels <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of source level files.");
		puts("");
		puts(" unpack_wads <input files> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of WAD files.");
		puts("");
		puts(" unpack_global_wads <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of global WAD files.");
		puts("");
		puts(" unpack_level_wads <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of level WAD files.");
		puts("");
		puts(" unpack_binaries <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of binaries.");
		puts("");
		puts(" unpack_flat <input file> -o <output dir> [-g <game>] [-r <region>] [-s]");
		puts("   Unpack an ISO or WAD file to produce an asset bank of FlatWad assets.");
		puts("");
		puts(" unpack_collision <input file> -o <output dir>");
		puts("   Unpack a built collision file to produce an asset bank containing a single");
		puts("   collision asset and the corresponding mesh.");
		puts("");
		puts(" test <input asset bank> [-a <asset link>] [-f <filter string>]");
		puts("   Unpack and repack binaries from the asset bank, and diff them against the");
		puts("   originals. If -a is passed, only test the single specified binary and print");
		puts("   a hex dump, otherwise test all the binaries in the bank without the hex dump.");
		puts("   Use the unpack_binaries subcommand can produce the input asset bank.");
		puts("   If -f is passed, only tests on assets which contain the filter string in");
		puts("   their absolute asset link string will be run.");
		puts("");
		puts(" decompress <input file> -o <output file> [-x <offset>]");
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
		puts(" extract_tfrags <input path> -o <output path>");
		puts("   Convert packed tfrags to a .dae file.");
		puts("");
		puts(" extract_moby <input path> -o <output path> -g <game>");
		puts("   Convert a packed moby to a .glb file.");
		puts("");
		puts(" extract_mesh_only_moby <input path> -o <output path> -g <game>");
		puts("   Convert a packed moby to a .glb file.");
		puts("");
		puts(" extract_tie <input path> -o <output path>");
		puts("   Convert a packed tie to a .dae file.");
		puts("");
		puts(" extract_shrub <input path> -o <output path>");
		puts("   Convert a packed shrub to a .glb file.");
	}
}

static void print_version()
{
	if(strlen(wadinfo.build.version_string) != 0) {
		printf("Wrench Build Tool %s\n", wadinfo.build.version_string);
	} else {
		printf("Wrench Build Tool (Development Version)\n");
	}
	printf("Built from git commit %s", wadinfo.build.commit_string);
	printf("\n");
}

// If you're hitting this assert it means the asset schema is out of sync with
// the instance schema. The version numbers should be the same.
static_assert(ASSET_FORMAT_VERSION == INSTANCE_FORMAT_VERSION);
