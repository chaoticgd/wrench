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
#include <engine/texture.h>
#include <engine/collision.h>
#include <spanner/assets.h>
#include <spanner/primary.h>

static void run_level_tests(fs::path input_path);
struct GameplayTestArgs {
	std::string wad_file_path;
	FILE* file;
	SectorRange lump = {0, 0};
	const char* name;
	const std::vector<GameplayBlockDescription>* blocks;
	bool compressed;
	Game game;
};
static void run_gameplay_lump_test(GameplayTestArgs args);
static void run_moby_class_test(s32 o_class, Buffer src, const char* file_path, Game game);
static void run_ratchet_seqs_test(Buffer table, Buffer assets, const char* file_path, Game game, const std::vector<s64>& block_bounds);
static void assert_collada_scenes_equal(const ColladaScene& lhs, const ColladaScene& rhs);

void run_tests(fs::path input_path) {
	run_level_tests(input_path);
	
	printf("\nALL TESTS HAPPY\n");
}

static void run_level_tests(fs::path input_path) {
	for(fs::path wad_file_path : fs::directory_iterator(input_path)) {
		FILE* file = fopen(wad_file_path.string().c_str(), "rb");
		verify(file, "Failed to open input file.");
		
		auto build_args = [&](SectorRange lump, const char* name, const std::vector<GameplayBlockDescription>& blocks, bool compressed, Game game) {
			GameplayTestArgs args;
			args.wad_file_path = wad_file_path.string();
			args.file = file;
			args.lump = lump;
			args.name = name;
			args.blocks = &blocks;
			args.compressed = compressed;
			args.game = game;
			return args;
		};
		
		s32 header_size;
		verify(fread(&header_size, 4, 1, file) == 1, "Failed to read WAD header.");
		
		std::string file_path = wad_file_path.string().c_str();
		
		Game game;
		std::vector<u8> primary;
		switch(header_size) {
			case sizeof(Rac1LevelWadHeader): {
				auto header = read_header<Rac1LevelWadHeader>(file);
				primary = read_lump(file, header.primary, "primary");
				game = Game::RAC1;
				run_gameplay_lump_test(build_args(header.gameplay_ntsc, "gameplay NTSC", RAC1_GAMEPLAY_BLOCKS, true, Game::RAC1));
				break;
			}
			case sizeof(Rac23LevelWadHeader): {
				auto header = read_header<Rac23LevelWadHeader>(file);
				primary = read_lump(file, header.primary, "primary");
				game = detect_game_rac23(primary);
				run_gameplay_lump_test(build_args(header.gameplay, "gameplay", RAC23_GAMEPLAY_BLOCKS, true, game));
				break;
			}
			case sizeof(DeadlockedLevelWadHeader): {
				auto header = read_header<DeadlockedLevelWadHeader>(file);
				primary = read_lump(file, header.primary, "primary");
				game = Game::DL;
				run_gameplay_lump_test(build_args(header.gameplay_core, "gameplay core", DL_GAMEPLAY_CORE_BLOCKS, true, Game::DL));
				run_gameplay_lump_test(build_args(header.art_instances, "art instances", DL_ART_INSTANCE_BLOCKS, true, Game::DL));
				for(s32 i = 0; i < 128; i++) {
					std::string name = "mission instances " + std::to_string(i);
					run_gameplay_lump_test(build_args(header.gameplay_mission_instances[i], name.c_str(), DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS, false, Game::DL));
				}
				break;
			}
			default:
				verify_not_reached("Unable to identify '%s'.", file_path.c_str());
		}
		
		PrimaryHeader primary_header = read_primary_header(Buffer(primary), game);
		Buffer asset_header_buf = Buffer(primary).subbuf(primary_header.asset_header.offset, primary_header.asset_header.size);
		
		Buffer assets_compressed = Buffer(primary).subbuf(primary_header.assets.offset, primary_header.assets.size);
		std::vector<u8> assets_vec;
		decompress_wad(assets_vec, WadBuffer(assets_compressed.lo, assets_compressed.hi));
		Buffer assets(assets_vec);
		
		AssetHeader asset_header = asset_header_buf.read<AssetHeader>(0, "asset header");
		auto block_bounds = enumerate_asset_block_boundaries(asset_header_buf, asset_header, game);
		
		printf("%s collision\n", wad_file_path.string().c_str());
		ColladaScene src_collision = read_collision(assets.subbuf(asset_header.collision));
		auto src_collision_xml = write_collada(src_collision);
		auto dest_collision = read_collada(src_collision_xml);
		assert_collada_scenes_equal(src_collision, dest_collision);
		
		auto moby_classes = asset_header_buf.read_multiple<MobyClassEntry>(asset_header.moby_classes, "moby class table");
		for(const MobyClassEntry& entry : moby_classes) {
			if(entry.offset_in_asset_wad != 0 && entry.o_class >= 10) {
				s64 size = next_asset_block_size(entry.offset_in_asset_wad, block_bounds);
				run_moby_class_test(entry.o_class, assets.subbuf(entry.offset_in_asset_wad, size), file_path.c_str(), game);
			}
		}
		
		if(asset_header.ratchet_seqs_rac123 != 0 && game != Game::DL) {
			run_ratchet_seqs_test(asset_header_buf.subbuf(asset_header.ratchet_seqs_rac123), assets, file_path.c_str(), game, block_bounds);
		}
	}
}

static void run_gameplay_lump_test(GameplayTestArgs args) {
	printf("%s %s\n", args.wad_file_path.c_str(), args.name);
	
	if(args.lump.offset.sectors == 0) {
		return;
	}
	std::vector<u8> raw = read_lump(args.file, args.lump, args.name);
	std::vector<u8> src;
	if(args.compressed) {
		verify(decompress_wad(src, raw), "Decompressing %s file failed.", args.name);
	} else {
		src = std::move(raw);
	}
	if(args.blocks == &DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS && src.size() >= 4 && *(s32*) src.data() == 0x90) {
		printf("warning: Skipping %s %s. Weird format.\n", args.wad_file_path.c_str(), args.name);
		return;
	}
	
	// Test the binary reading/writing functions.
	LevelWad wad;
	Gameplay gameplay;
	read_gameplay(wad, gameplay, src, args.game, *args.blocks);
	std::vector<u8> dest = write_gameplay(wad, gameplay, args.game, *args.blocks);
	
	// The input buffer may or may not already be padded to the sector size.
	OutBuffer(dest).pad(SECTOR_SIZE, 0);
	OutBuffer(src).pad(SECTOR_SIZE, 0);
	
	Buffer dest_buf(dest);
	Buffer src_buf(src);
	
	bool good = true;
	
	std::string prefix_str = args.wad_file_path + " " + args.name;
	std::string gameplay_header_str = prefix_str + " header";
	std::string gameplay_data_str = prefix_str + " data";
	good &= diff_buffers(src_buf.subbuf(0, 0xa0), dest_buf.subbuf(0, 0xa0), 0, gameplay_header_str.c_str(), 0);
	good &= diff_buffers(src_buf.subbuf(0xa0), dest_buf.subbuf(0xa0), 0xa0, gameplay_data_str.c_str(), 0xa0);
	
	if(!good) {
		FILE* gameplay_file = fopen("/tmp/gameplay.bin", "wb");
		verify(gameplay_file, "Failed to open /tmp/gameplay.bin for writing.");
		fwrite(src.data(), src.size(), 1, gameplay_file);
		fclose(gameplay_file);
		exit(1);
	}
	
	// Test the JSON reading/writing functions.
	Json gameplay_json = to_json(gameplay);
	HelpMessages help_messages;
	help_messages.swap(gameplay); // gameplay -> help_messages
	Json help_messages_json = to_json(help_messages);
	Gameplay test_gameplay;
	from_json(test_gameplay, gameplay_json);
	help_messages.swap(test_gameplay); // help_messages -> test_gameplay
	std::vector<u8> test_dest = write_gameplay(wad, test_gameplay, args.game, *args.blocks);
	OutBuffer(test_dest).pad(SECTOR_SIZE, 0);
	if(test_dest != dest) {
		fprintf(stderr, "File read from JSON doesn't match original.\n");
		write_file("/tmp/", "gameplay_orig.bin", dest);
		write_file("/tmp/", "gameplay_test.bin", test_dest);
		exit(1);
	}
}

static std::map<s32, std::array<u8, MD5_DIGEST_LENGTH>> moby_md5s;

static void run_moby_class_test(s32 o_class, Buffer src, const char* file_path, Game game) {
	ERROR_CONTEXT("running moby class %d test", o_class);
	printf("%s moby class %d\n", file_path, o_class);
	
	// These have weird joint data.
	if(game == Game::RAC3 && (o_class == 5952 || o_class == 5953)) {
		return;
	}
	
	// Test the binary reading/writing functions.
	MobyClassData moby = read_moby_class(src, game);
	std::vector<u8> dest_vec;
	// Make sure relative pointers are set correctly.
	for(s32 i = 0; i < 0x40; i++) {
		OutBuffer(dest_vec).write<u8>(0);
	}
	write_moby_class(dest_vec, moby, game);
	OutBuffer(dest_vec).pad(0x40);
	Buffer dest(dest_vec);
	
	const s32 header_size = 0x80; // This is wrong but makes the hex printout better.
	
	std::string prefix_str = std::string(file_path) + " moby class " + std::to_string(o_class);
	std::string header_str = prefix_str + " header";
	std::string data_str = prefix_str + " data";
	
	// The moby exporter currently isn't good enough to get meshes identically
	// written out, so we just test up until the submesh table.
	s64 test_size = src.size();
	if(moby.has_submesh_table) {
		test_size = src.read<s32>(0, "submesh table offset");
	}
	
	Buffer data_src = src.subbuf(header_size, test_size - header_size);
	Buffer data_dest = dest.subbuf(0x40 + header_size, test_size - header_size);
	
	bool good = true;
	//good &= diff_buffers(src.subbuf(0, header_size), dest.subbuf(0x40, header_size), 0, header_str.c_str(), 0);
	good &= diff_buffers(data_src, data_dest, 0, data_str.c_str(), header_size);
	
	if(!good) {
		FILE* file = fopen("/tmp/moby.bin", "wb");
		verify(file, "Failed to open /tmp/moby.bin for writing.");
		fwrite(src.lo, src.hi - src.lo, 1, file);
		fclose(file);
		exit(1);
	}
	
	// Test that mobies of the same class from different levels are equal.
	std::array<u8, MD5_DIGEST_LENGTH> md5;
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, dest_vec.data(), dest_vec.size());
	MD5Final(md5.data(), &ctx);
	auto md5_iter = moby_md5s.find(o_class);
	if(md5_iter != moby_md5s.end()) {
		verify(md5 == md5_iter->second, "Moby %d differs between levels.", o_class);
	} else {
		moby_md5s.emplace(o_class, md5);
	}
	
	// Test the COLLADA importer/exporter.
	ColladaScene src_scene = recover_moby_class(moby, o_class, 0);
	std::vector<u8> collada_xml = write_collada(src_scene);
	ColladaScene dest_scene = read_collada(std::move(collada_xml));
	assert_collada_scenes_equal(src_scene, dest_scene);
}

static void run_ratchet_seqs_test(Buffer table, Buffer assets, const char* file_path, Game game, const std::vector<s64>& block_bounds) {
	ERROR_CONTEXT("running ratchet sequences");
	printf("%s ratchet sequences\n", file_path);
	
	s32 ratchet_joint_count = 0x6f;
	
	auto ratchet_seqs = table.read_multiple<s32>(0, 256, "ratchet seqs");
	for(s32 i = 0; i < (s32) ratchet_seqs.size(); i++) {
		s32 ratchet_seq_ofs = ratchet_seqs[i];
		if(ratchet_seq_ofs == 0) {
			continue;
		}
		
		s64 seq_size = next_asset_block_size(ratchet_seq_ofs, block_bounds);
		Buffer src = assets.subbuf(ratchet_seq_ofs, seq_size);
		MobySequence seq = read_moby_sequence(src, 0, ratchet_joint_count, game);
		std::vector<u8> dest;
		write_moby_sequence(dest, seq, 0, ratchet_joint_count, game);
		OutBuffer(dest).pad(0x10);
		if(!diff_buffers(src, Buffer(dest), 0, "ratchet sequence", 0)) {
			exit(1);
		}
	}
}

static void assert_collada_scenes_equal(const ColladaScene& lhs, const ColladaScene& rhs) {
	assert(lhs.texture_paths.size() == rhs.texture_paths.size());
	assert(lhs.texture_paths == rhs.texture_paths);
	assert(lhs.materials.size() == rhs.materials.size());
	for(size_t i = 0; i < lhs.materials.size(); i++) {
		const Material& lmat = lhs.materials[i];
		const Material& rmat = rhs.materials[i];
		assert(lmat.name == rmat.name);
		assert(lmat.colour == rmat.colour);
		assert(lmat.texture == rmat.texture);
	}
	assert(lhs.meshes.size() == rhs.meshes.size());
	for(size_t i = 0; i < lhs.meshes.size(); i++) {
		const Mesh& lmesh = lhs.meshes[i];
		const Mesh& rmesh = rhs.meshes[i];
		assert(lmesh.name == rmesh.name);
		assert(lmesh.submeshes.size() == rmesh.submeshes.size());
		// If there are no submeshes, we can't recover the flags.
		assert(lmesh.flags == rmesh.flags || lmesh.submeshes.size() == 0);
		// The COLLADA importer/exporter doesn't preserve the layout of the
		// vertex buffer, so don't check that.
		for(size_t j = 0; j < lmesh.submeshes.size(); j++) {
			const SubMesh& lsub = lmesh.submeshes[j];
			const SubMesh& rsub = rmesh.submeshes[j];
			for(size_t k = 0; k < lsub.faces.size(); k++) {
				const Face& lface = lsub.faces[k];
				const Face& rface = rsub.faces[k];
				Vertex lverts[4] = {
					lmesh.vertices.at(lface.v0),
					lmesh.vertices.at(lface.v1),
					lmesh.vertices.at(lface.v2),
					Vertex(glm::vec3(0, 0, 0))
				};
				Vertex rverts[4] = {
					rmesh.vertices.at(rface.v0),
					rmesh.vertices.at(rface.v1),
					rmesh.vertices.at(rface.v2),
					Vertex(glm::vec3(0, 0, 0))
				};
				assert((lface.v3 > -1) == (rface.v3 > -1));
				if(lface.v3 > -1) {
					lverts[3] = lmesh.vertices.at(lface.v3);
					rverts[3] = rmesh.vertices.at(rface.v3);
				}
				for(s32 k = 0; k < 4; k++) {
					assert(lverts[k].pos == rverts[k].pos);
					assert(lverts[k].normal == rverts[k].normal);
					// We don't currently preserve joint indices, so we don't
					// check them here.
					for(s32 l = 0; l < 3; l++) {
						lverts[k].skin.joints[l] = 0;
						rverts[k].skin.joints[l] = 0;
					}
					assert(lverts[k].skin == rverts[k].skin);
					assert(lverts[k].tex_coord == rverts[k].tex_coord);
				}
			}
			assert(lsub.material == rsub.material);
		}
	}
}