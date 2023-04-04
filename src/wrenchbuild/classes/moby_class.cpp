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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <engine/moby.h>

static void unpack_moby_class(MobyClassAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_moby_class_core(OutputStream& dest, const MobyClassAsset& src, BuildConfig config, const char* hint);
static bool test_moby_class_core(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(MobyClass, []() {
	MobyClassAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_dl = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	
	MobyClassAsset::funcs.pack_rac1 = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class_core);
	MobyClassAsset::funcs.pack_rac2 = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class_core);
	MobyClassAsset::funcs.pack_rac3 = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class_core);
	MobyClassAsset::funcs.pack_dl = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class_core);
	
	MobyClassAsset::funcs.test_rac = new AssetTestFunc(test_moby_class_core);
	MobyClassAsset::funcs.test_gc  = new AssetTestFunc(test_moby_class_core);
	MobyClassAsset::funcs.test_uya = new AssetTestFunc(test_moby_class_core);
	MobyClassAsset::funcs.test_dl  = new AssetTestFunc(test_moby_class_core);
})

static void unpack_moby_class(MobyClassAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	unpack_asset_impl(dest.core<BinaryAsset>(), src, nullptr, config);
	
	s32 texture_count = 0;
	if(!g_asset_unpacker.dump_binaries && dest.has_materials()) {
		CollectionAsset& materials = dest.get_materials();
		for(s32 i = 0; i < 16; i++) {
			if(materials.has_child(i)) {
				texture_count++;
			} else {
				break;
			}
		}
	}
	
	const char* type = next_hint(&hint);
	bool is_level = strcmp(type, "level") == 0;
	bool is_gadget = strcmp(type, "gadget") == 0;
	bool is_mission = strcmp(type, "mission") == 0;
	bool is_sparmor = strcmp(type, "sparmor") == 0;
	bool is_mparmor = strcmp(type, "mparmor") == 0;
	verify(is_level || is_gadget || is_mission || is_sparmor || is_mparmor, "Invalid moby hint.");
	
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	
	MobyClassData data;
	if((config.game() == Game::GC && is_sparmor) || (config.game() == Game::UYA && is_sparmor)) {
		data = read_armor_moby_class(buffer, config.game());
	} else {
		data = read_moby_class(buffer, config.game());
	}
	ColladaScene scene = recover_moby_class(data, -1, texture_count);
	
	if(!g_asset_unpacker.dump_binaries && dest.has_materials()) {
		CollectionAsset& materials = dest.get_materials();
		for(s32 i = 0; i < 16; i++) {
			if(materials.has_child(i)) {
				TextureAsset& texture = materials.get_child(i).as<TextureAsset>();
				scene.texture_paths.push_back(texture.src().path.string());
			} else {
				break;
			}
		}
	}
	
	std::vector<u8> xml = write_collada(scene);
	auto ref = dest.file().write_text_file("mesh.dae", (char*) xml.data());
	
	MeshAsset& editor_mesh = dest.editor_mesh();
	editor_mesh.set_name("high_lod");
	editor_mesh.set_src(ref);
}

static void pack_moby_class_core(OutputStream& dest, const MobyClassAsset& src, BuildConfig config, const char* hint) {
	pack_asset_impl(dest, nullptr, nullptr, src.get_core(), config);
	//const MeshAsset& mesh_asset = src.get_mesh();
	//const MeshAsset& low_lod_mesh_asset = src.get_low_lod_mesh();
	//
	//std::vector<std::unique_ptr<ColladaScene>> owners;
	//std::vector<ColladaScene*> scenes = read_collada_files(owners, {mesh_asset.src(), low_lod_mesh_asset.src()});
	//verify_fatal(scenes.size() == 2);
	//
	//Mesh* mesh = scenes[0]->find_mesh(mesh_asset.node());
	//Mesh* low_lod_mesh = scenes[1]->find_mesh(mesh_asset.node());
	//verify(mesh, "Failed to find mesh in COLLADA file.");
	//
	//MobyClassData moby;
	//moby.submeshes = build_moby_submeshes(*mesh, scenes[0]->materials);
	//moby.submesh_count = moby.submeshes.size();
	//if(low_lod_mesh) {
	//	moby.low_lod_submeshes = build_moby_submeshes(*low_lod_mesh, scenes[1]->materials);
	//	moby.low_lod_submesh_count = moby.low_lod_submeshes.size();
	//}
	//moby.skeleton = {};
	//moby.common_trans = {};
	//moby.unknown_9 = 0;
	//moby.lod_trans = 0x20;
	//moby.scale = 0.25;
	//moby.mip_dist = 0x8;
	//moby.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	//moby.glow_rgba = 0;
	//moby.mode_bits = 0x5000;
	//moby.type = 0;
	//moby.mode_bits2 = 0;
	//moby.header_end_offset = 0;
	//moby.submesh_table_offset = 0;
	//moby.rac1_byte_a = 0;
	//moby.rac1_byte_b = 0;
	//moby.rac1_short_2e = 0;
	//moby.has_submesh_table = true;
	//
	//MobySequence dummy_seq;
	//dummy_seq.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	//dummy_seq.frames.emplace_back();
	//moby.sequences.emplace_back(std::move(dummy_seq));
	//
	//std::vector<u8> dest_bytes;
	//write_moby_class(dest_bytes, moby, config.game());
	//dest.write_n(dest_bytes.data(), dest_bytes.size());
}

static bool test_moby_class_core(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode) {
	MobyClassData moby = read_moby_class(src, config.game());
	
	std::vector<u8> dest;
	write_moby_class(dest, moby, config.game());
	
	return diff_buffers(src, dest, 0, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
}
