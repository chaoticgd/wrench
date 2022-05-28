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

#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>
#include <engine/moby.h>

static void unpack_moby_class_core(MobyClassCoreAsset& dest, InputStream& src, Game game);
static void pack_moby_class_core(OutputStream& dest, const MobyClassCoreAsset& src, Game game);
static std::vector<ColladaScene*> read_collada_files(std::vector<std::unique_ptr<ColladaScene>>& owners, std::vector<FileReference> refs);
static bool test_moby_class_core(std::vector<u8>& original, std::vector<u8>& repacked, Game game, s32 hint);

on_load(MobyClass, []() {
	MobyClassCoreAsset::funcs.unpack_rac1 = wrap_unpacker_func<MobyClassCoreAsset>(unpack_moby_class_core);
	MobyClassCoreAsset::funcs.unpack_rac2 = wrap_unpacker_func<MobyClassCoreAsset>(unpack_moby_class_core);
	MobyClassCoreAsset::funcs.unpack_rac3 = wrap_unpacker_func<MobyClassCoreAsset>(unpack_moby_class_core);
	MobyClassCoreAsset::funcs.unpack_dl = wrap_unpacker_func<MobyClassCoreAsset>(unpack_moby_class_core);
	
	MobyClassCoreAsset::funcs.pack_rac1 = wrap_packer_func<MobyClassCoreAsset>(pack_moby_class_core);
	MobyClassCoreAsset::funcs.pack_rac2 = wrap_packer_func<MobyClassCoreAsset>(pack_moby_class_core);
	MobyClassCoreAsset::funcs.pack_rac3 = wrap_packer_func<MobyClassCoreAsset>(pack_moby_class_core);
	MobyClassCoreAsset::funcs.pack_dl = wrap_packer_func<MobyClassCoreAsset>(pack_moby_class_core);
	
	MobyClassCoreAsset::funcs.test = new AssetTestFunc(test_moby_class_core);
})

static void unpack_moby_class_core(MobyClassCoreAsset& dest, InputStream& src, Game game) {
	src.seek(0);
	std::vector<u8> buffer = src.read_multiple<u8>(src.size());
	MobyClassData data = read_moby_class(buffer, game);
	ColladaScene scene = recover_moby_class(data, -1, 0);
	std::vector<u8> xml = write_collada(scene);
	FileReference ref = dest.file().write_text_file("mesh.dae", (char*) xml.data());
	
	MeshAsset& mesh = dest.mesh();
	mesh.set_src(ref);
	mesh.set_node("high_lod");
	
	MeshAsset& low_lod_mesh = dest.low_lod_mesh();
	low_lod_mesh.set_src(ref);
	low_lod_mesh.set_node("low_lod");
}

static void pack_moby_class_core(OutputStream& dest, const MobyClassCoreAsset& src, Game game) {
	const MeshAsset& mesh_asset = src.get_mesh();
	const MeshAsset& low_lod_mesh_asset = src.get_low_lod_mesh();
	
	std::vector<std::unique_ptr<ColladaScene>> owners;
	std::vector<ColladaScene*> scenes = read_collada_files(owners, {mesh_asset.src(), low_lod_mesh_asset.src()});
	assert(scenes.size() == 2);
	
	Mesh* mesh = scenes[0]->find_mesh(mesh_asset.node());
	Mesh* low_lod_mesh = scenes[1]->find_mesh(mesh_asset.node());
	verify(mesh, "Failed to find mesh in COLLADA file.");
	
	MobyClassData moby;
	moby.submeshes = build_moby_submeshes(*mesh, scenes[0]->materials);
	moby.submesh_count = moby.submeshes.size();
	if(low_lod_mesh) {
		moby.low_lod_submeshes = build_moby_submeshes(*low_lod_mesh, scenes[1]->materials);
		moby.low_lod_submesh_count = moby.low_lod_submeshes.size();
	}
	moby.skeleton = {};
	moby.common_trans = {};
	moby.unknown_9 = 0;
	moby.lod_trans = 0x20;
	moby.scale = 0.25;
	moby.mip_dist = 0x8;
	moby.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	moby.glow_rgba = 0;
	moby.mode_bits = 0x5000;
	moby.type = 0;
	moby.mode_bits2 = 0;
	moby.header_end_offset = 0;
	moby.submesh_table_offset = 0;
	moby.rac1_byte_a = 0;
	moby.rac1_byte_b = 0;
	moby.rac1_short_2e = 0;
	moby.has_submesh_table = true;
	
	MobySequence dummy_seq;
	dummy_seq.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	dummy_seq.frames.emplace_back();
	moby.sequences.emplace_back(std::move(dummy_seq));
	
	std::vector<u8> dest_bytes;
	write_moby_class(dest_bytes, moby, game);
	dest.write_n(dest_bytes.data(), dest_bytes.size());
}

static std::vector<ColladaScene*> read_collada_files(std::vector<std::unique_ptr<ColladaScene>>& owners, std::vector<FileReference> refs) {
	std::vector<ColladaScene*> scenes;
	for(size_t i = 0; i < refs.size(); i++) {
		bool unique = true;
		size_t j;
		for(j = 0; j < refs.size(); j++) {
			if(i != j && i > j) {
				unique = false;
				break;
			}
		}
		if(unique) {
			std::string xml = refs[i].owner->read_text_file(refs[i].path);
			std::vector<u8> copy(xml.begin(), xml.end());
			std::unique_ptr<ColladaScene>& owner = owners.emplace_back(std::make_unique<ColladaScene>(read_collada(std::move(copy))));
			scenes.emplace_back(owner.get());
		} else {
			scenes.emplace_back(scenes[j]);
		}
	}
	return scenes;
}

static bool test_moby_class_core(std::vector<u8>& original, std::vector<u8>& repacked, Game game, s32 hint) {
	return false;
}
