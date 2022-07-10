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

#include "sky.h"

#include <core/mesh.h>

static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, f32 framerate);
static void write_sky_shell(OutBuffer dest, const SkyShell& shell, f32 framerate);
static f32 rotation_to_radians_per_frame(u16 angle, f32 framerate);
static u16 rotation_from_radians_per_frame(f32 angle, f32 framerate);
static Mesh read_sky_cluster(Buffer src, s64 offset, s32 texture_count, bool textured);
static void write_sky_cluster(OutBuffer dest, SkyClusterHeader& header, const Mesh& cluster);

Sky read_sky(Buffer src, Game game, f32 framerate) {
	Sky sky;
	
	SkyHeader header = src.read<SkyHeader>(0, "header");
	verify(header.shell_count <= 8, "Too many sky shells!");
	
	if(game == Game::DL) {
		for(const DlSkyTexture& def : src.read_multiple<DlSkyTexture>(header.texture_defs, header.texture_count, "texture defs")) {
			std::vector<u8> data = src.read_bytes(header.texture_data + def.texture_offset, def.width * def.height, "texture data");
			std::vector<u32> palette = src.read_multiple<u32>(header.texture_data + def.palette_offset, 256, "palette").copy();
			Texture texture = Texture::create_8bit_paletted(def.width, def.height, std::move(data), std::move(palette));
			texture.multiply_alphas();
			texture.swizzle_palette();
			sky.textures.emplace_back(std::move(texture));
		}
	} else {
		for(const RacGcUyaSkyTexture& def : src.read_multiple<RacGcUyaSkyTexture>(header.texture_defs, header.texture_count, "texture defs")) {
			std::vector<u8> data = src.read_bytes(header.texture_data + def.texture_offset, def.width * def.height, "texture data");
			std::vector<u32> palette = src.read_multiple<u32>(header.texture_data + def.palette_offset, 256, "palette").copy();
			Texture texture = Texture::create_8bit_paletted(def.width, def.height, std::move(data), std::move(palette));
			texture.multiply_alphas();
			texture.swizzle_palette();
			sky.textures.emplace_back(std::move(texture));
		}
	}
	
	for(s32 i = 0; i < header.shell_count; i++) {
		sky.shells.emplace_back(read_sky_shell(src, header.shells[i], header.texture_count, framerate));
	}
	
	return sky;
}

void write_sky(OutBuffer dest, const Sky& sky, Game game, f32 framerate) {
	verify(sky.shells.size() <= 8, "Too many sky shells!");
	
	dest.pad(0x40);
	s64 header_ofs = dest.alloc<SkyHeader>();
	SkyHeader header = {};
	
	header.shell_count = (s32) sky.shells.size();
	header.texture_count = (s32) sky.textures.size();
	
	header.texture_defs = dest.alloc_multiple<RacGcUyaSkyTexture>(sky.textures.size());
	dest.pad(0x40);
	header.texture_data = dest.tell();
	for(size_t i = 0; i < sky.textures.size(); i++) {
		Texture texture = sky.textures[i];
		texture.to_8bit_paletted();
		texture.divide_alphas();
		texture.swizzle_palette();
		
		RacGcUyaSkyTexture def = {};
		def.width = texture.width;
		def.height = texture.height;
		dest.pad(0x40);
		def.palette_offset = dest.tell() - header.texture_data;
		dest.write_multiple(texture.palette());
		dest.pad(0x40);
		def.texture_offset = dest.tell() - header.texture_data;
		dest.write_multiple(texture.data);
		
		dest.write(header.texture_defs + i * sizeof(RacGcUyaSkyTexture), def);
	}
	
	dest.pad(0x40);
	header.sprites = dest.tell();
	dest.alloc_multiple<u8>(0x1000); // Not sure what this is.
	
	for(size_t i = 0; i < sky.shells.size(); i++) {
		dest.pad(0x40);
		header.shells[i] = dest.tell();
		write_sky_shell(dest, sky.shells[i], framerate);
	}
	
	dest.write(header_ofs, header);
}

static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, f32 framerate) {
	SkyShell shell;
	
	SkyShellHeader header = src.read<SkyShellHeader>(offset, "shell header");
	
	shell.textured = (header.flags & 1) == 0;
	shell.bloom = ((header.flags >> 1) & 1) == 1;
	shell.rotation.x = rotation_to_radians_per_frame(header.rotation.x, framerate);
	shell.rotation.y = rotation_to_radians_per_frame(header.rotation.y, framerate);
	shell.rotation.z = rotation_to_radians_per_frame(header.rotation.z, framerate);
	shell.angular_velocity.x = rotation_to_radians_per_frame(header.angular_velocity.x, framerate);
	shell.angular_velocity.y = rotation_to_radians_per_frame(header.angular_velocity.y, framerate);
	shell.angular_velocity.z = rotation_to_radians_per_frame(header.angular_velocity.z, framerate);
	
	for(s32 i = 0; i < header.cluster_count; i++) {
		shell.clusters.emplace_back(read_sky_cluster(src, offset + sizeof(SkyShellHeader) + i * sizeof(SkyClusterHeader), texture_count, shell.textured));
	}
	
	return shell;
}

static void write_sky_shell(OutBuffer dest, const SkyShell& shell, f32 framerate) {
	dest.pad(0x10);
	SkyShellHeader header = {};
	s64 header_ofs = dest.alloc<SkyShellHeader>();
	
	header.flags |= shell.textured;
	header.flags |= shell.bloom << 1;
	header.rotation.x = rotation_from_radians_per_frame(shell.rotation.x, framerate);
	header.rotation.y = rotation_from_radians_per_frame(shell.rotation.y, framerate);
	header.rotation.z = rotation_from_radians_per_frame(shell.rotation.z, framerate);
	header.angular_velocity.x = rotation_from_radians_per_frame(shell.angular_velocity.x, framerate);
	header.angular_velocity.y = rotation_from_radians_per_frame(shell.angular_velocity.y, framerate);
	header.angular_velocity.z = rotation_from_radians_per_frame(shell.angular_velocity.z, framerate);
	
	s64 cluster_table_ofs = dest.alloc_multiple<SkyClusterHeader>(shell.clusters.size());
	for(size_t i = 0; i < shell.clusters.size(); i++) {
		SkyClusterHeader cluster = {};
		write_sky_cluster(dest, cluster, shell.clusters[i]);
		dest.write(cluster_table_ofs + i * sizeof(SkyClusterHeader), cluster);
	}
	
	dest.write(header_ofs, header);
}

static f32 rotation_to_radians_per_frame(u16 angle, f32 framerate) {
	return angle * ((2.f * M_PI) / UINT16_MAX);
}

static u16 rotation_from_radians_per_frame(f32 angle, f32 framerate) {
	return (u16) roundf(angle * (UINT16_MAX / (2 * M_PI)));
}

static Mesh read_sky_cluster(Buffer src, s64 offset, s32 texture_count, bool textured) {
	Mesh cluster;
	cluster.flags |= MESH_HAS_VERTEX_COLOURS | MESH_HAS_TEX_COORDS;
	
	SkyClusterHeader header = src.read<SkyClusterHeader>(offset, "sky cluster header");
	
	//printf("clustinfo :: size=%04d, verts=%04d, tris=%04d\n", header.data_size, header.vertex_count, header.tri_count);
	
	auto vertices = src.read_multiple<SkyVertex>(header.data + header.vertex_offset, header.vertex_count, "vertex positions");
	auto sts = src.read_multiple<SkyTexCoord>(header.data + header.st_offset, header.vertex_count, "texture coordinates");
	for(s32 i = 0; i < header.vertex_count; i++) {
		auto position = glm::vec3(
			vertices[i].x * (8.f / INT16_MAX),
			vertices[i].y * (8.f / INT16_MAX),
			vertices[i].z * (8.f / INT16_MAX));
		auto st = glm::vec2(sts[i].s * (8.f / INT16_MAX), -sts[i].t * (8.f / INT16_MAX));
		ColourAttribute colour = {255, 255, 255};
		if(vertices[i].alpha == 0x80) {
			colour.a = 0xff;
		} else {
			colour.a = vertices[i].alpha * 2;
		}
		cluster.vertices.emplace_back(position, colour, st);
	}
	
	SubMesh* submesh = nullptr;
	if(cluster.submeshes.size() >= 1) {
		submesh = &cluster.submeshes.back();
	}
	
	auto faces = src.read_multiple<SkyFace>(header.data + header.tri_offset, header.tri_count, "faces");
	for(const SkyFace& face : faces) {
		if(submesh == nullptr || submesh->material != face.texture + 1) {
			verify(face.texture < texture_count, "Sky has bad texture data.");
			submesh = &cluster.submeshes.emplace_back();
			if(textured) {
				verify(face.texture < texture_count, "Sky has bad texture data.");
				submesh->material = face.texture + 1;
			} else {
				submesh->material = 0;
			}
		}
		
		submesh->faces.emplace_back(face.indices[0], face.indices[1], face.indices[2]);
	}
	
	return cluster;
}

static void write_sky_cluster(OutBuffer dest, SkyClusterHeader& header, const Mesh& cluster) {
	header.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(cluster.vertices));
	header.vertex_count = cluster.vertices.size();
	
	dest.pad(0x10);
	header.data = dest.tell();
	header.vertex_offset = dest.tell() - header.data;
	for(const Vertex& src : cluster.vertices) {
		SkyVertex vertex;
		vertex.x = src.pos.x * (INT16_MAX / 8.f);
		vertex.y = src.pos.y * (INT16_MAX / 8.f);
		vertex.z = src.pos.z * (INT16_MAX / 8.f);
		if(src.colour.a == 0xff) {
			vertex.alpha = 0x80;
		} else {
			vertex.alpha = src.colour.a / 2;
		}
		dest.write(vertex);
	}
	
	dest.pad(0x4);
	header.st_offset = dest.tell() - header.data;
	for(const Vertex& src : cluster.vertices) {
		SkyTexCoord st;
		st.s = src.tex_coord.x * (INT16_MAX / 8.f);
		st.t = src.tex_coord.y * (INT16_MAX / 8.f);
		dest.write(st);
	}
	
	dest.pad(0x4);
	header.tri_offset = dest.tell() - header.data;
	for(const SubMesh& submesh : cluster.submeshes) {
		for(const Face& src : submesh.faces) {
			verify(src.v0 < 256 && src.v1 < 256 && src.v2 < 256 && src.v3 < 256,
				"Too many vertices in a single cluster.");
			SkyFace face;
			verify(submesh.material < 256, "Too many textures.");
			face.texture = (u8) submesh.material;
			if(src.is_quad()) {
				face.indices[0] = (u8) src.v0;
				face.indices[1] = (u8) src.v1;
				face.indices[2] = (u8) src.v2;
				dest.write(face);
				face.indices[0] = (u8) src.v2;
				face.indices[1] = (u8) src.v3;
				face.indices[2] = (u8) src.v0;
				dest.write(face);
				header.tri_count += 2;
			} else {
				face.indices[0] = (u8) src.v0;
				face.indices[1] = (u8) src.v1;
				face.indices[2] = (u8) src.v2;
				dest.write(face);
				header.tri_count++;
			}
		}
	}
	
	header.data_size = dest.tell() - header.data;
}
