/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

packed_struct(CollisionHeader,
	s32 mesh;
	s32 second_part;
)

template <typename T>
struct CollisionList {
	s32 coord;
	s32 temp_offset = -1; // Only used during writing.
	std::vector<T> list;
};

struct CollisionTri {
	u8 v0;
	u8 v1;
	u8 v2;
	u8 type;
};

struct CollisionQuad {
	u8 v0;
	u8 v1;
	u8 v2;
	u8 v3;
	u8 type;
};

struct CollisionSector {
	s32 offset = 0;
	std::vector<glm::vec3> vertices;
	std::vector<CollisionTri> tris;
	std::vector<CollisionQuad> quads;
	glm::vec3 displacement = {0, 0, 0};
	u8 unknown_byte;
};

using CollisionSectors = CollisionList<CollisionList<CollisionList<CollisionSector>>>;

static CollisionSectors parse_collision_mesh(Buffer mesh);
static void write_collision_mesh(OutBuffer dest, CollisionSectors& sectors);
static Mesh collision_sectors_to_mesh(const CollisionSectors& sectors);

Mesh read_collision(Buffer src) {
	CollisionHeader header = src.read<CollisionHeader>(0, "collision header");
	Buffer mesh;
	if(header.second_part != 0) {
		mesh = src.subbuf(header.mesh, header.second_part - header.mesh);
	} else {
		mesh = src.subbuf(header.mesh);
	}
	CollisionSectors sectors = parse_collision_mesh(mesh);
	Mesh raw_mesh = collision_sectors_to_mesh(sectors);
	// The vertices and faces stored in the games files are duplicated such that
	// only one sector must be accessed to do collision detection.
	return deduplicate_faces(deduplicate_vertices(std::move(raw_mesh)));
}


void roundtrip_collision(OutBuffer dest, Buffer src) {
	CollisionHeader header = src.read<CollisionHeader>(0, "collision header");
	Buffer mesh;
	if(header.second_part != 0) {
		mesh = src.subbuf(header.mesh, header.second_part - header.mesh);
	} else {
		mesh = src.subbuf(header.mesh);
	}
	CollisionSectors sectors = parse_collision_mesh(mesh);
	header.mesh = 0x40;
	header.second_part = 0;
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
				
				CollisionSector& sector = x_partitions.list[x];
				sector.offset = sector_offset;
				sector.vertices.resize(vertex_count);
				sector.tris.resize(face_count - quad_count);
				sector.quads.resize(quad_count);
				sector.unknown_byte = x_offsets[x] & 0xff;
				
				s32 ofs = sector_offset + 4;
				for(s32 vertex = 0; vertex < vertex_count; vertex++) {
					u32 value = mesh.read<u32>(ofs, "vertex");
					// value = 0bzzzzzzzzzzzzyyyyyyyyyyxxxxxxxxxx
					//  where z, y and x are signed.
					f32 x = ((s32) (value << 22) >> 22) / 16.f;
					f32 y = ((s32) (value << 12) >> 22) / 16.f;
					f32 z = ((s32) (value << 0) >> 20) / 64.f;
					sector.vertices[vertex] = {x, y, z};
					ofs += 4;
				}
				for(s32 quad = 0; quad < quad_count; quad++) {
					u8 v0 = mesh.read<u8>(ofs, "quad v0");
					u8 v1 = mesh.read<u8>(ofs + 1, "quad v1");
					u8 v2 = mesh.read<u8>(ofs + 2, "quad v2");
					u8 type = mesh.read<u8>(ofs + 3, "quad type");
					sector.quads[quad] = {v0, v1, v2, 0, type};
					ofs += 4;
				}
				for(s32 tri = 0; tri < face_count - quad_count; tri++) {
					u8 v0 = mesh.read<u8>(ofs, "tri v0");
					u8 v1 = mesh.read<u8>(ofs + 1, "tri v1");
					u8 v2 = mesh.read<u8>(ofs + 2, "tri v2");
					u8 type = mesh.read<u8>(ofs + 3, "tri type");
					sector.tris[tri] = {v0, v1, v2, type};
					ofs += 4;
				}
				for(s32 quad = 0; quad < quad_count; quad++) {
					u8 v3 = mesh.read<u8>(ofs, "quad x");
					sector.quads[quad].v3 = v3;
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
	verify(sectors.list.size() < 65536, "Collision has too many Z partitions (count too high).");
	dest.write<u16>(sectors.list.size());
	sectors.temp_offset = dest.alloc_multiple<u16>(sectors.list.size());
	
	for(s32 z = 0; z < sectors.list.size(); z++) {
		dest.pad(4);
		verify((dest.tell() - base_ofs) / 4 < 65536, "Collision has too many Z partitions (offset too high).");
		dest.write<u16>(sectors.temp_offset + z * sizeof(u16), (dest.tell() - base_ofs) / 4);
		auto& y_partitions = sectors.list[z];
		dest.write<s16>(y_partitions.coord);
		verify(y_partitions.list.size() < 65536, "Collision has too many Y partitions.");
		dest.write<u16>(y_partitions.list.size());
		y_partitions.temp_offset = dest.alloc_multiple<u32>(y_partitions.list.size());
	}
	
	for(s32 z = 0; z < sectors.list.size(); z++) {
		auto& y_partitions = sectors.list[z];
		for(s32 y = 0; y < y_partitions.list.size(); y++) {
			dest.pad(4);
			dest.write<u32>(y_partitions.temp_offset + y * sizeof(u32), dest.tell() - base_ofs);
			auto& x_partitions = y_partitions.list[y];
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
				dest.pad(0x10);
				dest.write<u32>(x_partitions.temp_offset + x * sizeof(u32), (dest.tell() - base_ofs) << 8 | x_partitions.list[x].unknown_byte);
				const CollisionSector& sector = x_partitions.list[x];
				verify(sector.tris.size() + sector.quads.size() < 65536, "Too many faces in collision sector.");
				dest.write<u16>(sector.tris.size() + sector.quads.size());
				verify(sector.vertices.size() < 256, "Too many vertices in collision sector.");
				dest.write<u8>(sector.vertices.size());
				verify(sector.quads.size() < 256, "Too many quads in collision sector.");
				dest.write<u8>(sector.quads.size());
				
				for(const glm::vec3& vertex : sector.vertices) {
					u32 value = 0;
					// value = 0bzzzzzzzzzzzzyyyyyyyyyyxxxxxxxxxx
					//  where z, y and x are signed.
					value |= ((((s32) (vertex.x * 16.f)) << 22) >> 22) & 0b00000000000000000000001111111111;
					value |= (((s32) (vertex.y * 16.f) << 22) >> 12) & 0b00000000000011111111110000000000;
					value |= (((s32) (vertex.z * 64.f)) << 20) & 0b11111111111100000000000000000000;
					dest.write(value);
				}
				for(const CollisionQuad& quad : sector.quads) {
					dest.write<u8>(quad.v0);
					dest.write<u8>(quad.v1);
					dest.write<u8>(quad.v2);
					dest.write<u8>(quad.type);
				}
				for(const CollisionTri& tri : sector.tris) {
					dest.write<u8>(tri.v0);
					dest.write<u8>(tri.v1);
					dest.write<u8>(tri.v2);
					dest.write<u8>(tri.type);
				}
				for(const CollisionQuad& quad : sector.quads) {
					dest.write<u8>(quad.v3);
				}
			}
		}
	}
}

static Mesh collision_sectors_to_mesh(const CollisionSectors& sectors) {
	Mesh mesh;
	for(const auto& y_partitions : sectors.list) {
		for(const auto& x_partitions : y_partitions.list) {
			for(const CollisionSector& sector : x_partitions.list) {
				mesh.positions.reserve(mesh.positions.size() + sector.vertices.size());
				s32 vertex_index = mesh.positions.size();
				for(const glm::vec3& vertex : sector.vertices) {
					mesh.positions.push_back(sector.displacement + vertex);
				}
				for(const CollisionTri& tri : sector.tris) {
					mesh.tris.push_back({
						vertex_index + tri.v0,
						vertex_index + tri.v1,
						vertex_index + tri.v2
					});
				}
				for(const CollisionQuad& quad : sector.quads) {
					mesh.quads.push_back({
						vertex_index + quad.v0,
						vertex_index + quad.v1,
						vertex_index + quad.v2,
						vertex_index + quad.v3
					});
				}
				mesh.is_collision_mesh = true;
			}
		}
	}
	return mesh;
}
