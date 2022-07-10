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

#include "assetmgr/asset_dispatch.h"
#include "core/mesh.h"
#include <core/png.h>
#include <core/collada.h>
#include <engine/sky.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

#define SEPERATE_CLUSTERS

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config);
static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config);
static void unpack_materials(ColladaScene& scene, CollectionAsset& materials, const std::vector<Texture>& textures);
static AssetTestResult test_sky_asset(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(Sky, []() {
	SkyAsset::funcs.unpack_rac1 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_rac2 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_rac3 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_dl = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	
	SkyAsset::funcs.pack_rac1 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_rac2 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_rac3 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_dl = wrap_packer_func<SkyAsset>(pack_sky_asset);
	
	SkyAsset::funcs.test = new AssetTestFunc(test_sky_asset);
})

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config) {
	std::vector<u8> buffer = src.read_multiple<u8>(src.size());
	Sky sky = read_sky(buffer, config.game(), config.framerate());
	
	CollectionAsset& shells = dest.shells();
	
	ColladaScene scene;
	unpack_materials(scene, dest.materials(), sky.textures);
	
	for(size_t i = 0; i < sky.shells.size(); i++) {
#ifdef SEPERATE_CLUSTERS
		SkyShell& shell = sky.shells[i];
		for(size_t j = 0; j < shell.clusters.size(); j++) {
			shell.clusters[j].name = stringf("shell%d_cluster%d", i, j);
			scene.meshes.emplace_back(std::move(shell.clusters[j]));
		}
#else
		u32 flags = MESH_HAS_VERTEX_COLOURS | MESH_HAS_TEX_COORDS;
		Mesh mesh = merge_meshes(sky.shells[i].clusters, stringf("shell_%d", i), flags);
		scene.meshes.emplace_back(std::move(mesh));
#endif
	}
	
	std::vector<u8> collada = write_collada(scene);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.dae");
	stream->write_v(collada);
	
	for(size_t i = 0; i < sky.shells.size(); i++) {
		SkyShell& src = sky.shells[i];
		SkyShellAsset& dest = shells.child<SkyShellAsset>(i);
		
		dest.set_textured(src.textured);
		dest.set_bloom(src.bloom);
		dest.set_starting_rotation(src.rotation);
		dest.set_angular_velocity(src.angular_velocity);
		
		CollectionAsset& meshes = dest.mesh<CollectionAsset>();
		for(size_t j = 0; j < sky.shells[i].clusters.size(); j++) {
			MeshAsset& mesh = meshes.child<MeshAsset>(j);
			mesh.set_name(stringf("shell%d_cluster%d", i, j));
			mesh.set_src(ref);
		}
	}
}

struct TextureLoad {
	FileReference ref;
	s32 index;
};

static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config) {
	Sky sky;
	
	std::map<std::string, TextureLoad> texture_loads;
	src.get_materials().for_each_logical_child_of_type<MaterialAsset>([&](const MaterialAsset& material) {
		if(material.has_texture()) {
			texture_loads[material.name()] = {material.get_texture().src()};
		}
	});
	
	s32 tex = 0;
	for(auto& [name, texture_load] : texture_loads) {
		auto stream = texture_load.ref.owner->open_binary_file_for_reading(texture_load.ref);
		Opt<Texture> texture = read_png(*stream);
		verify(texture.has_value(), "Failed to read sky texture.");
		sky.textures.emplace_back(*texture);
		texture_load.index = tex++;
	}
	
	std::vector<FileReference> refs;
	src.get_shells().for_each_logical_child_of_type<SkyShellAsset>([&](const SkyShellAsset& shell_asset) {
		SkyShell shell;
		shell.textured = shell_asset.textured();
		shell.bloom = shell_asset.bloom();
		shell.rotation = shell_asset.starting_rotation();
		shell.angular_velocity = shell_asset.angular_velocity();
		sky.shells.emplace_back(shell);
		
		shell_asset.get_mesh().for_each_logical_child_of_type<MeshAsset>([&](const MeshAsset& cluster) {
			refs.emplace_back(cluster.src());
		});
	});
	
	std::vector<std::unique_ptr<ColladaScene>> owners;
	std::vector<ColladaScene*> scenes = read_collada_files(owners, refs);
	
	s32 i = 0;
	s32 shell = 0;
	src.get_shells().for_each_logical_child_of_type<SkyShellAsset>([&](const SkyShellAsset& shell_asset) {
		shell_asset.get_mesh().for_each_logical_child_of_type<MeshAsset>([&](const MeshAsset& asset) {
			std::string name = asset.name();
			ColladaScene& scene = *scenes[i];
			Mesh* mesh_ptr = scene.find_mesh(name);
			verify(mesh_ptr, "Cannot find mesh '%s'.", name.c_str());
			Mesh mesh = *mesh_ptr;
			for(SubMesh& submesh : mesh.submeshes) {
				if(submesh.material > -1) {
					std::string& material_name = scene.materials.at(submesh.material).name;
					auto texture_load = texture_loads.find(material_name);
					if(texture_load != texture_loads.end()) {
						submesh.material = texture_load->second.index;
					} else {
						submesh.material = -1;
					}
				}
			}
			sky.shells.at(shell).clusters.emplace_back(std::move(mesh));
		});
		shell++;
	});
	
	std::vector<u8> buffer;
	write_sky(buffer, sky, config.game(), config.framerate());
	dest.write_v(buffer);
}

static void unpack_materials(ColladaScene& scene, CollectionAsset& materials, const std::vector<Texture>& textures) {
	Material& dummy = scene.materials.emplace_back();
	dummy.name = "dummy";
	dummy.colour = glm::vec4(1.f, 0.f, 1.f, 1.f);
	
	MaterialAsset& dummy_asset = materials.child<MaterialAsset>(0);
	dummy_asset.set_name("dummy");
	
	for(s32 i = 0; i < (s32) textures.size(); i++) {
		auto [stream, ref] = materials.file().open_binary_file_for_writing(stringf("%d.png", i));
		write_png(*stream, textures[i]);
		
		Material& mat = scene.materials.emplace_back();
		mat.name = stringf("material_%d", i);
		mat.texture = i;
		
		scene.texture_paths.emplace_back(ref.path.string());
		
		MaterialAsset& asset = materials.child<MaterialAsset>(i + 1);
		asset.set_name(mat.name);
		asset.texture().set_src(ref);
	}
}

static AssetTestResult test_sky_asset(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode) {
	SkyHeader header = Buffer(original).read<SkyHeader>(0, "header");
	bool headers_equal = diff_buffers(original, repacked, 0, header.texture_data, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	
	// Don't test the bounding spheres.
	std::vector<ByteRange64> ignore;
	verify(header.shell_count <= 8, "Bad shell count.");
	for(s32 i = 0; i < header.shell_count; i++) {
		SkyShellHeader shell = Buffer(original).read<SkyShellHeader>(header.shells[i], "shell header");
		for(s32 j = 0; j < shell.cluster_count; j++) {
			s64 cluster_header_ofs = header.shells[i] + sizeof(SkyShellHeader) + j * sizeof(SkyClusterHeader);
			ignore.emplace_back(cluster_header_ofs, sizeof(SkyClusterHeader::bounding_sphere));
		}
	}
	
	bool data_equal = diff_buffers(original, repacked, header.texture_data, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL, &ignore);
	return (headers_equal && data_equal) ? AssetTestResult::PASS : AssetTestResult::FAIL;
}
