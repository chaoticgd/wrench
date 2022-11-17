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

#include <assetmgr/material_asset.h>
#include <engine/shrub.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_shrub_class(ShrubClassAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_shrub_class(OutputStream& dest, const ShrubClassAsset& src, BuildConfig config, const char* hint);
static AssetTestResult test_shrub_class(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(ShrubClass, []() {
	ShrubClassAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	ShrubClassAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	ShrubClassAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	ShrubClassAsset::funcs.unpack_dl = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	
	ShrubClassAsset::funcs.pack_rac1 = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	ShrubClassAsset::funcs.pack_rac2 = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	ShrubClassAsset::funcs.pack_rac3 = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	ShrubClassAsset::funcs.pack_dl = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	
	ShrubClassAsset::funcs.test_rac = new AssetTestFunc(test_shrub_class);
	ShrubClassAsset::funcs.test_gc  = new AssetTestFunc(test_shrub_class);
	ShrubClassAsset::funcs.test_uya = new AssetTestFunc(test_shrub_class);
	ShrubClassAsset::funcs.test_dl  = new AssetTestFunc(test_shrub_class);
})

static void unpack_shrub_class(ShrubClassAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	if(g_asset_unpacker.dump_binaries) {
		if(!dest.has_core()) {
			unpack_asset_impl(dest.core<BinaryAsset>(), src, nullptr, config);
		}
		return;
	}
	
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	ShrubClass shrub = read_shrub_class(buffer);
	ColladaScene scene = recover_shrub_class(shrub);
	
	if(dest.has_materials()) {
		CollectionAsset& materials = dest.get_materials();
		for(s32 i = 0; i < 16; i++) {
			if(materials.has_child(i)) {
				MaterialAsset& material = materials.get_child(i).as<MaterialAsset>();
				scene.texture_paths.push_back(material.diffuse().src().path.string());
			} else {
				break;
			}
		}
	}
	
	std::vector<u8> xml = write_collada(scene);
	auto ref = dest.file().write_text_file("mesh.dae", (char*) xml.data());
	
	ShrubClassCoreAsset& core = dest.core<ShrubClassCoreAsset>();
	core.set_mipmap_distance(shrub.mip_distance);
	
	MeshAsset& mesh = core.mesh();
	mesh.set_name("mesh");
	mesh.set_src(ref);
	
	if(shrub.billboard.has_value()) {
		ShrubBillboardAsset& billboard = core.billboard();
		billboard.set_fade_distance(shrub.billboard->fade_distance);
		billboard.set_width(shrub.billboard->width);
		billboard.set_height(shrub.billboard->height);
		billboard.set_z_offset(shrub.billboard->z_ofs);
	}
}

static void pack_shrub_class(OutputStream& dest, const ShrubClassAsset& src, BuildConfig config, const char* hint) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	if(src.get_core().logical_type() == BinaryAsset::ASSET_TYPE) {
		pack_asset_impl(dest, nullptr, nullptr, src.get_core(), config, nullptr);
		return;
	}
	
	const ShrubClassCoreAsset& core = src.get_core().as<ShrubClassCoreAsset>();
	
	const MeshAsset& mesh_asset = core.get_mesh();
	std::string xml = mesh_asset.file().read_text_file(mesh_asset.src().path);
	ColladaScene scene = read_collada((char*) xml.data());
	Mesh* mesh = scene.find_mesh(mesh_asset.name());
	verify(mesh, "No mesh with name '%s'.", mesh_asset.name().c_str());
	
	MaterialSet material_set = read_material_assets(src.get_materials());
	map_lhs_material_indices_to_rhs_list(scene, material_set.materials);
	
	Opt<ShrubBillboardInfo> billboard;
	if(core.has_billboard()) {
		const ShrubBillboardAsset& billboard_asset = core.get_billboard();
		billboard.emplace();
		billboard->fade_distance = billboard_asset.fade_distance();
		billboard->width = billboard_asset.width();
		billboard->height = billboard_asset.height();
		billboard->z_ofs = billboard_asset.z_offset();
	}
	
	ShrubClass shrub = build_shrub_class(*mesh, material_set.materials, core.mipmap_distance(), 0, src.id(), billboard);
	
	std::vector<u8> buffer;
	write_shrub_class(buffer, shrub);
	dest.write_v(buffer);
}

static AssetTestResult test_shrub_class(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode) {
	return AssetTestResult::NOT_RUN;
}
