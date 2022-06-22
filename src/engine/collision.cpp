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

#include <core/timer.h>

packed_struct(CollisionHeader,
	s32 mesh;
	s32 second_part;
)

template <typename T>
struct CollisionList {
	s32 coord = 0;
	s32 temp_offset = -1; // Only used during writing.
	std::vector<T> list;
};

struct CollFace {
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

// A single collision sector is 4x4x4 in metres/game units and is aligned to a
// 4x4x4 boundary.
struct CollisionSector {
	s32 offset = 0;
	std::vector<glm::vec3> vertices;
	std::vector<CollFace> faces;
	glm::vec3 displacement = {0, 0, 0};
};

// The sectors are arranged into a tree such that a sector at position (x,y,z)
// in the grid can be accessed by taking the (z-coord)th child of the root, the
// (y-coord)th child of that node, and then the (x-coord)th child of that node.
using CollisionSectors = CollisionList<CollisionList<CollisionList<CollisionSector>>>;

static CollisionSectors parse_collision_mesh(Buffer mesh);
static void write_collision_mesh(OutBuffer dest, CollisionSectors& sectors);
static ColladaScene collision_sectors_to_scene(const CollisionSectors& sectors);
static CollisionSectors build_collision_sectors(const ColladaScene& scene);
static bool test_tri_sector_intersection(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);
static CollisionSector& lookup_sector(CollisionSectors& sectors, s32 x, s32 y, s32 z);
static void optimise_collision(CollisionSectors& sectors);
static void reduce_quads_to_tris(CollisionSector& sector);
static void remove_killed_faces(CollisionSector& sector);
static void remove_unreferenced_vertices(CollisionSector& sector);

ColladaScene read_collision(Buffer src) {
	ERROR_CONTEXT("collision");
	
	CollisionHeader header = src.read<CollisionHeader>(0, "collision header");
	Buffer mesh_buffer;
	if(header.second_part != 0) {
		mesh_buffer = src.subbuf(header.mesh, header.second_part - header.mesh);
	} else {
		mesh_buffer = src.subbuf(header.mesh);
	}
	CollisionSectors sectors = parse_collision_mesh(mesh_buffer);
	return collision_sectors_to_scene(sectors);
}

void write_collision(OutBuffer dest, const ColladaScene& scene) {
	ERROR_CONTEXT("collision");
	
	CollisionSectors sectors = build_collision_sectors(scene);
	optimise_collision(sectors);
	CollisionHeader header;
	header.mesh = 0x40;
	header.second_part = 0;
	assert(dest.tell() % 0x40 == 0);
	dest.write(header);
	dest.pad(0x40);
	write_collision_mesh(dest, sectors);
}

static CollisionSectors parse_collision_mesh(Buffer mesh) {
	CollisionSectors sectors;
	s32 z_coord = mesh.read<s16>(0, "z coord");
	u16 z_count = mesh.read<u16>(2, "z count");
	
	sectors.coord = z_coord;
	sectors.list.resize(z_count);
	
	auto z_offsets = mesh.read_multiple<u16>(4, z_count, "z offsets");
	for(s32 z = 0; z < z_count; z++) {
		s32 z_offset = ((s32) z_offsets[z]) * 4;
		if(z_offset == 0) {
			continue;
		}
		s32 y_coord = mesh.read<s16>(z_offset, "y coord");
		u16 y_count = mesh.read<u16>(z_offset + 2, "y count");
		
		auto& y_partitions = sectors.list[z];
		y_partitions.coord = y_coord;
		y_partitions.list.resize(y_count);
		
		auto y_offsets = mesh.read_multiple<u32>(z_offset + 4, y_count, "y offsets");
		for(s32 y = 0; y < y_count; y++) {
			s32 y_offset = (s32) y_offsets[y];
			if(y_offset == 0) {
				continue;
			}
			s32 x_coord = mesh.read<s16>(y_offset, "x coord");
			u16 x_count = mesh.read<u16>(y_offset + 2, "x count");
			
			auto& x_partitions = y_partitions.list[y];
			x_partitions.coord = x_coord;
			x_partitions.list.resize(x_count);
			
			auto x_offsets = mesh.read_multiple<u32>(y_offset + 4, x_count, "x offsets");
			for(s32 x = 0; x < x_count; x++) {
				s32 sector_offset = x_offsets[x] >> 8;
				if(sector_offset == 0) {
					continue;
				}
				u16 vertex_count = mesh.read<u8>(sector_offset + 2, "tri count");
				u16 face_count = mesh.read<u16>(sector_offset, "tri count");
				u16 quad_count = mesh.read<u8>(sector_offset + 3, "tri count");
				verify(face_count >= quad_count, "Face count less than quad count.");
				
				CollisionSector& sector = x_partitions.list[x];
				sector.offset = sector_offset;
				sector.vertices.reserve(vertex_count);
				sector.faces.reserve(face_count);
				
				s32 ofs = sector_offset + 4;
				for(s32 vertex = 0; vertex < vertex_count; vertex++) {
					u32 value = mesh.read<u32>(ofs, "vertex");
					// value = 0bzzzzzzzzzzzzyyyyyyyyyyxxxxxxxxxx
					//  where z, y and x are signed.
					f32 x = ((s32) (value << 22) >> 22) / 16.f;
					f32 y = ((s32) (value << 12) >> 22) / 16.f;
					f32 z = ((s32) (value << 0) >> 20) / 64.f;
					sector.vertices.emplace_back(x, y, z);
					ofs += 4;
				}
				for(s32 face = 0; face < face_count; face++) {
					u8 v0 = mesh.read<u8>(ofs, "face v0");
					u8 v1 = mesh.read<u8>(ofs + 1, "face v1");
					u8 v2 = mesh.read<u8>(ofs + 2, "face v2");
					u8 type = mesh.read<u8>(ofs + 3, "face type");
					sector.faces.emplace_back(v0, v1, v2, type);
					ofs += 4;
				}
				for(s32 quad = 0; quad < quad_count; quad++) {
					u8 v3 = mesh.read<u8>(ofs, "quad v3");
					sector.faces[quad].v3 = v3;
					sector.faces[quad].is_quad = true;
					ofs += 1;
				}
			}
		}
	}
	
	glm::vec3 displacement(0, 0, sectors.coord * 4 + 2);
	for(auto& y_part : sectors.list) {
		displacement.y = y_part.coord * 4 + 2;
		for(auto& x_part : y_part.list) {
			displacement.x = x_part.coord * 4 + 2;
			for(CollisionSector& sector : x_part.list) {
				sector.displacement = displacement;
				displacement.x += 4;
			}
			displacement.y += 4;
		}
		displacement.z += 4;
	}
	
	return sectors;
}

static void write_collision_mesh(OutBuffer dest, CollisionSectors& sectors) {
	s64 base_ofs = dest.tell();
	
	// Allocate all the internal nodes and fill in pointers to internal nodes.
	dest.write<s16>(sectors.coord);
	verify(sectors.list.size() < 65536, "Too many Z partitions (count too high).");
	dest.write<u16>(sectors.list.size());
	sectors.temp_offset = dest.alloc_multiple<u16>(sectors.list.size());
	
	for(s32 z = 0; z < sectors.list.size(); z++) {
		auto& y_partitions = sectors.list[z];
		dest.pad(4);
		verify((dest.tell() - base_ofs) / 4 < 65536, "Too many Z partitions (offset too high).");
		dest.write<u16>(sectors.temp_offset + z * sizeof(u16), (dest.tell() - base_ofs) / 4);
		dest.write<s16>(y_partitions.coord);
		verify(y_partitions.list.size() < 65536, "Too many Y partitions.");
		dest.write<u16>(y_partitions.list.size());
		y_partitions.temp_offset = dest.alloc_multiple<u32>(y_partitions.list.size());
	}
	
	for(s32 z = 0; z < sectors.list.size(); z++) {
		auto& y_partitions = sectors.list[z];
		for(s32 y = 0; y < y_partitions.list.size(); y++) {
			auto& x_partitions = y_partitions.list[y];
			if(x_partitions.list.empty()) {
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
	
	// Write out all the sectors and fill in pointers to sectors.
	for(s32 z = 0; z < sectors.list.size(); z++) {
		const auto& y_partitions = sectors.list[z];
		for(s32 y = 0; y < y_partitions.list.size(); y++) {
			const auto& x_partitions = y_partitions.list[y];
			for(s32 x = 0; x < x_partitions.list.size(); x++) {
				const CollisionSector& sector = x_partitions.list[x];
				if(sector.faces.empty()) {
					dest.write<u32>(x_partitions.temp_offset + x * 4, 0);
					continue;
				}
				
				s32 quad_count = 0;
				for(const CollFace& face : sector.faces) {
					if(face.is_quad) {
						quad_count++;
					}
				}
				
				dest.pad(0x10);
				s64 sector_ofs = dest.tell() - base_ofs;
				
				if(sector.vertices.size() >= 256) {
					printf("warning: Collision sector %d %d %d dropped: Too many verticies.\n", sectors.coord + x, y_partitions.coord + y, x_partitions.coord + x);
					dest.write<u32>(x_partitions.temp_offset + x * 4, 0);
					continue;
				}
				if(quad_count >= 256) {
					printf("warning: Collision sector %d %d %d dropped: Too many quads.\n", sectors.coord + x, y_partitions.coord + y, x_partitions.coord + x);
					dest.write<u32>(x_partitions.temp_offset + x * 4, 0);
					continue;
				}
				
				verify(sector.faces.size() < 65536, "Too many faces in sector.");
				dest.write<u16>(sector.faces.size());
				dest.write<u8>(sector.vertices.size());
				dest.write<u8>(quad_count);
				
				for(const glm::vec3& vertex : sector.vertices) {
					u32 value = 0;
					// value = 0bzzzzzzzzzzzzyyyyyyyyyyxxxxxxxxxx
					//  where z, y and x are signed.
					value |= ((((s32) (vertex.x * 16.f)) << 22) >> 22) & 0b00000000000000000000001111111111;
					value |= (((s32) (vertex.y * 16.f) << 22) >> 12) & 0b00000000000011111111110000000000;
					value |= (((s32) (vertex.z * 64.f)) << 20) & 0b11111111111100000000000000000000;
					dest.write(value);
				}
				for(const CollFace& face : sector.faces) {
					if(face.is_quad) {
						dest.write<u8>(face.v0);
						dest.write<u8>(face.v1);
						dest.write<u8>(face.v2);
						dest.write<u8>(face.type);
					}
				}
				for(const CollFace& face : sector.faces) {
					if(!face.is_quad) {
						dest.write<u8>(face.v0);
						dest.write<u8>(face.v1);
						dest.write<u8>(face.v2);
						dest.write<u8>(face.type);
					}
				}
				for(const CollFace& face : sector.faces) {
					if(face.is_quad) {
						dest.write<u8>(face.v3);
					}
				}
				
				size_t sector_size = 4 + sector.vertices.size() * 4 + sector.faces.size() * 4 + quad_count;
				if(sector_size % 0x10 != 0) {
					sector_size += 0x10 - (sector_size % 0x10);
				}
				verify(sector_size < 0x1000, "Sector too large.");
				dest.write<u32>(x_partitions.temp_offset + x * 4, (sector_size / 0x10) | (sector_ofs << 8));
			}
		}
	}
}

static ColladaScene collision_sectors_to_scene(const CollisionSectors& sectors) {
	ColladaScene scene;

	Mesh& mesh = scene.meshes.emplace_back();
	mesh.name = "collision";
	mesh.flags = MESH_HAS_QUADS;
	Opt<size_t> submeshes[256];
	
	for(s32 i = 0; i < 256; i++) {
		Material material;
		material.name = stringf("col_%x", i);
		material.colour = glm::vec4{};
		// From https://github.com/RatchetModding/replanetizer/blob/ada7ca73418d7b01cc70eec58a41238986b84112/LibReplanetizer/Models/Collision.cs#L26
		// Colour different types of collision without knowing what they are.
		material.colour->r = ((i & 0x3) << 6) / 255.0;
		material.colour->g = ((i & 0xc) << 4) / 255.0;
		material.colour->b = (i & 0xf0) / 255.0;
		material.colour->a = 1.f;
		material.collision_id = i;
		scene.materials.emplace_back(std::move(material));
	}
	for(const auto& y_partitions : sectors.list) {
		for(const auto& x_partitions : y_partitions.list) {
			for(const CollisionSector& sector : x_partitions.list) {
				s32 base = mesh.vertices.size();
				for(const glm::vec3& vertex : sector.vertices) {
					mesh.vertices.emplace_back(sector.displacement + vertex);
				}
				for(const CollFace& face : sector.faces) {
					if(!submeshes[face.type].has_value()) {
						submeshes[face.type] = mesh.submeshes.size();
						mesh.submeshes.emplace_back().material = face.type;
					}
					auto& face_list = mesh.submeshes[*submeshes[face.type]].faces;
					if(face.is_quad) {
						face_list.emplace_back(base + face.v3, base + face.v2, base + face.v1, base + face.v0);
					} else {
						face_list.emplace_back(base + face.v2, base + face.v1, base + face.v0, -1);
					}
				}
			}
		}
	}
	
	// The vertices and faces stored in the games files are duplicated such that
	// only one sector must be accessed to do collision detection.
	scene.meshes[0] = deduplicate_vertices(std::move(scene.meshes[0]));
	scene.meshes[0] = deduplicate_faces(std::move(scene.meshes[0]));
	
	return scene;
}

static CollisionSectors build_collision_sectors(const ColladaScene& scene) {
	start_timer("build collision");
	
	CollisionSectors sectors;
	for(const Mesh& mesh : scene.meshes) {
		for(const SubMesh& submesh : mesh.submeshes) {
			const Material& material = scene.materials[submesh.material];
			verify(material.collision_id >= 0 && material.collision_id <= 255,
				"Invalid collision ID.");
			u8 type = (u8) material.collision_id;
			
			for(const Face& face : submesh.faces) {
				const glm::vec3* verts[4] = {
					&mesh.vertices.at(face.v0).pos,
					&mesh.vertices.at(face.v1).pos,
					&mesh.vertices.at(face.v2).pos,
					nullptr
				};
				if(face.is_quad()) {
					verts[3] = &mesh.vertices.at(face.v3).pos;
				} else {
					verts[3] = verts[0];
				}
				
				// Find the minimum axis-aligned bounding box of the face on the
				// sector grid.
				s32 zmin = INT32_MAX, zmax = 0;
				s32 ymin = INT32_MAX, ymax = 0;
				s32 xmin = INT32_MAX, xmax = 0;
				for(const glm::vec3* v : verts) {
					s32 vzmin = (s32) (v->z * 0.25f);
					s32 vymin = (s32) (v->y * 0.25f);
					s32 vxmin = (s32) (v->x * 0.25f);
					s32 vzmax = (s32) ceilf(v->z * 0.25f);
					s32 vymax = (s32) ceilf(v->y * 0.25f);
					s32 vxmax = (s32) ceilf(v->x * 0.25f);
					if(vzmin < zmin) zmin = vzmin;
					if(vymin < ymin) ymin = vymin;
					if(vxmin < xmin) xmin = vxmin;
					if(vzmax > zmax) zmax = vzmax;
					if(vymax > ymax) ymax = vymax;
					if(vxmax > xmax) xmax = vxmax;
				}
				
				if(zmin == zmax) { zmin--; zmax++; }
				if(ymin == ymax) { ymin--; ymax++; }
				if(xmin == xmax) { xmin--; xmax++; }
				
				// Iterate over the bounding box of sectors that could contain
				// the current face and check which ones actually do. If a
				// sector does contain said face, add the vertices/faces to the
				// sector. Add new sectors to the tree as needed.
				s32 inserts = 0;
				for(s32 z = zmin; z < zmax; z++) {
					for(s32 y = ymin; y < ymax; y++) {
						for(s32 x = xmin; x < xmax; x++) {
							auto disp = glm::vec3(x * 4 + 2, y * 4 + 2, z * 4 + 2);
							bool accept = false;
							accept |= test_tri_sector_intersection(*verts[0] - disp, *verts[1] - disp, *verts[2] - disp);
							if(face.is_quad()) {
								accept |= test_tri_sector_intersection(*verts[2] - disp, *verts[3] - disp, *verts[0] - disp);
							}
							if(accept) {
								s32 sector_inds[4] = {-1, -1, -1, -1};
								CollisionSector& sector = lookup_sector(sectors, x, y, z);
								
								// Merge vertices.
								for(s32 i = 0; i < (face.is_quad() ? 4 : 3); i++) {
									glm::vec3 pos = *verts[i] - disp;
									for(size_t j = 0; j < sector.vertices.size(); j++) {
										if(vec3_equal_eps(pos, sector.vertices[j])) {
											sector_inds[i] = j;
										}
									}
									if(sector_inds[i] == -1) {
										sector_inds[i] = sector.vertices.size();
										sector.vertices.push_back(pos);
									}
								}
								
								if(sector_inds[3] > -1) {
									sector.faces.emplace_back(sector_inds[3], sector_inds[2], sector_inds[1], sector_inds[0], type);
								} else {
									sector.faces.emplace_back(sector_inds[2], sector_inds[1], sector_inds[0], type);
								}
								inserts++;
							}
						}
					}
				}
				assert(inserts > 0);
			}
		}
	}
	
	stop_timer();
	return sectors;
}

// https://gdbooks.gitbooks.io/3dcollisions/content/Chapter4/aabb-triangle.html
static bool test_tri_sector_intersection(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2) {
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
	
	for(const glm::vec3& axis : axes) {
		f32 p0 = glm::dot(v0, axis);
		f32 p1 = glm::dot(v1, axis);
		f32 p2 = glm::dot(v2, axis);
		float r = 0.f;
		r += fabs(glm::dot(u0, axis));
		r += fabs(glm::dot(u1, axis));
		r += fabs(glm::dot(u2, axis));
		r *= 2.f;
		if(std::max(-std::max({p0, p1, p2}), std::min({p0, p1, p2})) > r) {
			return false;
		}
	}
	
	return true;
}

static CollisionSector& lookup_sector(CollisionSectors& sectors, s32 x, s32 y, s32 z) {
	if(sectors.list.empty()) {
		sectors.list.resize(1);
		sectors.coord = z;
	} else if(z < sectors.coord) {
		sectors.list.insert(sectors.list.begin(), sectors.coord - z, {});
		sectors.coord = z;
	} else if(z >= sectors.coord + sectors.list.size()) {
		sectors.list.resize(z - sectors.coord + 1);
	}
	CollisionList<CollisionList<CollisionSector>>& z_node = sectors.list[z - sectors.coord];
	if(z_node.list.empty()) {
		z_node.list.resize(1);
		z_node.coord = y;
	} else if(y < z_node.coord) {
		z_node.list.insert(z_node.list.begin(), z_node.coord - y, {});
		z_node.coord = y;
	} else if(y >= z_node.coord + z_node.list.size()) {
		z_node.list.resize(y - z_node.coord + 1);
	}
	CollisionList<CollisionSector>& y_node = z_node.list[y - z_node.coord];
	if(y_node.list.empty()) {
		y_node.list.resize(1);
		y_node.coord = x;
	} else if(x < y_node.coord) {
		y_node.list.insert(y_node.list.begin(), y_node.coord - x, {});
		y_node.coord = x;
	} else if(x >= y_node.coord + y_node.list.size()) {
		y_node.list.resize(x - y_node.coord + 1);
	}
	return y_node.list[x - y_node.coord];
}

static void optimise_collision(CollisionSectors& sectors) {
	start_timer("Optimising collision tree");
	
	for(auto& y_partitions : sectors.list) {
		for(auto& x_partitions : y_partitions.list) {
			for(CollisionSector& sector : x_partitions.list) {
				reduce_quads_to_tris(sector);
				remove_killed_faces(sector);
				remove_unreferenced_vertices(sector);
			}
		}
	}
	
	stop_timer();
}

static void reduce_quads_to_tris(CollisionSector& sector) {
	size_t face_count = sector.faces.size();
	for(size_t i = 0; i < face_count; i++) {
		CollFace& face = sector.faces[i];
		if(face.is_quad && face.alive) {
			glm::vec3& v0 = sector.vertices[face.v0];
			glm::vec3& v1 = sector.vertices[face.v1];
			glm::vec3& v2 = sector.vertices[face.v2];
			glm::vec3& v3 = sector.vertices[face.v3];
			bool i0 = test_tri_sector_intersection(v0, v1, v2);
			bool i2 = test_tri_sector_intersection(v2, v3, v0);
			if(i0 && !i2) {
				face.alive = false;
				sector.faces.emplace_back(face.v0, face.v1, face.v2, face.type);
			} else if(i2 && !i0) {
				face.alive = false;
				sector.faces.emplace_back(face.v2, face.v3, face.v0, face.type);
			} else {
				bool i1 = test_tri_sector_intersection(v1, v2, v3);
				bool i3 = test_tri_sector_intersection(v3, v0, v1);
				if(i1 && !i3) {
					face.alive = false;
					sector.faces.emplace_back(face.v1, face.v2, face.v3, face.type);
				} else if(i3 && !i1) {
					face.alive = false;
					sector.faces.emplace_back(face.v3, face.v0, face.v1, face.type);
				}
			}
		}
	}
}

static void remove_killed_faces(CollisionSector& sector) {
	std::vector<CollFace> new_faces;
	for(CollFace& face : sector.faces) {
		if(face.alive) {
			new_faces.push_back(face);
		}
	}
	sector.faces = std::move(new_faces);
}

static void remove_unreferenced_vertices(CollisionSector& sector) {
	std::vector<glm::vec3> new_vertices;
	std::vector<size_t> new_indices(sector.vertices.size(), SIZE_MAX);
	for(CollFace& face : sector.faces) {
		u8* inds[4] = {&face.v0, &face.v1, &face.v2, &face.v3};
		for(s32 i = 0; i < (face.is_quad ? 4 : 3); i++) {
			if(new_indices[*inds[i]] == SIZE_MAX) {
				new_indices[*inds[i]] = new_vertices.size();
				new_vertices.push_back(sector.vertices[*inds[i]]);
			}
			*inds[i] = new_indices[*inds[i]];
		}
	}
	sector.vertices = std::move(new_vertices);
}
