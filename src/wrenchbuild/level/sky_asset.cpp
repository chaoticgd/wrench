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
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config);
static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config);

on_load(Sky, []() {
	SkyAsset::funcs.unpack_rac1 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_rac2 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_rac3 = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	SkyAsset::funcs.unpack_dl = wrap_unpacker_func<SkyAsset>(unpack_sky_asset);
	
	SkyAsset::funcs.pack_rac1 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_rac2 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_rac3 = wrap_packer_func<SkyAsset>(pack_sky_asset);
	SkyAsset::funcs.pack_dl = wrap_packer_func<SkyAsset>(pack_sky_asset);
})

static void unpack_sky_asset(SkyAsset& dest, InputStream& src, BuildConfig config) {
	std::vector<u8> buffer = src.read_multiple<u8>(src.size());
	Sky sky = read_sky(buffer, config.game());
	
	ColladaScene scene;
	
	Material& dummy = scene.materials.emplace_back();
	dummy.name = "dummy";
	dummy.colour = glm::vec4(1.f, 0.f, 1.f, 1.f);
	
	for(s32 i = 0; i < sky.textures.size(); i++) {
		auto [stream, ref] = dest.file().open_binary_file_for_writing(stringf("%d.png", i));
		write_png(*stream, sky.textures[i]);
		
		Material& mat = scene.materials.emplace_back();
		mat.name = stringf("texture_%d", i);
		mat.texture = i;
		
		scene.texture_paths.emplace_back(ref.path.string());
	}
	
	for(size_t i = 0; i < sky.shells.size(); i++) {
		Mesh& mesh = sky.shells[i].mesh;
		mesh.name = stringf("shell_%d", i);
		scene.meshes.emplace_back(std::move(mesh));
	}
	
	std::vector<u8> collada = write_collada(scene);
	auto [stream, ref] = dest.file().open_binary_file_for_writing("mesh.dae");
	stream->write_v(collada);
	
	CollectionAsset& shells = dest.shells();
	for(size_t i = 0; i < sky.shells.size(); i++) {
		SkyShellAsset& shell = shells.child<SkyShellAsset>(i);
		shell.mesh().set_name(stringf("shell_%d", i));
		shell.mesh().set_src(ref);
	}
}

static void pack_sky_asset(OutputStream& dest, const SkyAsset& src, BuildConfig config) {
	
}
