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

#include <core/collada.h>
#include <engine/collision.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_collision_asset(CollisionAsset& dest, InputStream& src, BuildConfig config);
static void pack_collision_asset(OutputStream& dest, const CollisionAsset& src, BuildConfig config);

on_load(Collision, []() {
	CollisionAsset::funcs.unpack_rac1 = wrap_unpacker_func<CollisionAsset>(unpack_collision_asset);
	CollisionAsset::funcs.unpack_rac2 = wrap_unpacker_func<CollisionAsset>(unpack_collision_asset);
	CollisionAsset::funcs.unpack_rac3 = wrap_unpacker_func<CollisionAsset>(unpack_collision_asset);
	CollisionAsset::funcs.unpack_dl = wrap_unpacker_func<CollisionAsset>(unpack_collision_asset);
	
	CollisionAsset::funcs.pack_rac1 = wrap_packer_func<CollisionAsset>(pack_collision_asset);
	CollisionAsset::funcs.pack_rac2 = wrap_packer_func<CollisionAsset>(pack_collision_asset);
	CollisionAsset::funcs.pack_rac3 = wrap_packer_func<CollisionAsset>(pack_collision_asset);
	CollisionAsset::funcs.pack_dl = wrap_packer_func<CollisionAsset>(pack_collision_asset);
})

static void unpack_collision_asset(CollisionAsset& dest, InputStream& src, BuildConfig config) {
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	ColladaScene scene = read_collision(bytes);
	std::vector<u8> collada = write_collada(scene);
	collada.push_back(0);
	
	MeshAsset& mesh = dest.mesh<MeshAsset>();
	auto ref = mesh.file().write_text_file("collision.dae", (const char*) collada.data());
	mesh.set_src(ref);
	mesh.set_name(scene.meshes.at(0).name);
	
	CollectionAsset& materials = dest.materials();
	for(Material& material : scene.materials) {
		CollisionMaterialAsset& asset = materials.child<CollisionMaterialAsset>(material.name.c_str());
		asset.set_name(material.name);
		asset.set_id(material.collision_id);
	}
}

static void pack_collision_asset(OutputStream& dest, const CollisionAsset& src, BuildConfig config) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	const MeshAsset& mesh = src.get_mesh();
	std::string xml = mesh.file().read_text_file(mesh.src().path);;
	ColladaScene scene = read_collada((char*) xml.data());
	
	src.get_materials().for_each_logical_child_of_type<CollisionMaterialAsset>([&](const CollisionMaterialAsset& asset) {
		std::string name = asset.name();
		s32 id = asset.id();
		
		for(Material& material : scene.materials) {
			if(material.name == name) {
				material.collision_id = id;
			}
		}
	});
	
	std::vector<u8> bytes;
	write_collision(OutBuffer(bytes), scene, mesh.name());
	dest.write_v(bytes);
}
