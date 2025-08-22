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

#include <map>

#include <core/png.h>
#include <core/collada.h>
#include <engine/sky.h>
#include <toolwads/wads.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

#define SEPERATE_SKY_CLUSTERS

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config);
static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config);
static void unpack_sky_textures(
	GLTF::ModelFile& gltf, CollectionAsset& fx, CollectionAsset& materials, const Sky& sky);
static std::map<std::string, s32> pack_sky_textures(Sky& dest, const SkyAsset& src);
static bool test_sky_asset(std::vector<u8>& original, std::vector<u8>& repacked, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(Sky, []() {
	SkyAsset::funcs.unpack_rac1 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_rac2 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_rac3 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_dl = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	
	SkyAsset::funcs.pack_rac1 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_rac2 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_rac3 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_dl = wrap_packer_func<SkyAsset>(pack_sky_asset);
	
	SkyAsset::funcs.test_rac = wrap_diff_test_func(test_sky_asset);
	SkyAsset::funcs.test_gc  = wrap_diff_test_func(test_sky_asset);
	SkyAsset::funcs.test_uya = wrap_diff_test_func(test_sky_asset);
	SkyAsset::funcs.test_dl  = wrap_diff_test_func(test_sky_asset);
})

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config)
{
	std::vector<u8> buffer = src.read_multiple<u8>(src.size());
	Sky sky = read_sky(buffer, config.game(), config.framerate());
	
	SkyColour col = sky.colour;
	dest.set_colour(glm::vec4(
		col.r / 255.f, col.g / 255.f, col.b / 255.f, col.a == 0x80 ? 1.f : col.a / 127.f));
	dest.set_clear_screen(sky.clear_screen);
	dest.set_maximum_sprite_count(sky.maximum_sprite_count);
	
	CollectionAsset& shells = dest.shells();
	
	auto [gltf, scene] = GLTF::create_default_scene(get_versioned_application_name("Wrench Build Tool"));
	unpack_sky_textures(gltf, dest.fx(), dest.materials(), sky);
	
	// Copy all the meshes into the scene.
	for (size_t i = 0; i < sky.shells.size(); i++) {
		SkyShell& shell = sky.shells[i];
		
		scene->nodes.emplace_back((s32) gltf.nodes.size());
		GLTF::Node& node = gltf.nodes.emplace_back();
		node.name = stringf("shell_%d", (s32) i);
		node.mesh = (s32) gltf.meshes.size();
		
		gltf.meshes.emplace_back(std::move(shell.mesh));
	}
	
	for (size_t i = 0; i < gltf.meshes.size(); i++) {
		gltf.meshes[i].name = stringf("shell_%d", (s32) i);
		for (GLTF::MeshPrimitive& primitive : gltf.meshes[i].primitives) {
			if (primitive.material.has_value()) {
				*primitive.material -= sky.fx.size();
			} else {
				primitive.material = (s32) sky.texture_mappings.size() - sky.fx.size();
			}
		}
	}
	
	// Write out the GLB file.
	std::vector<u8> glb = GLTF::write_glb(gltf);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.glb");
	stream->write_v(glb);
	
	// Create the assets for the shells and clusters.
	for (size_t i = 0; i < sky.shells.size(); i++) {
		SkyShell& shell_src = sky.shells[i];
		SkyShellAsset& shell_dest = shells.child<SkyShellAsset>(i);
		
		if (config.game() != Game::RAC && config.game() != Game::GC) {
			shell_dest.set_bloom(shell_src.bloom);
			shell_dest.set_starting_rotation(shell_src.rotation);
			shell_dest.set_angular_velocity(shell_src.angular_velocity);
		}
		
		MeshAsset& mesh = shell_dest.mesh();
		mesh.set_name(stringf("shell_%d", (s32) i));
		mesh.set_src(ref);
	}
}

static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config)
{
	Sky sky;
	
	if (src.has_colour()) {
		glm::vec4 col = src.colour();
		sky.colour = {
			(u8) roundf(col.r * 255.f),
			(u8) roundf(col.g * 255.f),
			(u8) roundf(col.b * 255.f),
			(u8) (fabs(col.a - 1.f) < 0.0001f ? 0x80 : roundf(col.a * 127.f))
		};
	}
	if (src.has_clear_screen()) {
		sky.clear_screen = src.clear_screen();
	}
	if (src.has_maximum_sprite_count()) {
		sky.maximum_sprite_count = src.maximum_sprite_count();
	}
	
	// Read all the references to meshes.
	std::vector<FileReference> refs;
	src.get_shells().for_each_logical_child_of_type<SkyShellAsset>([&](const SkyShellAsset& shell_asset) {
		refs.emplace_back(shell_asset.get_mesh().src());
	});
	
	// Read all the referenced meshes, avoiding duplicated work where possible.
	std::vector<std::unique_ptr<GLTF::ModelFile>> owners;
	std::vector<GLTF::ModelFile*> gltfs = read_glb_files(owners, refs);
	
	// Setup all the textures.
	std::map<std::string, s32> material_to_texture = pack_sky_textures(sky, src);
	
	s32 shell_index = 0;
	src.get_shells().for_each_logical_child_of_type<SkyShellAsset>([&](const SkyShellAsset& shell_asset) {
		SkyShell& shell = sky.shells.emplace_back();
		shell.textured = false;
		if (config.game() != Game::RAC && config.game() != Game::GC) {
			if (shell_asset.has_bloom()) {
				shell.bloom = shell_asset.bloom();
			}
			if (shell_asset.has_starting_rotation()) {
				shell.rotation = shell_asset.starting_rotation();
			}
			if (shell_asset.has_angular_velocity()) {
				shell.angular_velocity = shell_asset.angular_velocity();
			}
		}
		
		const MeshAsset& asset = shell_asset.get_mesh();
		std::string name = asset.name();
		GLTF::ModelFile& gltf = *gltfs.at(shell_index);
		GLTF::Node* node = GLTF::lookup_node(gltf, name.c_str());
		verify(node, "Node '%s' not found.", name.c_str());
		verify(node->mesh.has_value(), "Node '%s' has no mesh.", name.c_str());
		verify(*node->mesh >= 0 && *node->mesh < gltf.meshes.size(), "Node '%s' has no invalid mesh index.", name.c_str());
		GLTF::Mesh mesh = gltf.meshes[*node->mesh];
		bool has_set_textured = false;
		for (GLTF::MeshPrimitive& primitive : mesh.primitives) {
			bool primitive_has_texture = false;
			if (primitive.material.has_value()) {
				Opt<std::string>& material_name = gltf.materials.at(*primitive.material).name;
				verify(material_name.has_value(), "Material %d has no name.", *primitive.material);
				auto mapping = material_to_texture.find(*material_name);
				if (mapping != material_to_texture.end()) {
					primitive.material = mapping->second;
					shell.textured = true;
					primitive_has_texture = true;
				}
			}
				verify(!has_set_textured || shell.textured == primitive_has_texture, "Sky shell contains both textured and untextured faces.");
			has_set_textured = true;
		}
		shell.mesh = std::move(mesh);
		shell_index++;
	});
	
	std::vector<u8> buffer;
	write_sky(buffer, sky, config.game(), config.framerate());
	dest.write_v(buffer);
}

static void unpack_sky_textures(
	GLTF::ModelFile& gltf, CollectionAsset& fx, CollectionAsset& materials, const Sky& sky)
{
	std::vector<FileReference> texture_refs;
	
	// Write out the textures.
	for (s32 i = 0; i < (s32) sky.textures.size(); i++) {
		auto [stream, ref] = materials.file().open_binary_file_for_writing(stringf("%d.png", i));
		write_png(*stream, sky.textures[i]);
		GLTF::Image& image = gltf.images.emplace_back();
		image.uri = ref.path.string();
		image.mime_type = "image/png";
		texture_refs.emplace_back(std::move(ref));
	}
	
	for (s32 i = 0; i < (s32) sky.fx.size(); i++) {
		TextureAsset& texture = fx.child<TextureAsset>(i);
		texture.set_src(texture_refs.at(sky.texture_mappings.at(i)));
	}
	
	// Create shell material assets.
	for (s32 i = (s32) sky.fx.size(); i < (s32) sky.texture_mappings.size(); i++) {
		GLTF::Material& material = gltf.materials.emplace_back();
		material.name = stringf("material_%d", i - (s32) sky.fx.size());
		material.pbr_metallic_roughness.emplace();
		material.pbr_metallic_roughness->base_color_texture.emplace();
		material.pbr_metallic_roughness->base_color_texture->index = (s32) gltf.textures.size();
		material.alpha_mode = GLTF::BLEND;
		material.double_sided = true;
		
		GLTF::Texture& texture = gltf.textures.emplace_back();
		texture.source = sky.texture_mappings[i];
		
		MaterialAsset& asset = materials.child<MaterialAsset>(i);
		asset.set_name(*material.name);
		asset.diffuse().set_src(texture_refs.at(sky.texture_mappings[i]));
	}
	
	// Create the placeholder material for untextured shells.
	GLTF::Material& gouraud = gltf.materials.emplace_back();
	gouraud.name = "gouraud";
	gouraud.pbr_metallic_roughness.emplace();
	gouraud.pbr_metallic_roughness->base_color_factor.emplace();
	gouraud.pbr_metallic_roughness->base_color_factor->r = sky.colour.r / 255.f;
	gouraud.pbr_metallic_roughness->base_color_factor->g = sky.colour.g / 255.f;
	gouraud.pbr_metallic_roughness->base_color_factor->b = sky.colour.b / 255.f;
	gouraud.pbr_metallic_roughness->base_color_factor->a = sky.colour.a / 255.f;
	gouraud.alpha_mode = GLTF::BLEND;
	gouraud.double_sided = true;
	
	MaterialAsset& gouraud_asset = materials.child<MaterialAsset>("gouraud");
	gouraud_asset.set_name("gouraud");
}

struct TextureLoad
{
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
		if (material.has_diffuse()) {
			const TextureAsset& texture = material.get_diffuse();
			FileReference ref = texture.src();
			s32 index = -1;
			for (s32 i = 0; i < (s32) refs.size(); i++) {
				if (refs[i].owner == ref.owner && refs[i].path == ref.path) {
					index = i;
					break;
				}
			}
			if (index == -1) {
				index = refs.size();
				refs.emplace_back(std::move((ref)));
			}
			material_to_texture[material.name()] = dest.texture_mappings.size();
			dest.texture_mappings.emplace_back(index);
		}
	});
	
	// Read in the textures from disk.
	for (FileReference& ref : refs) {
		auto stream = ref.open_binary_file_for_reading();
		Opt<Texture> texture = read_png(*stream);
		verify(texture.has_value(), "Failed to read sky texture.");
		dest.textures.emplace_back(*texture);
	}
	
	return material_to_texture;
}

static bool test_sky_asset(
	std::vector<u8>& original,
	std::vector<u8>& repacked,
	BuildConfig config,
	const char* hint,
	AssetTestMode mode)
{
	SkyHeader header = Buffer(original).read<SkyHeader>(0, "header");
	bool headers_equal = diff_buffers(original, repacked, 0, header.texture_data, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
	
	// Don't test the bounding spheres.
	std::vector<ByteRange64> ignore;
	verify(header.shell_count <= 8, "Bad shell count.");
	for (s32 i = 0; i < header.shell_count; i++) {
		s16 cluster_count = Buffer(original).read<s16>(header.shells[i], "shell header");
		for (s32 j = 0; j < cluster_count; j++) {
			s64 cluster_header_ofs = header.shells[i] + 0x10 + j * sizeof(SkyClusterHeader);
			ignore.emplace_back(cluster_header_ofs, sizeof(SkyClusterHeader::bounding_sphere));
		}
	}
	
	bool data_equal = diff_buffers(original, repacked, header.texture_data, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL, &ignore);
	return headers_equal && data_equal;
}
