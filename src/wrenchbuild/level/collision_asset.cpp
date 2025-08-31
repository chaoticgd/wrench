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

#include "collision_asset.h"

static void unpack_collision_asset(CollisionAsset& dest, InputStream& src, BuildConfig config);
static void pack_collision_asset(OutputStream& dest, const CollisionAsset& src, BuildConfig config);
static void append_collision(Mesh& dest, const CollisionAsset& src, const glm::mat4& matrix);

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

static void unpack_collision_asset(CollisionAsset& dest, InputStream& src, BuildConfig config)
{
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	CollisionOutput output = read_collision(bytes);
	std::vector<u8> collada = write_collada(output.scene);
	collada.push_back(0);
	
	MeshAsset& mesh = dest.mesh<MeshAsset>();
	auto ref = mesh.file().write_text_file("collision.dae", (const char*) collada.data());
	mesh.set_src(ref);
	mesh.set_name(output.scene.meshes.at(0).name);
	
	CollectionAsset& hero_groups = dest.hero_groups();
	s32 i = 0;
	for (const std::string& mesh : output.hero_group_meshes) {
		MeshAsset& group_mesh = hero_groups.child<MeshAsset>(stringf("%d", i++).c_str());
		group_mesh.set_src(ref);
		group_mesh.set_name(mesh);
	}
	
	CollectionAsset& materials = dest.materials();
	for (ColladaMaterial& material : output.scene.materials) {
		CollisionMaterialAsset& asset = materials.child<CollisionMaterialAsset>(material.name.c_str());
		asset.set_name(material.name);
		asset.set_id(material.collision_id);
	}
}

static void pack_collision_asset(OutputStream& dest, const CollisionAsset& src, BuildConfig config)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	pack_level_collision(dest, src, nullptr, nullptr, -1);
}

struct ColInstance
{
	s32 mesh;
	glm::mat4 matrix;
};

void pack_level_collision(
	OutputStream& dest,
	const CollisionAsset& src,
	const LevelWadAsset* level_wad,
	const Gameplay* gameplay,
	s32 chunk)
{
	ColladaScene scene;
	
	for (s32 i = 0; i < 256; i++) {
		ColladaMaterial& material = scene.materials.emplace_back();
		material.collision_id = i;
	}
	
	Mesh& mesh = scene.meshes.emplace_back();
	mesh.name = "combined";
	
	append_collision(mesh, src, glm::mat4(1.f));
	
	if (level_wad && gameplay && gameplay->level_settings.has_value()) {
		const CollectionAsset& moby_classes = level_wad->get_moby_classes();
		for (const MobyInstance& inst : opt_iterator(gameplay->moby_instances)) {
			if (inst.has_static_collision && chunk_index_from_position(inst.transform().pos(), *gameplay->level_settings) == chunk) {
				const MobyClassAsset& class_asset = moby_classes.get_child(inst.o_class()).as<MobyClassAsset>();
				append_collision(mesh, class_asset.get_static_collision(), inst.transform().matrix());
			}
		}
		
		const CollectionAsset& tie_classes = level_wad->get_tie_classes();
		for (const TieInstance& inst : opt_iterator(gameplay->tie_instances)) {
			if (inst.has_static_collision && chunk_index_from_position(inst.transform().pos(), *gameplay->level_settings) == chunk) {
				const TieClassAsset& class_asset = tie_classes.get_child(inst.o_class()).as<TieClassAsset>();
				append_collision(mesh, class_asset.get_static_collision(), inst.transform().matrix());
			}
		}
		
		const CollectionAsset& shrub_classes = level_wad->get_shrub_classes();
		for (const ShrubInstance& inst : opt_iterator(gameplay->shrub_instances)) {
			if (inst.has_static_collision && chunk_index_from_position(inst.transform().pos(), *gameplay->level_settings) == chunk) {
				const ShrubClassAsset& class_asset = shrub_classes.get_child(inst.o_class()).as<ShrubClassAsset>();
				append_collision(mesh, class_asset.get_static_collision(), inst.transform().matrix());
			}
		}
	}
	
	CollisionInput input;
	input.main_scene = &scene;
	input.main_mesh = mesh.name;
	
	std::vector<FileReference> hero_group_refs;
	std::vector<std::string> hero_group_names;
	src.get_hero_groups().for_each_logical_child_of_type<MeshAsset>([&](const MeshAsset& mesh) {
		hero_group_refs.emplace_back(mesh.src());
		hero_group_names.emplace_back(mesh.name());
	});
	
	std::vector<std::unique_ptr<ColladaScene>> hero_group_owners;
	std::vector<ColladaScene*> hero_group_scenes = read_collada_files(hero_group_owners, hero_group_refs);
	for (size_t i = 0; i < hero_group_scenes.size(); i++) {
		Mesh* mesh = hero_group_scenes[i]->find_mesh(hero_group_names[i]);
		verify(mesh, "No mesh '%s' for hero collision group.", hero_group_names[i].c_str());
		input.hero_groups.emplace_back(mesh);
	}
	
	std::vector<u8> bytes;
	write_collision(OutBuffer(bytes), input);
	dest.write_v(bytes);
}

static void append_collision(Mesh& dest, const CollisionAsset& src, const glm::mat4& matrix)
{
	const MeshAsset& mesh_asset = src.get_mesh();
	std::string xml = mesh_asset.src().read_text_file();
	ColladaScene scene = read_collada((char*) xml.data());
	
	src.get_materials().for_each_logical_child_of_type<CollisionMaterialAsset>([&](const CollisionMaterialAsset& asset) {
		std::string name = asset.name();
		s32 id = asset.id();
		
		for (ColladaMaterial& material : scene.materials) {
			if (material.name == name) {
				material.collision_id = id;
			}
		}
	});
	
	Mesh* mesh = scene.find_mesh(mesh_asset.name());
	verify(mesh, "Cannot find mesh '%s' in collision model.", mesh_asset.name());
	
	s32 vertex_base = (s32) dest.vertices.size();
	for (const Vertex& vertex_src : mesh->vertices) {
		Vertex& vertex_dest = dest.vertices.emplace_back(vertex_src);
		vertex_dest.pos = matrix * glm::vec4(vertex_dest.pos, 1.f);
	}
	
	for (const SubMesh& submesh_src : mesh->submeshes) {
		SubMesh& submesh_dest = dest.submeshes.emplace_back();
		submesh_dest.material = scene.materials.at(submesh_src.material).collision_id;
		verify(submesh_dest.material != -1, "Tried to reference collision material that doesn't exist.");
		submesh_dest.faces = submesh_src.faces;
		for (Face& face : submesh_dest.faces) {
			face.v0 += vertex_base;
			face.v1 += vertex_base;
			face.v2 += vertex_base;
			if (face.v3 > -1) {
				face.v3 += vertex_base;
			}
		}
	}
}
