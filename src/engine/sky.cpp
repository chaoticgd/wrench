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

#include <core/vif.h>
#include <core/mesh.h>

static std::tuple<std::vector<Texture>, std::vector<s32>> read_sky_textures(Buffer src, const SkyHeader& header);
static std::tuple<s64, s64> write_sky_textures(OutBuffer dest, const std::vector<Texture>& textures, const std::vector<s32>& texture_mappings);
static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, f32 framerate);
static s64 write_sky_shell(OutBuffer dest, const SkyShell& shell, f32 framerate);
static f32 rotation_to_radians_per_second(u16 angle, f32 framerate);
static u16 rotation_from_radians_per_second(f32 angle, f32 framerate);
static Mesh read_sky_cluster(Buffer src, s64 offset, s32 texture_count, bool textured);
static void write_sky_cluster(OutBuffer dest, SkyClusterHeader& header, const Mesh& cluster);

Sky read_sky(Buffer src, f32 framerate) {
	Sky sky;
	
	SkyHeader header = src.read<SkyHeader>(0, "header");
	verify(header.shell_count <= 8, "Too many sky shells!");
	
	sky.colour = header.colour;
	sky.clear_screen = !!header.clear_screen;
	sky.fx = src.read_multiple<u8>(header.fx_list, header.fx_count, "FX indices").copy();
	sky.maximum_sprite_count = header.maximum_sprite_count;
	
	std::tie(sky.textures, sky.texture_mappings) = read_sky_textures(src, header);
	
	for(s32 i = 0; i < header.shell_count; i++) {
		sky.shells.emplace_back(read_sky_shell(src, header.shells[i], header.texture_count, framerate));
	}
	
	return sky;
}

void write_sky(OutBuffer dest, const Sky& sky, f32 framerate) {
	verify(sky.shells.size() <= 8, "Too many sky shells!");
	
	dest.pad(0x40);
	s64 header_ofs = dest.alloc<SkyHeader>();
	SkyHeader header = {};
	
	header.colour = sky.colour;
	header.clear_screen = sky.clear_screen;
	header.shell_count = (s16) sky.shells.size();
	header.texture_count = (s16) sky.texture_mappings.size();
	header.fx_count = (s16) sky.fx.size();
	
	dest.pad(0x10);
	header.fx_list = dest.write_multiple(sky.fx);
	header.maximum_sprite_count = (s16) sky.maximum_sprite_count;
	
	auto [defs, data] = write_sky_textures(dest, sky.textures, sky.texture_mappings);
	header.texture_defs = defs;
	header.texture_data = data;
	
	if(header.maximum_sprite_count > 0) {
		dest.pad(0x40);
		header.sprites = dest.tell();
		dest.alloc_multiple<u8>(header.maximum_sprite_count * 0x20);
	}
	
	for(size_t i = 0; i < sky.shells.size(); i++) {
		header.shells[i] = write_sky_shell(dest, sky.shells[i], framerate);
	}
	
	dest.write(header_ofs, header);
}

static std::tuple<std::vector<Texture>, std::vector<s32>> read_sky_textures(Buffer src, const SkyHeader& header) {
	std::vector<Texture> textures;
	std::vector<s32> texture_mappings;
	std::vector<SkyTexture> defs;
	
	// Multiple texture headers can point to the same texture. Here, we only
	// write out each texture once, but we create a seperate element in the
	// texture_mappings list for each duplicate.
	for(const SkyTexture& def : src.read_multiple<SkyTexture>(header.texture_defs, header.texture_count, "texture defs")) {
		s32 index = -1;
		for(s32 i = 0; i < (s32) defs.size(); i++) {
			if(memcmp(&def, &defs[i], sizeof(SkyTexture)) == 0) {
				verify(i >= header.fx_count, "Weird fx texture mapping.");
				index = texture_mappings[i];
				break;
			}
		}
		if(index == -1) {
			std::vector<u8> data = src.read_bytes(header.texture_data + def.texture_offset, def.width * def.height, "texture data");
			std::vector<u32> palette = src.read_multiple<u32>(header.texture_data + def.palette_offset, 256, "palette").copy();
			Texture texture = Texture::create_8bit_paletted(def.width, def.height, std::move(data), std::move(palette));
			texture.multiply_alphas();
			texture.swizzle_palette();
			
			index = (s32) textures.size();
			textures.emplace_back(std::move(texture));
		}
		texture_mappings.emplace_back(index);
		defs.emplace_back(def);
	}
	
	return {textures, texture_mappings};
}

static std::tuple<s64, s64> write_sky_textures(OutBuffer dest, const std::vector<Texture>& textures, const std::vector<s32>& texture_mappings) {
	dest.pad(0x10);
	s64 defs_ofs = dest.alloc_multiple<SkyTexture>(texture_mappings.size());
	dest.pad(0x40);
	s64 data_ofs = dest.tell();
	
	std::vector<SkyTexture> defs(texture_mappings.size(), {{}});
	
	for(s32 i = 0; i < (s32) textures.size(); i++) {
		Texture texture = textures[i];
		texture.to_8bit_paletted();
		texture.divide_alphas();
		texture.swizzle_palette();
		
		dest.pad(0x40);
		s32 palette_ofs = dest.tell() - data_ofs;
		dest.write_multiple(texture.palette());
		dest.pad(0x40);
		s32 texture_ofs = dest.tell() - data_ofs;
		dest.write_multiple(texture.data);
		
		// Populate all the texture headers that point to this texture.
		for(s32 j = 0; j < (s32) texture_mappings.size(); j++) {
			if(texture_mappings[j] == i) {
				SkyTexture& def = defs[j];
				def.texture_offset = texture_ofs;
				def.palette_offset = palette_ofs;
				def.width = texture.width;
				def.height = texture.height;
			}
		}
	}
	
	dest.write_multiple(defs_ofs, defs);
	
	return {defs_ofs, data_ofs};
}

static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, f32 framerate) {
	SkyShell shell;
	
	SkyShellHeader header = src.read<SkyShellHeader>(offset, "shell header");
	shell.textured = (header.flags & 1) == 0;
	shell.bloom = ((header.flags >> 1) & 1) == 1;
	shell.rotation.x = rotation_to_radians_per_second(header.rotation.x, framerate);
	shell.rotation.y = rotation_to_radians_per_second(header.rotation.y, framerate);
	shell.rotation.z = rotation_to_radians_per_second(header.rotation.z, framerate);
	shell.angular_velocity.x = rotation_to_radians_per_second(header.angular_velocity.x, framerate);
	shell.angular_velocity.y = rotation_to_radians_per_second(header.angular_velocity.y, framerate);
	shell.angular_velocity.z = rotation_to_radians_per_second(header.angular_velocity.z, framerate);
	
	for(s32 i = 0; i < header.cluster_count; i++) {
		shell.clusters.emplace_back(read_sky_cluster(src, offset + sizeof(SkyShellHeader) + i * sizeof(SkyClusterHeader), texture_count, shell.textured));
	}
	
	return shell;
}

static s64 write_sky_shell(OutBuffer dest, const SkyShell& shell, f32 framerate) {
	dest.pad(0x10);
	SkyShellHeader header = {};
	s64 header_ofs = dest.alloc<SkyShellHeader>();
	
	header.cluster_count = (s16) shell.clusters.size();
	header.flags |= !shell.textured;
	header.flags |= shell.bloom << 1;
	header.rotation.x = rotation_from_radians_per_second(shell.rotation.x, framerate);
	header.rotation.y = rotation_from_radians_per_second(shell.rotation.y, framerate);
	header.rotation.z = rotation_from_radians_per_second(shell.rotation.z, framerate);
	header.angular_velocity.x = rotation_from_radians_per_second(shell.angular_velocity.x, framerate);
	header.angular_velocity.y = rotation_from_radians_per_second(shell.angular_velocity.y, framerate);
	header.angular_velocity.z = rotation_from_radians_per_second(shell.angular_velocity.z, framerate);
	
	s64 cluster_table_ofs = dest.alloc_multiple<SkyClusterHeader>(shell.clusters.size());
	for(size_t i = 0; i < shell.clusters.size(); i++) {
		SkyClusterHeader cluster = {};
		write_sky_cluster(dest, cluster, shell.clusters[i]);
		dest.write(cluster_table_ofs + i * sizeof(SkyClusterHeader), cluster);
	}
	
	dest.write(header_ofs, header);
	return header_ofs;
}

static f32 rotation_to_radians_per_second(u16 angle, f32 framerate) {
	return angle * (framerate * ((2.f * WRENCH_PI) / UINT16_MAX));
}

static u16 rotation_from_radians_per_second(f32 angle, f32 framerate) {
	return (u16) roundf(angle * ((UINT16_MAX / (2.f * WRENCH_PI)) / framerate));
}

static Mesh read_sky_cluster(Buffer src, s64 offset, s32 texture_count, bool textured) {
	Mesh cluster;
	cluster.flags |= MESH_HAS_VERTEX_COLOURS | MESH_HAS_TEX_COORDS;
	
	SkyClusterHeader header = src.read<SkyClusterHeader>(offset, "sky cluster header");
	
	//printf("clustinfo :: size=%04d, verts=%04d, tris=%04d\n", header.data_size, header.vertex_count, header.tri_count);
	
	auto vertices = src.read_multiple<SkyVertex>(header.data + header.vertex_offset, header.vertex_count, "vertex positions");
	auto sts = src.read_multiple<SkyTexCoord>(header.data + header.st_offset, header.vertex_count, "texture coordinates");
	for(s32 i = 0; i < header.vertex_count; i++) {
		f32 x = vertices[i].x * (1 / 1024.f);
		f32 y = vertices[i].y * (1 / 1024.f);
		f32 z = vertices[i].z * (1 / 1024.f);
		f32 s = vu_fixed12_to_float(sts[i].s);
		f32 t = vu_fixed12_to_float(sts[i].t);
		ColourAttribute colour = {255, 255, 255};
		if(vertices[i].alpha == 0x80) {
			colour.a = 0xff;
		} else {
			colour.a = vertices[i].alpha * 2;
		}
		cluster.vertices.emplace_back(glm::vec3(x, y, z), colour, glm::vec2(s, t));
	}
	
	SubMesh* submesh = nullptr;
	if(cluster.submeshes.size() >= 1) {
		submesh = &cluster.submeshes.back();
	}
	
	s32 last_submesh_material = 0;
	
	auto faces = src.read_multiple<SkyFace>(header.data + header.tri_offset, header.tri_count, "faces");
	for(const SkyFace& face : faces) {
		if(submesh == nullptr || face.texture != last_submesh_material) {
			verify(face.texture < texture_count, "Sky has bad texture data.");
			submesh = &cluster.submeshes.emplace_back();
			submesh->material = face.texture;
		}
		last_submesh_material = submesh->material;
		
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
		vertex.x = (s16) roundf(src.pos.x * 1024.f);
		vertex.y = (s16) roundf(src.pos.y * 1024.f);
		vertex.z = (s16) roundf(src.pos.z * 1024.f);
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
		st.s = vu_float_to_fixed12(src.tex_coord.x);
		st.t = vu_float_to_fixed12(src.tex_coord.y);
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
	
	dest.pad(0x10);
	header.data_size = dest.tell() - header.data;
}
