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

#include "collision.h"

#include <algorithm>

#include <core/timer.h>

packed_struct(CollisionHeader,
	s32 mesh;
	s32 hero_groups;
)

packed_struct(PackedHeroCollisionGroup,
	u16 bsphere_x;
	u16 bsphere_y;
	u16 bsphere_z;
	u16 bsphere_radius;
	u16 triangle_count;
	u16 vertex_count;
	u32 data_offset;
)

packed_struct(PackedHeroCollisionVertex,
	u16 x;
	u16 y;
	u16 z;
	u16 pad = 0;
)

packed_struct(HeroCollisionTriangle,
	u8 v0;
	u8 v1;
	u8 v2;
	u8 pad = 0;
)

template <typename T>
struct CollisionList
{
	s32 coord = 0;
	s32 temp_offset = -1; // Only used during writing.
	std::vector<T> list;
};

struct CollFace
{
	CollFace() {}
	CollFace(u8 a0, u8 a1, u8 a2, u8 t) : v0(a0), v1(a1), v2(a2), v3(0), type(t), is_quad(false) {}
	CollFace(u8 a0, u8 a1, u8 a2, u8 a3, u8 t) : v0(a0), v1(a1), v2(a2), v3(a3), type(t), is_quad(true) {}
	u8 v0;
	u8 v1;
	u8 v2;
	u8 v3;
	u8 type;
	bool is_quad;
	bool alive = true;
};

// A single collision octant is 4x4x4 in metres/game units and is aligned to a
// 4x4x4 boundary.
struct CollisionOctant
{
	s32 offset = 0;
	std::vector<glm::vec3> vertices;
	std::vector<CollFace> faces;
	glm::vec3 displacement = {0, 0, 0};
};

struct HeroCollisionGroup
{
	glm::vec4 bsphere;
	std::vector<glm::vec3> vertices;
	std::vector<HeroCollisionTriangle> triangles;
};

// The octants are arranged into a tree such that a octant at position (x,y,z)
// in the grid can be accessed by taking the (z-coord)th child of the root, the
// (y-coord)th child of that node, and then the (x-coord)th child of that node.
using CollisionOctants = CollisionList<CollisionList<CollisionList<CollisionOctant>>>;

static CollisionOctants read_collision_mesh(Buffer mesh);
static void write_collision_mesh(OutBuffer dest, CollisionOctants& octants);
static std::vector<HeroCollisionGroup> read_hero_collision_groups(Buffer buffer);
static void write_hero_collision_groups(OutBuffer dest, const std::vector<HeroCollisionGroup>& groups);
static CollisionOutput collision_to_scene(const CollisionOctants& octants, const std::vector<HeroCollisionGroup>& groups);
static CollisionOctants build_collision_octants(const ColladaScene& scene, const std::string& name);
static std::vector<HeroCollisionGroup> build_hero_collision_groups(const std::vector<const Mesh*>& meshes);
static bool test_tri_octant_intersection(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
static CollisionOctant& lookup_octant(CollisionOctants& octants, s32 x, s32 y, s32 z);
static void optimise_collision(CollisionOctants& octants);
static void reduce_quads_to_tris(CollisionOctant& octant);
static void remove_killed_faces(CollisionOctant& octant);
static void remove_unreferenced_vertices(CollisionOctant& octant);

CollisionOutput read_collision(Buffer src)
{
	ERROR_CONTEXT("collision");
	
	CollisionHeader header = src.read<CollisionHeader>(0, "collision header");
	
	CollisionOctants octants;
	std::vector<HeroCollisionGroup> hero_groups;
	if (header.hero_groups != 0) {
		octants = read_collision_mesh(src.subbuf(header.mesh, header.hero_groups - header.mesh));
		hero_groups = read_hero_collision_groups(src.subbuf(header.hero_groups));
	} else {
		octants = read_collision_mesh(src.subbuf(header.mesh));
	}
	
	return collision_to_scene(octants, hero_groups);
}

void write_collision(OutBuffer dest, const CollisionInput& input)
{
	ERROR_CONTEXT("collision");
	
	CollisionOctants octants = build_collision_octants(*input.main_scene, input.main_mesh);
	std::vector<HeroCollisionGroup> hero_groups = build_hero_collision_groups(input.hero_groups);
	optimise_collision(octants);
	
	s64 header_ofs = dest.alloc<CollisionHeader>();
	
	dest.pad(0x40);
	s64 mesh_ofs = dest.tell();
	write_collision_mesh(dest, octants);
	
	s64 hero_groups_ofs = 0;
	if (!hero_groups.empty()) {
		dest.pad(0x40);
		hero_groups_ofs = dest.tell();
		write_hero_collision_groups(dest, hero_groups);
	}
	
	CollisionHeader header;
	header.mesh = (s32) mesh_ofs;
	header.hero_groups = (s32) hero_groups_ofs;
	dest.write(header_ofs, header);
}

static CollisionOctants read_collision_mesh(Buffer mesh)
{
	CollisionOctants octants;
	s32 z_coord = mesh.read<s16>(0, "z coord");
	u16 z_count = mesh.read<u16>(2, "z count");
	
	octants.coord = z_coord;
	octants.list.resize(z_count);
	
	auto z_offsets = mesh.read_multiple<u16>(4, z_count, "z offsets");
	for (s32 z = 0; z < z_count; z++) {
		s32 z_offset = ((s32) z_offsets[z]) * 4;
		if (z_offset == 0) {
			continue;
		}
		s32 y_coord = mesh.read<s16>(z_offset, "y coord");
		u16 y_count = mesh.read<u16>(z_offset + 2, "y count");
		
		auto& y_partitions = octants.list[z];
		y_partitions.coord = y_coord;
		y_partitions.list.resize(y_count);
		
		auto y_offsets = mesh.read_multiple<u32>(z_offset + 4, y_count, "y offsets");
		for (s32 y = 0; y < y_count; y++) {
			s32 y_offset = (s32) y_offsets[y];
			if (y_offset == 0) {
				continue;
			}
			s32 x_coord = mesh.read<s16>(y_offset, "x coord");
			u16 x_count = mesh.read<u16>(y_offset + 2, "x count");
			
			auto& x_partitions = y_partitions.list[y];
			x_partitions.coord = x_coord;
			x_partitions.list.resize(x_count);
			
			auto x_offsets = mesh.read_multiple<u32>(y_offset + 4, x_count, "x offsets");
			for (s32 x = 0; x < x_count; x++) {
				s32 octant_offset = x_offsets[x] >> 8;
				if (octant_offset == 0) {
					continue;
				}
				u16 vertex_count = mesh.read<u8>(octant_offset + 2, "tri count");
				u16 face_count = mesh.read<u16>(octant_offset, "tri count");
				u16 quad_count = mesh.read<u8>(octant_offset + 3, "tri count");
				verify(face_count >= quad_count, "Face count less than quad count.");
				
				CollisionOctant& octant = x_partitions.list[x];
				octant.offset = octant_offset;
				octant.vertices.reserve(vertex_count);
				octant.faces.reserve(face_count);
				
				s32 ofs = octant_offset + 4;
				for (s32 vertex = 0; vertex < vertex_count; vertex++) {
					u32 value = mesh.read<u32>(ofs, "vertex");
					// value = 0bzzzzzzzzzzzzyyyyyyyyyyxxxxxxxxxx
					//  where z, y and x are signed.
					f32 x = ((s32) (value << 22) >> 22) / 16.f;
					f32 y = ((s32) (value << 12) >> 22) / 16.f;
					f32 z = ((s32) (value << 0) >> 20) / 64.f;
					octant.vertices.emplace_back(x, y, z);
					ofs += 4;
				}
				for (s32 face = 0; face < face_count; face++) {
					u8 v0 = mesh.read<u8>(ofs, "face v0");
					u8 v1 = mesh.read<u8>(ofs + 1, "face v1");
					u8 v2 = mesh.read<u8>(ofs + 2, "face v2");
					u8 type = mesh.read<u8>(ofs + 3, "face type");
					octant.faces.emplace_back(v0, v1, v2, type);
					ofs += 4;
				}
				for (s32 quad = 0; quad < quad_count; quad++) {
					u8 v3 = mesh.read<u8>(ofs, "quad v3");
					octant.faces[quad].v3 = v3;
					octant.faces[quad].is_quad = true;
					ofs += 1;
				}
			}
		}
	}
	
	glm::vec3 displacement(0, 0, octants.coord * 4 + 2);
	for (auto& y_part : octants.list) {
		displacement.y = y_part.coord * 4 + 2;
		for (auto& x_part : y_part.list) {
			displacement.x = x_part.coord * 4 + 2;
			for (CollisionOctant& octant : x_part.list) {
				octant.displacement = displacement;
				displacement.x += 4;
			}
			displacement.y += 4;
		}
		displacement.z += 4;
	}
	
	return octants;
}

static void write_collision_mesh(OutBuffer dest, CollisionOctants& octants)
{
	s64 base_ofs = dest.tell();
	
	// Allocate all the internal nodes and fill in pointers to internal nodes.
	dest.write<s16>(octants.coord);
	verify(octants.list.size() < 65536, "Too many Z partitions (count too high).");
	dest.write<u16>(octants.list.size());
	octants.temp_offset = dest.alloc_multiple<u16>(octants.list.size());
	
	for (s32 z = 0; z < octants.list.size(); z++) {
		auto& y_partitions = octants.list[z];
		dest.pad(4);
		verify((dest.tell() - base_ofs) / 4 < 65536, "Too many Z partitions (offset too high).");
		dest.write<u16>(octants.temp_offset + z * sizeof(u16), (dest.tell() - base_ofs) / 4);
		dest.write<s16>(y_partitions.coord);
		verify(y_partitions.list.size() < 65536, "Too many Y partitions.");
		dest.write<u16>(y_partitions.list.size());
		y_partitions.temp_offset = dest.alloc_multiple<u32>(y_partitions.list.size());
	}
	
	for (s32 z = 0; z < octants.list.size(); z++) {
		auto& y_partitions = octants.list[z];
		for (s32 y = 0; y < y_partitions.list.size(); y++) {
			auto& x_partitions = y_partitions.list[y];
			if (x_partitions.list.empty()) {
				dest.write<u32>(y_partitions.temp_offset + y * 4, 0);
				continue;
			}
			
			dest.pad(4);
			dest.write<u32>(y_partitions.temp_offset + y * 4, dest.tell() - base_ofs);
			dest.write<s16>(x_partitions.coord);
			verify(x_partitions.list.size() < 65536, "Collision has too many X partitions.");
			dest.write<u16>(x_partitions.list.size());
			x_partitions.temp_offset = dest.alloc_multiple<u32>(x_partitions.list.size());
		}
	}
	
	// Write out all the octants and fill in pointers to octants.
	for (s32 z = 0; z < octants.list.size(); z++) {
		const auto& y_partitions = octants.list[z];
		for (s32 y = 0; y < y_partitions.list.size(); y++) {
			const auto& x_partitions = y_partitions.list[y];
			for (s32 x = 0; x < x_partitions.list.size(); x++) {
				const CollisionOctant& octant = x_partitions.list[x];
				if (octant.faces.empty()) {
					dest.write<u32>(x_partitions.temp_offset + x * 4, 0);
					continue;
				}
				
				s32 quad_count = 0;
				for (const CollFace& face : octant.faces) {
					if (face.is_quad) {
						quad_count++;
					}
				}
				
				dest.pad(0x10);
				s64 octant_ofs = dest.tell() - base_ofs;
				
				if (octant.vertices.size() >= 256) {
					printf("warning: Collision octant %d %d %d dropped: Too many verticies.\n", octants.coord + x, y_partitions.coord + y, x_partitions.coord + x);
					dest.write<u32>(x_partitions.temp_offset + x * 4, 0);
					continue;
				}
				if (quad_count >= 256) {
					printf("warning: Collision octant %d %d %d dropped: Too many quads.\n", octants.coord + x, y_partitions.coord + y, x_partitions.coord + x);
					dest.write<u32>(x_partitions.temp_offset + x * 4, 0);
					continue;
				}
				
				verify(octant.faces.size() < 65536, "Too many faces in octant.");
				dest.write<u16>(octant.faces.size());
				dest.write<u8>(octant.vertices.size());
				dest.write<u8>(quad_count);
				
				for (const glm::vec3& vertex : octant.vertices) {
					u32 value = 0;
					// value = 0bzzzzzzzzzzzzyyyyyyyyyyxxxxxxxxxx
					//  where z, y and x are signed.
					value |= ((((s32) (vertex.x * 16.f)) << 22) >> 22) & 0b00000000000000000000001111111111;
					value |= (((s32) (vertex.y * 16.f) << 22) >> 12) & 0b00000000000011111111110000000000;
					value |= (((s32) (vertex.z * 64.f)) << 20) & 0b11111111111100000000000000000000;
					dest.write(value);
				}
				for (const CollFace& face : octant.faces) {
					if (face.is_quad) {
						dest.write<u8>(face.v0);
						dest.write<u8>(face.v1);
						dest.write<u8>(face.v2);
						dest.write<u8>(face.type);
					}
				}
				for (const CollFace& face : octant.faces) {
					if (!face.is_quad) {
						dest.write<u8>(face.v0);
						dest.write<u8>(face.v1);
						dest.write<u8>(face.v2);
						dest.write<u8>(face.type);
					}
				}
				for (const CollFace& face : octant.faces) {
					if (face.is_quad) {
						dest.write<u8>(face.v3);
					}
				}
				
				size_t octant_size = 4 + octant.vertices.size() * 4 + octant.faces.size() * 4 + quad_count;
				if (octant_size % 0x10 != 0) {
					octant_size += 0x10 - (octant_size % 0x10);
				}
				verify(octant_size < 0x1000, "Octant too large.");
				dest.write<u32>(x_partitions.temp_offset + x * 4, (octant_size / 0x10) | (octant_ofs << 8));
			}
		}
	}
}

static std::vector<HeroCollisionGroup> read_hero_collision_groups(Buffer buffer)
{
	s32 count = buffer.read<s32>(0);
	auto packed_groups = buffer.read_multiple<PackedHeroCollisionGroup>(0x10, count, "hero collision groups");
	
	std::vector<HeroCollisionGroup> groups;
	for (const PackedHeroCollisionGroup& packed_group : packed_groups) {
		HeroCollisionGroup& group = groups.emplace_back();
		group.bsphere.x = packed_group.bsphere_x * (1.f / 64.f);
		group.bsphere.y = packed_group.bsphere_y * (1.f / 64.f);
		group.bsphere.z = packed_group.bsphere_z * (1.f / 64.f);
		group.bsphere.w = packed_group.bsphere_radius * (1.f / 64.f);
		
		s64 ofs = packed_group.data_offset;
		
		for (s32 i = 0; i < packed_group.vertex_count; i++) {
			const PackedHeroCollisionVertex& packed_vertex = buffer.read<PackedHeroCollisionVertex>(ofs);
			verify(packed_vertex.pad == 0, "Unknown type of hero collision vertex.");
			ofs += 8;
			
			glm::vec3& vertex = group.vertices.emplace_back();
			vertex.x = packed_vertex.x * (1.f / 64.f);
			vertex.y = packed_vertex.y * (1.f / 64.f);
			vertex.z = packed_vertex.z * (1.f / 64.f);
		}
		
		group.triangles = buffer.read_multiple<HeroCollisionTriangle>(ofs, packed_group.triangle_count, "hero collision triangles").copy();
		for (const HeroCollisionTriangle& triangle : group.triangles) {
			verify(triangle.pad == 0, "Unknown type of hero collision triangle.");
		}
	}
	
	return groups;
}

static void write_hero_collision_groups(OutBuffer dest, const std::vector<HeroCollisionGroup>& groups)
{
	s64 begin_ofs = dest.tell();
	dest.write((s32) groups.size());
	
	dest.pad(0x10);
	s64 group_array_ofs = dest.alloc_multiple<PackedHeroCollisionGroup>((s64) groups.size());
	
	std::vector<PackedHeroCollisionGroup> packed_groups;
	for (const HeroCollisionGroup& group : groups) {
		dest.pad(0x10);
		
		verify(group.vertices.size() <= UINT16_MAX, "Too many vertices in hero collision group.");
		verify(group.triangles.size() <= UINT16_MAX, "Too many triangles in hero collision group.");
		
		PackedHeroCollisionGroup& packed_group = packed_groups.emplace_back();
		packed_group.bsphere_x = (u16) (group.bsphere.x * 64.f);
		packed_group.bsphere_y = (u16) (group.bsphere.y * 64.f);
		packed_group.bsphere_z = (u16) (group.bsphere.z * 64.f);
		packed_group.bsphere_radius = (u16) (group.bsphere.w * 64.f);
		packed_group.triangle_count = (u16) group.triangles.size();
		packed_group.vertex_count = (u16) group.vertices.size();
		packed_group.data_offset = (u32) (dest.tell() - begin_ofs);
		
		for (const glm::vec3& vertex : group.vertices) {
			PackedHeroCollisionVertex packed_vertex;
			packed_vertex.x = (u16) vertex.x * 64.f;
			packed_vertex.y = (u16) vertex.y * 64.f;
			packed_vertex.z = (u16) vertex.z * 64.f;
			dest.write(packed_vertex);
		}
		
		dest.write_multiple(group.triangles);
	}
	
	dest.write_multiple(group_array_ofs, packed_groups);
}

static CollisionOutput collision_to_scene(const CollisionOctants& octants, const std::vector<HeroCollisionGroup>& groups)
{
	CollisionOutput output;

	Mesh& mesh = output.scene.meshes.emplace_back();
	mesh.name = "collision";
	mesh.flags = MESH_HAS_QUADS;
	Opt<size_t> submeshes[256];
	
	output.scene.materials = create_collision_materials();
	
	for (const auto& y_partitions : octants.list) {
		for (const auto& x_partitions : y_partitions.list) {
			for (const CollisionOctant& octant : x_partitions.list) {
				s32 base = mesh.vertices.size();
				for (const glm::vec3& vertex : octant.vertices) {
					mesh.vertices.emplace_back(octant.displacement + vertex);
				}
				for (const CollFace& face : octant.faces) {
					if (!submeshes[face.type].has_value()) {
						submeshes[face.type] = mesh.submeshes.size();
						mesh.submeshes.emplace_back().material = face.type;
					}
					auto& face_list = mesh.submeshes[*submeshes[face.type]].faces;
					if (face.is_quad) {
						face_list.emplace_back(base + face.v3, base + face.v2, base + face.v1, base + face.v0);
					} else {
						face_list.emplace_back(base + face.v2, base + face.v1, base + face.v0, -1);
					}
				}
			}
		}
	}
	
	// The vertices and faces stored in the games files are duplicated such that
	// only one octant must be accessed to do collision detection.
	output.scene.meshes[0] = deduplicate_vertices(std::move(output.scene.meshes[0]));
	output.scene.meshes[0] = deduplicate_faces(std::move(output.scene.meshes[0]));
	
	s32 group_index = 0;
	for (const HeroCollisionGroup& group : groups) {
		Mesh& group_mesh = output.scene.meshes.emplace_back();
		group_mesh.name = stringf("hero_collision_group_%d", group_index++);
		for(const glm::vec3& vertex : group.vertices) {
			group_mesh.vertices.emplace_back(vertex);
		}
		
		SubMesh& submesh = group_mesh.submeshes.emplace_back();
		submesh.material = 256; // hero_group_collision
		for (const HeroCollisionTriangle& triangle : group.triangles) {
			Face& face = submesh.faces.emplace_back();
			face.v0 = triangle.v0;
			face.v1 = triangle.v1;
			face.v2 = triangle.v2;
			
			verify(face.v0 < group.vertices.size() && face.v1 < group.vertices.size() && face.v2 < group.vertices.size(),
				"Hero collision triangle references out of bounds vertex.");
		}
		
		output.hero_group_meshes.emplace_back(group_mesh.name);
	}
	
	return output;
}

std::vector<ColladaMaterial> create_collision_materials()
{
	std::vector<ColladaMaterial> materials;
	
	for (s32 i = 0; i < 256; i++) {
		ColladaMaterial& material = materials.emplace_back();
		material.name = stringf("col_%x", i);
		material.surface.type = MaterialSurfaceType::COLOUR;
		// Colouring logic taken from Replanetizer:
		// https://github.com/RatchetModding/replanetizer/blob/ada7ca73418d7b01cc70eec58a41238986b84112/LibReplanetizer/Models/Collision.cs#L26
		material.surface.colour.r = ((i & 0x3) << 6) / 255.0;
		material.surface.colour.g = ((i & 0xc) << 4) / 255.0;
		material.surface.colour.b = (i & 0xf0) / 255.0;
		material.surface.colour.a = 1.f;
		material.collision_id = i;
	}
	
	ColladaMaterial& hero_group_collision = materials.emplace_back();
	hero_group_collision.name = "hero_group_collision";
	hero_group_collision.surface.type = MaterialSurfaceType::COLOUR;
	hero_group_collision.surface.colour.r = 0.f;
	hero_group_collision.surface.colour.g = 0.f;
	hero_group_collision.surface.colour.b = 1.f;
	hero_group_collision.surface.colour.a = 1.f;
	
	return materials;
}

static CollisionOctants build_collision_octants(const ColladaScene& scene, const std::string& name)
{
	start_timer("build collision");
	
	CollisionOctants octants;
	for (const Mesh& mesh : scene.meshes) {
		if (mesh.name != name) {
			continue;
		}
		
		for (const SubMesh& submesh : mesh.submeshes) {
			const ColladaMaterial& material = scene.materials.at(submesh.material);
			verify(material.collision_id >= 0 && material.collision_id <= 255,
				"Invalid collision ID.");
			u8 type = (u8) material.collision_id;
			
			for (const Face& face : submesh.faces) {
				const glm::vec3* verts[4] = {
					&mesh.vertices.at(face.v0).pos,
					&mesh.vertices.at(face.v1).pos,
					&mesh.vertices.at(face.v2).pos,
					nullptr
				};
				if (face.is_quad()) {
					verts[3] = &mesh.vertices.at(face.v3).pos;
				} else {
					verts[3] = verts[0];
				}
				
				// Find the minimum axis-aligned bounding box of the face on the
				// octant grid.
				s32 zmin = INT32_MAX, zmax = 0;
				s32 ymin = INT32_MAX, ymax = 0;
				s32 xmin = INT32_MAX, xmax = 0;
				for (const glm::vec3* v : verts) {
					s32 vzmin = (s32) (v->z * 0.25f);
					s32 vymin = (s32) (v->y * 0.25f);
					s32 vxmin = (s32) (v->x * 0.25f);
					s32 vzmax = (s32) ceilf(v->z * 0.25f);
					s32 vymax = (s32) ceilf(v->y * 0.25f);
					s32 vxmax = (s32) ceilf(v->x * 0.25f);
					if (vzmin < zmin) zmin = vzmin;
					if (vymin < ymin) ymin = vymin;
					if (vxmin < xmin) xmin = vxmin;
					if (vzmax > zmax) zmax = vzmax;
					if (vymax > ymax) ymax = vymax;
					if (vxmax > xmax) xmax = vxmax;
				}
				
				if (zmin == zmax) { zmin--; zmax++; }
				if (ymin == ymax) { ymin--; ymax++; }
				if (xmin == xmax) { xmin--; xmax++; }
				
				if (xmin < 0) xmin = 0;
				if (ymin < 0) ymin = 0;
				if (zmin < 0) zmin = 0;
				
				// Iterate over the bounding box of octants that could contain
				// the current face and check which ones actually do. If a
				// octant does contain said face, add the vertices/faces to the
				// octant. Add new octants to the tree as needed.
				s32 inserts = 0;
				for (s32 z = zmin; z < zmax; z++) {
					for (s32 y = ymin; y < ymax; y++) {
						for (s32 x = xmin; x < xmax; x++) {
							auto disp = glm::vec3(x * 4 + 2, y * 4 + 2, z * 4 + 2);
							bool accept = false;
							accept |= test_tri_octant_intersection(*verts[0] - disp, *verts[1] - disp, *verts[2] - disp);
							if (face.is_quad()) {
								accept |= test_tri_octant_intersection(*verts[2] - disp, *verts[3] - disp, *verts[0] - disp);
							}
							if (accept) {
								s32 octant_inds[4] = {-1, -1, -1, -1};
								CollisionOctant& octant = lookup_octant(octants, x, y, z);
								
								// Merge vertices.
								for (s32 i = 0; i < (face.is_quad() ? 4 : 3); i++) {
									glm::vec3 pos = *verts[i] - disp;
									for (size_t j = 0; j < octant.vertices.size(); j++) {
										if (vec3_equal_eps(pos, octant.vertices[j])) {
											octant_inds[i] = j;
										}
									}
									if (octant_inds[i] == -1) {
										octant_inds[i] = octant.vertices.size();
										octant.vertices.push_back(pos);
									}
								}
								
								if (octant_inds[3] > -1) {
									octant.faces.emplace_back(octant_inds[3], octant_inds[2], octant_inds[1], octant_inds[0], type);
								} else {
									octant.faces.emplace_back(octant_inds[2], octant_inds[1], octant_inds[0], type);
								}
								inserts++;
							}
						}
					}
				}
				verify_fatal(inserts > 0);
			}
		}
	}
	
	stop_timer();
	return octants;
}

static std::vector<HeroCollisionGroup> build_hero_collision_groups(const std::vector<const Mesh*>& meshes)
{
	std::vector<HeroCollisionGroup> groups;
	
	for (const Mesh* mesh : meshes) {
		HeroCollisionGroup& group = groups.emplace_back();
		
		group.bsphere = approximate_bounding_sphere(mesh->vertices);
		
		for (const Vertex& vertex : mesh->vertices) {
			group.vertices.emplace_back(vertex.pos);
		}
		
		for (const SubMesh& submesh : mesh->submeshes) {
			for (const Face& face : submesh.faces) {
				HeroCollisionTriangle& triangle = group.triangles.emplace_back();
				triangle.v0 = face.v0;
				triangle.v1 = face.v1;
				triangle.v2 = face.v2;
				if (face.is_quad()) {
					HeroCollisionTriangle& triangle = group.triangles.emplace_back();
					triangle.v0 = face.v2;
					triangle.v1 = face.v3;
					triangle.v2 = face.v0;
				}
			}
		}
	}
	
	return groups;
}

// https://gdbooks.gitbooks.io/3dcollisions/content/Chapter4/aabb-triangle.html
static bool test_tri_octant_intersection(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
	glm::vec3 f0 = v1 - v0;
	glm::vec3 f1 = v2 - v1;
	glm::vec3 f2 = v0 - v2;
	
	glm::vec3 u0(1.f, 0.f, 0.f);
	glm::vec3 u1(0.f, 1.f, 0.f);
	glm::vec3 u2(0.f, 0.f, 1.f);
	
	auto axes = {
		glm::cross(u0, f0), glm::cross(u0, f1), glm::cross(u0, f2),
		glm::cross(u1, f0), glm::cross(u1, f1), glm::cross(u1, f2),
		glm::cross(u2, f0), glm::cross(u2, f1), glm::cross(u2, f2),
		u0, u1, u2, glm::cross(f0, f1)
	};
	
	for (const glm::vec3& axis : axes) {
		f32 p0 = glm::dot(v0, axis);
		f32 p1 = glm::dot(v1, axis);
		f32 p2 = glm::dot(v2, axis);
		float r = 0.f;
		r += fabs(glm::dot(u0, axis));
		r += fabs(glm::dot(u1, axis));
		r += fabs(glm::dot(u2, axis));
		r *= 2.f;
		if (std::max(-std::max({p0, p1, p2}), std::min({p0, p1, p2})) > r) {
			return false;
		}
	}
	
	return true;
}

static CollisionOctant& lookup_octant(CollisionOctants& octants, s32 x, s32 y, s32 z)
{
	if (octants.list.empty()) {
		octants.list.resize(1);
		octants.coord = z;
	} else if (z < octants.coord) {
		octants.list.insert(octants.list.begin(), octants.coord - z, {});
		octants.coord = z;
	} else if (z >= octants.coord + octants.list.size()) {
		octants.list.resize(z - octants.coord + 1);
	}
	CollisionList<CollisionList<CollisionOctant>>& z_node = octants.list[z - octants.coord];
	if (z_node.list.empty()) {
		z_node.list.resize(1);
		z_node.coord = y;
	} else if (y < z_node.coord) {
		z_node.list.insert(z_node.list.begin(), z_node.coord - y, {});
		z_node.coord = y;
	} else if (y >= z_node.coord + z_node.list.size()) {
		z_node.list.resize(y - z_node.coord + 1);
	}
	CollisionList<CollisionOctant>& y_node = z_node.list[y - z_node.coord];
	if (y_node.list.empty()) {
		y_node.list.resize(1);
		y_node.coord = x;
	} else if (x < y_node.coord) {
		y_node.list.insert(y_node.list.begin(), y_node.coord - x, {});
		y_node.coord = x;
	} else if (x >= y_node.coord + y_node.list.size()) {
		y_node.list.resize(x - y_node.coord + 1);
	}
	return y_node.list[x - y_node.coord];
}

static void optimise_collision(CollisionOctants& octants)
{
	start_timer("Optimising collision tree");
	
	for (auto& y_partitions : octants.list) {
		for (auto& x_partitions : y_partitions.list) {
			for (CollisionOctant& octant : x_partitions.list) {
				reduce_quads_to_tris(octant);
				remove_killed_faces(octant);
				remove_unreferenced_vertices(octant);
			}
		}
	}
	
	stop_timer();
}

static void reduce_quads_to_tris(CollisionOctant& octant)
{
	size_t face_count = octant.faces.size();
	for (size_t i = 0; i < face_count; i++) {
		CollFace& face = octant.faces[i];
		if (face.is_quad && face.alive) {
			glm::vec3& v0 = octant.vertices[face.v0];
			glm::vec3& v1 = octant.vertices[face.v1];
			glm::vec3& v2 = octant.vertices[face.v2];
			glm::vec3& v3 = octant.vertices[face.v3];
			bool i0 = test_tri_octant_intersection(v0, v1, v2);
			bool i2 = test_tri_octant_intersection(v2, v3, v0);
			if (i0 && !i2) {
				face.alive = false;
				octant.faces.emplace_back(face.v0, face.v1, face.v2, face.type);
			} else if (i2 && !i0) {
				face.alive = false;
				octant.faces.emplace_back(face.v2, face.v3, face.v0, face.type);
			} else {
				bool i1 = test_tri_octant_intersection(v1, v2, v3);
				bool i3 = test_tri_octant_intersection(v3, v0, v1);
				if (i1 && !i3) {
					face.alive = false;
					octant.faces.emplace_back(face.v1, face.v2, face.v3, face.type);
				} else if (i3 && !i1) {
					face.alive = false;
					octant.faces.emplace_back(face.v3, face.v0, face.v1, face.type);
				}
			}
		}
	}
}

static void remove_killed_faces(CollisionOctant& octant)
{
	std::vector<CollFace> new_faces;
	for (CollFace& face : octant.faces) {
		if (face.alive) {
			new_faces.push_back(face);
		}
	}
	octant.faces = std::move(new_faces);
}

static void remove_unreferenced_vertices(CollisionOctant& octant)
{
	std::vector<glm::vec3> new_vertices;
	std::vector<size_t> new_indices(octant.vertices.size(), SIZE_MAX);
	for (CollFace& face : octant.faces) {
		u8* inds[4] = {&face.v0, &face.v1, &face.v2, &face.v3};
		for (s32 i = 0; i < (face.is_quad ? 4 : 3); i++) {
			if (new_indices[*inds[i]] == SIZE_MAX) {
				new_indices[*inds[i]] = new_vertices.size();
				new_vertices.push_back(octant.vertices[*inds[i]]);
			}
			*inds[i] = new_indices[*inds[i]];
		}
	}
	octant.vertices = std::move(new_vertices);
}
