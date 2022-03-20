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
#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/moby.h>
#include <engine/collision.h>
#include <spanner/tests.h>
#include <spanner/wad_file.h>
#include <spanner/globals/armor_wad.h>
#include <spanner/globals/audio_wad.h>
#include <spanner/globals/bonus_wad.h>
#include <spanner/globals/hud_wad.h>
#include <spanner/globals/misc_wad.h>
#include <spanner/globals/mpeg_wad.h>
#include <spanner/globals/online_wad.h>
#include <spanner/globals/space_wad.h>

static void unpack(const char* input_path, const char* output_path);
static void extract(fs::path input_path, fs::path output_path);
static void build(fs::path input_path, fs::path output_path);
static void extract_collision(fs::path input_path, fs::path output_path);
static void build_collision(fs::path input_path, fs::path output_path);
static void extract_moby(const char* input_path, const char* output_path);
static void build_moby(const char* input_path, const char* output_path);
static void print_usage(char* argv0);

#define require_args(arg_count) verify(argc == arg_count, "Incorrect number of arguments.");

int main(int argc, char** argv) {
	if(argc < 2) {
		print_usage(argv[0]);
		return 1;
	}
	
	std::string mode = argv[1];
	
	if(mode == "unpack") {
		require_args(4);
		unpack(argv[2], argv[3]);
	} else if(mode == "extract") {
		require_args(4);
		extract(argv[2], argv[3]);
	} else if(mode == "build") {
		require_args(4);
		build(argv[2], argv[3]);
	} else if(mode == "extract_collision") {
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
		print_usage(argv[0]);
		return 1;
	}
	return 0;
}

static void unpack(const char* input_path, const char* output_path) {
	AssetForest forest;
	
	AssetPack& src_pack = forest.mount<LooseAssetPack>("extracted", input_path, false);
	verify(!src_pack.game_info.game.empty(),
		"Source asset pack has invalid 'game' attribute in gameinfo.txt.");
	verify(src_pack.game_info.type == AssetPackType::EXTRACTED,
		"Unpacking can only be done on an extracted asset pack.");
	
	AssetPack& dest_pack = forest.mount<LooseAssetPack>("unpacked", output_path, true);
	dest_pack.game_info.game = src_pack.game_info.game;
	dest_pack.game_info.type = AssetPackType::UNPACKED;
	
	std::string game_ref = stringf("/Game:%s", src_pack.game_info.game.c_str());
	GameAsset* game = dynamic_cast<GameAsset*>(forest.lookup_asset(parse_asset_reference(game_ref.c_str())));
	verify(game, "Invalid Game asset '%s'.", game_ref.c_str());
	std::vector<Asset*> builds = game->builds();
	verify(builds.size() == 1, "Extracted asset pack must have exactly one build.");
	BuildAsset* build = dynamic_cast<BuildAsset*>(builds[0]);
	verify(build, "Invalid Build asset.");
	
	BinaryAsset* armor_wad = dynamic_cast<BinaryAsset*>(build->armor());
	verify(armor_wad, "Invalid armor.wad asset.");
	unpack_armor_wad(dest_pack, *armor_wad);
	
	BinaryAsset* audio_wad = dynamic_cast<BinaryAsset*>(build->audio());
	verify(audio_wad, "Invalid audio.wad asset.");
	unpack_audio_wad(dest_pack, *audio_wad);
	
	BinaryAsset* bonus_wad = dynamic_cast<BinaryAsset*>(build->bonus());
	verify(bonus_wad, "Invalid bonus.wad asset.");
	unpack_bonus_wad(dest_pack, *bonus_wad);
	
	BinaryAsset* hud_wad = dynamic_cast<BinaryAsset*>(build->hud());
	verify(hud_wad, "Invalid hud.wad asset.");
	unpack_hud_wad(dest_pack, *hud_wad);
	
	BinaryAsset* misc_wad = dynamic_cast<BinaryAsset*>(build->misc());
	verify(misc_wad, "Invalid misc.wad asset.");
	unpack_misc_wad(dest_pack, *misc_wad);
	
	BinaryAsset* mpeg_wad = dynamic_cast<BinaryAsset*>(build->mpeg());
	verify(mpeg_wad, "Invalid mpeg.wad asset.");
	unpack_mpeg_wad(dest_pack, *mpeg_wad);
	
	BinaryAsset* online_wad = dynamic_cast<BinaryAsset*>(build->online());
	verify(online_wad, "Invalid online.wad asset.");
	unpack_online_wad(dest_pack, *online_wad);
	
	BinaryAsset* space_wad = dynamic_cast<BinaryAsset*>(build->space());
	verify(space_wad, "Invalid space.wad asset.");
	unpack_space_wad(dest_pack, *space_wad);
	
	dest_pack.write();
}

static void extract(fs::path input_path, fs::path output_path) {
	start_timer("Extract WAD");
	FILE* wad_file = fopen(input_path.string().c_str(), "rb");
	verify(wad_file, "Failed to open input file.");
	std::unique_ptr<Wad> wad = read_wad(wad_file);
	write_wad_json(output_path, wad.get());
	fclose(wad_file);
	stop_timer();
}

static void build(fs::path input_path, fs::path output_path) {
	start_timer("Building WAD");
	start_timer("Collecting files");
	std::unique_ptr<Wad> wad = read_wad_json(input_path);
	stop_timer();
	FILE* wad_file = fopen(output_path.string().c_str(), "wb");
	verify(wad_file, "Failed to open output file for writing.");
	write_wad(wad_file, wad.get());
	fclose(wad_file);
	stop_timer();
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

static void print_usage(char* argv0) {
	printf("~* The Wrench WAD Utility *~\n");
	printf("\n");
	printf("An asset packer/unpacker for the Ratchet & Clank PS2 games intended for modding.\n");
	printf("\n");
	printf("usage: \n");
	printf("  %s extract <input wad> <output dir>\n", argv0);
	printf("  %s build <input level json> <output wad>\n", argv0);
	printf("  %s extract_collision <input bin> <output dae>\n", argv0);
	printf("  %s build_collision <input dae> <output bin>\n", argv0);
	printf("  %s test <level wads dir>\n", argv0);
}
