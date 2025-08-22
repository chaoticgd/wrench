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

#include <assetmgr/material_asset.h>
#include <engine/shrub.h>
#include <toolwads/wads.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

static void unpack_shrub_class(
	ShrubClassAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_shrub_class(
	OutputStream& dest, const ShrubClassAsset& src, BuildConfig config, const char* hint);
static bool test_shrub_class_core(
	std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(ShrubClass, []() {
	ShrubClassAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	ShrubClassAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	ShrubClassAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	ShrubClassAsset::funcs.unpack_dl = wrap_hint_unpacker_func<ShrubClassAsset>(unpack_shrub_class);
	
	ShrubClassAsset::funcs.pack_rac1 = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	ShrubClassAsset::funcs.pack_rac2 = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	ShrubClassAsset::funcs.pack_rac3 = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	ShrubClassAsset::funcs.pack_dl = wrap_hint_packer_func<ShrubClassAsset>(pack_shrub_class);
	
	ShrubClassCoreAsset::funcs.test_rac = new AssetTestFunc(test_shrub_class_core);
	ShrubClassCoreAsset::funcs.test_gc  = new AssetTestFunc(test_shrub_class_core);
	ShrubClassCoreAsset::funcs.test_uya = new AssetTestFunc(test_shrub_class_core);
	ShrubClassCoreAsset::funcs.test_dl  = new AssetTestFunc(test_shrub_class_core);
})

static void unpack_shrub_class(ShrubClassAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	if (g_asset_unpacker.dump_binaries) {
		if (!dest.has_core()) {
			unpack_asset_impl(dest.core<ShrubClassCoreAsset>(), src, nullptr, config);
		}
		return;
	}
	
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	ShrubClass shrub = read_shrub_class(buffer);
	
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	scene->nodes.emplace_back((s32) gltf.nodes.size());
	GLTF::Node& node = gltf.nodes.emplace_back();
	node.name = "shrub";
	node.mesh = (s32) gltf.meshes.size();
	gltf.meshes.emplace_back(recover_shrub_class(shrub));
	
	if (dest.has_materials()) {
		CollectionAsset& materials = dest.get_materials();
		for (s32 i = 0; i < 16; i++) {
			if (materials.has_child(i)) {
				MaterialAsset& material_asset = materials.get_child(i).as<MaterialAsset>();
				
				GLTF::Material& material = gltf.materials.emplace_back();
				material.name = stringf("material_%d", i);
				material.pbr_metallic_roughness.emplace();
				material.pbr_metallic_roughness->base_color_texture.emplace();
				material.pbr_metallic_roughness->base_color_texture->index = (s32) gltf.textures.size();
				material.alpha_mode = GLTF::MASK;
				material.double_sided = true;
				
				GLTF::Texture& texture = gltf.textures.emplace_back();
				texture.source = (s32) gltf.images.size();
				
				GLTF::Image& image = gltf.images.emplace_back();
				image.uri = material_asset.diffuse().src().path.string();
				
				material_asset.set_name(*material.name);
			} else {
				break;
			}
		}
	}
	
	std::vector<u8> glb = GLTF::write_glb(gltf);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.glb");
	stream->write_v(glb);
	
	ShrubClassCoreAsset& core = dest.core<ShrubClassCoreAsset>();
	core.set_mip_distance(shrub.mip_distance);
	
	MeshAsset& mesh = core.mesh();
	mesh.set_name(*node.name);
	mesh.set_src(ref);
	
	if (shrub.billboard.has_value()) {
		ShrubBillboardAsset& billboard = dest.billboard();
		billboard.set_fade_distance(shrub.billboard->fade_distance);
		billboard.set_width(shrub.billboard->width);
		billboard.set_height(shrub.billboard->height);
		billboard.set_z_offset(shrub.billboard->z_ofs);
	}
}

static void pack_shrub_class(OutputStream& dest, const ShrubClassAsset& src, BuildConfig config, const char* hint)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	if (src.get_core().logical_type() == BinaryAsset::ASSET_TYPE) {
		pack_asset_impl(dest, nullptr, nullptr, src.get_core(), config, nullptr);
		return;
	}
	
	const ShrubClassCoreAsset& core = src.get_core().as<ShrubClassCoreAsset>();
	
	const MeshAsset& mesh_asset = core.get_mesh();
	std::unique_ptr<InputStream> stream = mesh_asset.src().open_binary_file_for_reading();
	GLTF::ModelFile gltf = GLTF::read_glb(stream->read_multiple<u8>(stream->size()));
	GLTF::Node* node = GLTF::lookup_node(gltf, mesh_asset.name().c_str());
	verify(node, "No node with name '%s'.", mesh_asset.name().c_str());
	verify(node->mesh.has_value(), "Node with name '%s' has no mesh.", mesh_asset.name().c_str());
	GLTF::Mesh& mesh = gltf.meshes.at(*node->mesh);
	
	MaterialSet material_set = read_material_assets(src.get_materials());
	map_gltf_materials_to_wrench_materials(gltf, material_set.materials);
	
	Opt<ShrubBillboardInfo> billboard;
	if (src.has_billboard()) {
		const ShrubBillboardAsset& billboard_asset = src.get_billboard();
		billboard.emplace();
		billboard->fade_distance = billboard_asset.fade_distance();
		billboard->width = billboard_asset.width();
		billboard->height = billboard_asset.height();
		billboard->z_ofs = billboard_asset.z_offset();
	}
	
	ShrubClass shrub = build_shrub_class(mesh, material_set.materials, core.mip_distance(), 0, src.id(), billboard);
	
	std::vector<u8> buffer;
	write_shrub_class(buffer, shrub);
	dest.write_v(buffer);
}

static bool test_shrub_class_core(
	std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode)
{
	ShrubClass shrub = read_shrub_class(src);
	std::vector<u8> dest;
	write_shrub_class(dest, shrub);
	strip_trailing_padding_from_lhs(src, dest);
	return diff_buffers(src, dest, 0, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
}
