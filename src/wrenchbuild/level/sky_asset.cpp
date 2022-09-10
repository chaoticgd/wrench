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

#include <core/png.h>
#include <core/collada.h>
#include <engine/sky.h>
#include <map>
#include <string>
#include <utility>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

#define SEPERATE_SKY_CLUSTERS

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config);
static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config);
static void unpack_sky_textures(ColladaScene& scene, CollectionAsset& fx, CollectionAsset& materials, const Sky& sky);
static std::map<std::string, s32> pack_sky_textures(Sky& dest, const SkyAsset& src);
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
	
	SkyAsset::funcs.test_rac = new AssetTestFunc(test_sky_asset);
	SkyAsset::funcs.test_gc  = new AssetTestFunc(test_sky_asset);
	SkyAsset::funcs.test_uya = new AssetTestFunc(test_sky_asset);
	SkyAsset::funcs.test_dl  = new AssetTestFunc(test_sky_asset);
})

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config) {
	std::vector<u8> buffer = src.read_multiple<u8>(src.size());
	Sky sky = read_sky(buffer, config.framerate());
	
	SkyColour col = sky.colour;
	dest.set_colour(glm::vec4(
		col.r / 255.f, col.g / 255.f, col.b / 255.f, col.a == 0x80 ? 1.f : col.a / 127.f));
	dest.set_clear_screen(sky.clear_screen);
	dest.set_maximum_sprite_count(sky.maximum_sprite_count);
	
	CollectionAsset& shells = dest.shells();
	
	ColladaScene scene;
	unpack_sky_textures(scene, dest.fx(), dest.materials(), sky);
	
	// Copy all the meshes into the scene.
	for(size_t i = 0; i < sky.shells.size(); i++) {
#ifdef SEPERATE_SKY_CLUSTERS
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
	
	// Convert from texture indices to material indices.
	for(Mesh& mesh : scene.meshes) {
		for(SubMesh& submesh : mesh.submeshes) {
			if(submesh.material > -1) {
				submesh.material += 1 - (s32) sky.fx.size();
			} else {
				submesh.material = 0;
			}
		}
	}
	
	// Write out the COLLADA file.
	std::vector<u8> collada = write_collada(scene);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.dae");
	stream->write_v(collada);
	
	// Create the assets for the shells and clusters.
	for(size_t i = 0; i < sky.shells.size(); i++) {
		SkyShell& src = sky.shells[i];
		SkyShellAsset& dest = shells.child<SkyShellAsset>(i);
		
		dest.set_textured(src.textured);
		if(config.game() == Game::UYA || config.game() == Game::DL) {
			dest.set_bloom(src.bloom);
		}
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

static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config) {
	Sky sky;
	
	if(src.has_colour()) {
		glm::vec4 col = src.colour();
		sky.colour = {
			(u8) roundf(col.r * 255.f),
			(u8) roundf(col.g * 255.f),
			(u8) roundf(col.b * 255.f),
			(u8) (fabs(col.a - 1.f) < 0.0001f ? 0x80 : roundf(col.a * 127.f))
		};
	}
	if(src.has_clear_screen()) {
		sky.clear_screen = src.clear_screen();
	}
	if(src.has_maximum_sprite_count()) {
		sky.maximum_sprite_count = src.maximum_sprite_count();
	}
	
	// Read all the references to meshes.
	std::vector<FileReference> refs;
	src.get_shells().for_each_logical_child_of_type<SkyShellAsset>([&](const SkyShellAsset& shell_asset) {
		SkyShell shell;
		shell.textured = shell_asset.textured();
		if(shell_asset.has_bloom() && (config.game() == Game::UYA || config.game() == Game::DL)) {
			shell.bloom = shell_asset.bloom();
		}
		if(shell_asset.has_starting_rotation()) {
			shell.rotation = shell_asset.starting_rotation();
		}
		if(shell_asset.has_angular_velocity()) {
			shell.angular_velocity = shell_asset.angular_velocity();
		}
		sky.shells.emplace_back(shell);
		
		shell_asset.get_mesh().for_each_logical_child_of_type<MeshAsset>([&](const MeshAsset& cluster) {
			refs.emplace_back(cluster.src());
		});
	});
	
	// Read all the referenced meshes, avoiding duplicated work where possible.
	std::vector<std::unique_ptr<ColladaScene>> owners;
	std::vector<ColladaScene*> scenes = read_collada_files(owners, refs);
	
	// Setup all the textures.
	std::map<std::string, s32> material_to_texture = pack_sky_textures(sky, src);
	
	s32 i = 0;
	s32 shell = 0;
	src.get_shells().for_each_logical_child_of_type<SkyShellAsset>([&](const SkyShellAsset& shell_asset) {
		shell_asset.get_mesh().for_each_logical_child_of_type<MeshAsset>([&](const MeshAsset& asset) {
			std::string name = asset.name();
			ColladaScene& scene = *scenes.at(i++);
			Mesh* mesh_ptr = scene.find_mesh(name);
			verify(mesh_ptr, "Cannot find mesh '%s'.", name.c_str());
			Mesh mesh = *mesh_ptr;
			for(SubMesh& submesh : mesh.submeshes) {
				if(submesh.material > -1) {
					std::string& material_name = scene.materials.at(submesh.material).name;
					auto mapping = material_to_texture.find(material_name);
					if(mapping != material_to_texture.end()) {
						submesh.material = mapping->second;
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
	write_sky(buffer, sky, config.framerate());
	dest.write_v(buffer);
}

static void unpack_sky_textures(ColladaScene& scene, CollectionAsset& fx, CollectionAsset& materials, const Sky& sky) {
	std::vector<FileReference> texture_refs;
	
	// Write out the textures.
	for(s32 i = 0; i < (s32) sky.textures.size(); i++) {
		auto [stream, ref] = materials.file().open_binary_file_for_writing(stringf("%d.png", i));
		write_png(*stream, sky.textures[i]);
		scene.texture_paths.emplace_back(ref.path.string());
		texture_refs.emplace_back(std::move(ref));
	}
	
	// Create the placeholder material for untextured shells.
	Material& gouraud = scene.materials.emplace_back();
	gouraud.name = "gouraud";
	gouraud.colour = glm::vec4(1.f, 0.f, 1.f, 1.f);
	
	MaterialAsset& gouraud_asset = materials.child<MaterialAsset>("gouraud");
	gouraud_asset.set_name("gouraud");
	
	// Create fx texture assets.
	for(s32 i = 0; i < (s32) sky.fx.size(); i++) {
		TextureAsset& texture = fx.child<TextureAsset>(i);
		texture.set_src(texture_refs.at(sky.texture_mappings.at(sky.fx[i])));
	}
	
	// Create shell material assets.
	for(s32 i = (s32) sky.fx.size(); i < (s32) sky.texture_mappings.size(); i++) {
		Material& mat = scene.materials.emplace_back();
		mat.name = stringf("material_%d", i - (s32) sky.fx.size());
		mat.texture = sky.texture_mappings[i];
		
		MaterialAsset& asset = materials.child<MaterialAsset>(i);
		asset.set_name(mat.name);
		asset.texture().set_src(texture_refs.at(sky.texture_mappings[i]));
	}
}

struct TextureLoad {
	FileReference ref;
	Texture texture;
};

std::map<std::string, s32> pack_sky_textures(Sky& dest, const SkyAsset& src) {
	std::map<std::string, s32> material_to_texture;
	
	std::vector<FileReference> refs;
	
	src.get_fx().for_each_logical_child_of_type<TextureAsset>([&](const TextureAsset& texture) {
		FileReference ref = texture.src();
		dest.fx.emplace_back(dest.texture_mappings.size());
		dest.texture_mappings.emplace_back(refs.size());
		refs.emplace_back(std::move(ref));
	});
	
	src.get_materials().for_each_logical_child_of_type<MaterialAsset>([&](const MaterialAsset& material) {
		if(material.has_texture()) {
			const TextureAsset& texture = material.get_texture();
			FileReference ref = texture.src();
			s32 index = -1;
			for(s32 i = 0; i < (s32) refs.size(); i++) {
				if(refs[i].owner == ref.owner && refs[i].path == ref.path) {
					index = i;
					break;
				}
			}
			if(index == -1) {
				index = refs.size();
				refs.emplace_back(std::move((ref)));
			}
			material_to_texture[material.name()] = dest.texture_mappings.size();
			dest.texture_mappings.emplace_back(index);
		}
	});
	
	// Read in the textures from disk.
	for(FileReference& ref : refs) {
		auto stream = ref.owner->open_binary_file_for_reading(ref);
		Opt<Texture> texture = read_png(*stream);
		verify(texture.has_value(), "Failed to read sky texture.");
		dest.textures.emplace_back(*texture);
	}
	
	return material_to_texture;
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
