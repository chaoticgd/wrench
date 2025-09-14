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
#include <toolwads/wads.h>
#include <assetmgr/material_asset.h>
#include <engine/moby_low.h>
#include <engine/moby_high.h>
#include <wrenchbuild/tests.h>

static void unpack_moby_class(
	MobyClassAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_moby_class(
	OutputStream& dest, const MobyClassAsset& src, BuildConfig config, const char* hint);
static void unpack_phat_class(MobyClassAsset& dest, InputStream& src, BuildConfig config);
static void unpack_mesh_only_class(
	MobyClassAsset& dest, InputStream& src, f32 scale, bool animated, BuildConfig config);
static void unpack_moby_mesh(
	GLTF::ModelFile& gltf,
	GLTF::Scene& scene,
	const std::vector<MOBY::MobyPacket>& packets,
	f32 scale,
	bool animated,
	const std::string name);
static void pack_mesh_only_class(OutputStream& dest, const MobyClassAsset& src, BuildConfig config);
static s32 count_materials(const CollectionAsset& materials);
static void unpack_materials(CollectionAsset& materials, GLTF::ModelFile& gltf);
static bool test_moby_class_core(
	std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(MobyClass, []() {
	MobyClassAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	MobyClassAsset::funcs.unpack_dl = wrap_hint_unpacker_func<MobyClassAsset>(unpack_moby_class);
	
	MobyClassAsset::funcs.pack_rac1 = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class);
	MobyClassAsset::funcs.pack_rac2 = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class);
	MobyClassAsset::funcs.pack_rac3 = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class);
	MobyClassAsset::funcs.pack_dl = wrap_hint_packer_func<MobyClassAsset>(pack_moby_class);
	
	MobyClassCoreAsset::funcs.test_rac = new AssetTestFunc(test_moby_class_core);
	MobyClassCoreAsset::funcs.test_gc  = new AssetTestFunc(test_moby_class_core);
	MobyClassCoreAsset::funcs.test_uya = new AssetTestFunc(test_moby_class_core);
	MobyClassCoreAsset::funcs.test_dl  = new AssetTestFunc(test_moby_class_core);
})

static void unpack_moby_class(
	MobyClassAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	if (g_asset_unpacker.dump_binaries) {
		if (!dest.has_core()) {
			unpack_asset_impl(dest.core<MobyClassCoreAsset>(), src, nullptr, config);
		}
		return;
	}
	
	const char* type = next_hint(&hint);
	bool is_mesh_only = strcmp(type, "meshonly") == 0;
	
	if (is_mesh_only) {
		f32 scale = atof(next_hint(&hint));
		const char* animated_str = next_hint(&hint);
		bool animated;
		if (strcmp(animated_str, "false") == 0) {
			animated = false;
		} else if (strcmp(animated_str, "true") == 0) {
			animated = true;
		} else {
			verify_not_reached("Invalid moby class hint: <animated> must be 'true' or 'false'.");
		}
		unpack_mesh_only_class(dest, src, scale, animated, config);
		return;
	}
	
	unpack_phat_class(dest, src, config);
}

static void pack_moby_class(
	OutputStream& dest, const MobyClassAsset& src, BuildConfig config, const char* hint)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	if (src.get_core().logical_type() == BinaryAsset::ASSET_TYPE) {
		pack_asset_impl(dest, nullptr, nullptr, src.get_core(), config);
		return;
	}
	
	const char* type = next_hint(&hint);
	bool is_mesh_only = strcmp(type, "meshonly") == 0;
	if (is_mesh_only) {
		pack_mesh_only_class(dest, src, config);
		return;
	}
	
	verify_not_reached("MobyClassCore asset packing not yet implemented");
}

static void unpack_phat_class(MobyClassAsset& dest, InputStream& src, BuildConfig config)
{
	unpack_asset_impl(dest.core<BinaryAsset>(), src, nullptr, config);
	
	s32 texture_count = 0;
	if (!g_asset_unpacker.dump_binaries && dest.has_materials()) {
		texture_count = count_materials(dest.get_materials());
	}
	
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	MOBY::MobyClassData data = MOBY::read_class(buffer, config.game());
	
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	
	bool animated = data.animation.joints.size() > 0;
	
	unpack_moby_mesh(gltf, *scene, data.mesh.high_lod, data.scale, animated, "moby");
	unpack_moby_mesh(gltf, *scene, data.mesh.low_lod, data.scale, animated, "moby_low_lod");
	
	for (size_t i = 0; i < data.bangles.size(); i++) {
		unpack_moby_mesh(gltf, *scene, data.bangles[i].high_lod, data.scale, animated, stringf("bangle_%zu", i));
		unpack_moby_mesh(gltf, *scene, data.bangles[i].low_lod, data.scale, animated, stringf("bangle_%zu_low_lod", i));
	}
	
	if (!g_asset_unpacker.dump_binaries && dest.has_materials()) {
		unpack_materials(dest.get_materials(), gltf);
	}
	
	std::vector<u8> glb = GLTF::write_glb(gltf);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.glb");
	stream->write_v(glb);
	
	MeshAsset& editor_mesh = dest.editor_mesh();
	editor_mesh.set_name("moby");
	editor_mesh.set_src(ref);
}

static void unpack_mesh_only_class(
	MobyClassAsset& dest, InputStream& src, f32 scale, bool animated, BuildConfig config)
{
	unpack_asset_impl(dest.core<BinaryAsset>(), src, nullptr, config);
	
	std::vector<u8> buffer = src.read_multiple<u8>(0, src.size());
	MOBY::MobyMeshSection meshes = MOBY::read_mesh_only_class(buffer, config.game());
	
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	
	unpack_moby_mesh(gltf, *scene, meshes.high_lod, scale, animated, "moby");
	unpack_moby_mesh(gltf, *scene, meshes.low_lod, scale, animated, "moby_low_lod");
	
	if (!g_asset_unpacker.dump_binaries && dest.has_materials()) {
		unpack_materials(dest.get_materials(), gltf);
	}
	
	std::vector<u8> glb = GLTF::write_glb(gltf);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.glb");
	stream->write_v(glb);
	
	//MobyClassCoreAsset& core = dest.core<MobyClassCoreAsset>();
	//
	//MeshAsset& moby_mesh = core.mesh();
	//moby_mesh.set_name("moby");
	//moby_mesh.set_src(ref);
	//
	//MeshAsset& moby_low_lod_mesh = core.low_lod_mesh();
	//moby_low_lod_mesh.set_name("moby_low_lod");
	//moby_low_lod_mesh.set_src(ref);
	//
	//core.set_scale(scale);
}

static void unpack_moby_mesh(
	GLTF::ModelFile& gltf,
	GLTF::Scene& scene,
	const std::vector<MOBY::MobyPacket>& packets,
	f32 scale,
	bool animated,
	const std::string name)
{
	scene.nodes.emplace_back((s32) gltf.nodes.size());
	GLTF::Node& high_lod_node = gltf.nodes.emplace_back();
	high_lod_node.name = name;
	high_lod_node.mesh = (s32) gltf.meshes.size();
	
	std::vector<GLTF::Mesh> high_lod_packets = MOBY::recover_packets(packets, -1, scale, animated);
	gltf.meshes.emplace_back(MOBY::merge_packets(high_lod_packets, stringf("%s_mesh", name.c_str())));
}

static void pack_mesh_only_class(OutputStream& dest, const MobyClassAsset& src, BuildConfig config)
{
	const MobyClassCoreAsset& core = src.get_core().as<MobyClassCoreAsset>();
	
	const MeshAsset& mesh_asset = core.get_mesh();
	std::unique_ptr<InputStream> stream = mesh_asset.src().open_binary_file_for_reading();
	GLTF::ModelFile gltf = GLTF::read_glb(stream->read_multiple<u8>(stream->size()));
	GLTF::Node* node = GLTF::lookup_node(gltf, mesh_asset.name().c_str());
	verify(node, "No node with name '%s'.", mesh_asset.name().c_str());
	verify(node->mesh.has_value(), "Node with name '%s' has no mesh.", mesh_asset.name().c_str());
	GLTF::Mesh& mesh = gltf.meshes.at(*node->mesh);
	
	MaterialSet material_set = read_material_assets(src.get_materials());
	map_gltf_materials_to_wrench_materials(gltf, material_set.materials);
	
	auto [effectives, material_to_effective] = effective_materials(material_set.materials, MATERIAL_ATTRIB_SURFACE | MATERIAL_ATTRIB_WRAP_MODE);
	
	std::vector<GLTF::Mesh> gltf_packets = MOBY::split_packets(mesh, material_to_effective, false);
	std::vector<MOBY::MobyPacket> packets = MOBY::build_packets(gltf_packets, effectives, material_set.materials, core.scale());
	
	MOBY::MobyMeshSection moby;
	moby.high_lod = std::move(packets);
	moby.high_lod_count = (u8) moby.high_lod.size();
	moby.has_packet_table = true;
	
	std::vector<u8> buffer;
	MOBY::write_mesh_only_class(buffer, moby, core.scale(), config.game());
	dest.write_v(buffer);
}

static s32 count_materials(const CollectionAsset& materials)
{
	s32 texture_count = 0;
	for (s32 i = 0; i < 16; i++) {
		if (materials.has_child(i)) {
			texture_count++;
		} else {
			break;
		}
	}
	return texture_count;
}

static void unpack_materials(CollectionAsset& materials, GLTF::ModelFile& gltf)
{
	for (s32 i = 0; i < 16; i++) {
		if (!materials.has_child(i)) {
			break;
		}
		
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
	}
}

static bool test_moby_class_core(
	std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode)
{
	// Test the binary reading/writing code.
	MOBY::MobyClassData moby = MOBY::read_class(src, config.game());
	
	std::vector<u8> dest;
	MOBY::write_class(dest, moby, config.game());
	
	strip_trailing_padding_from_lhs(src, dest, 0x40);
	
	bool header_matches = diff_buffers(src, dest, 0, 0x50, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	bool data_matches = diff_buffers(src, dest, 0x50, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	
	// Test the code that splits up the mesh into packets.
	for (std::vector<MOBY::MobyPacket>* packets : {&moby.mesh.high_lod, &moby.mesh.low_lod}) {
		std::vector<GLTF::Mesh> src_meshes = MOBY::recover_packets(*packets, -1, 1.f, moby.animation.joints.size() > 0);
		GLTF::Mesh combined_mesh = MOBY::merge_packets(src_meshes, "moby");
		std::vector<GLTF::Mesh> dest_meshes = MOBY::split_packets(combined_mesh, {}, true);
		for (size_t i = 0; i < std::min(src_meshes.size(), dest_meshes.size()); i++) {
			std::string context = stringf("packet %d", (s32) i);
			GLTF::verify_meshes_equal(src_meshes[i], dest_meshes[i], false, false, context.c_str());
			printf("packet %d passed!\n", (s32) i);
		}
		verify(src_meshes.size() == dest_meshes.size(), "Packet count doesn't match.");
	}
	
	return header_matches && data_matches;
}
