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
#include <core/algorithm.h>

static std::tuple<std::vector<Texture>, std::vector<s32>> read_sky_textures(Buffer src, const SkyHeader& header, Game game);
static std::tuple<s64, s64> write_sky_textures(OutBuffer dest, const std::vector<Texture>& textures, const std::vector<s32>& texture_mappings, Game game);
static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, f32 framerate);
static s64 write_sky_shell(OutBuffer dest, const SkyShell& shell, f32 framerate);
static f32 rotation_to_radians_per_second(s16 angle, f32 framerate);
static s16 rotation_from_radians_per_second(f32 angle, f32 framerate);
static GLTF::Mesh read_sky_cluster(Buffer src, s64 offset, s32 texture_count, bool textured);
static void write_sky_cluster(OutBuffer dest, SkyClusterHeader& header, const GLTF::Mesh& cluster);

Sky read_sky(Buffer src, Game game, f32 framerate) {
	Sky sky;
	
	SkyHeader header = src.read<SkyHeader>(0, "header");
	verify(header.shell_count <= 8, "Too many sky shells!");
	
	sky.colour = header.colour;
	sky.clear_screen = !!header.clear_screen;
	sky.fx = src.read_multiple<u8>(header.fx_list, header.fx_count, "FX indices").copy();
	sky.maximum_sprite_count = header.maximum_sprite_count;
	
	std::tie(sky.textures, sky.texture_mappings) = read_sky_textures(src, header, game);
	
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
	
	header.colour = sky.colour;
	header.clear_screen = sky.clear_screen;
	header.shell_count = (s16) sky.shells.size();
	header.texture_count = (s16) sky.texture_mappings.size();
	header.fx_count = (s16) sky.fx.size();
	
	dest.pad(0x10);
	header.fx_list = dest.write_multiple(sky.fx);
	header.maximum_sprite_count = (s16) sky.maximum_sprite_count;
	
	auto [defs, data] = write_sky_textures(dest, sky.textures, sky.texture_mappings, game);
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

static std::tuple<std::vector<Texture>, std::vector<s32>> read_sky_textures(Buffer src, const SkyHeader& header, Game game) {
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
			if(game == Game::DL) {
				texture.swizzle();
			}
			
			index = (s32) textures.size();
			textures.emplace_back(std::move(texture));
		}
		texture_mappings.emplace_back(index);
		defs.emplace_back(def);
	}
	
	return {textures, texture_mappings};
}

static std::tuple<s64, s64> write_sky_textures(OutBuffer dest, const std::vector<Texture>& textures, const std::vector<s32>& texture_mappings, Game game) {
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
		if(game == Game::DL) {
			texture.swizzle();
		}
		
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

static f32 rotation_to_radians_per_second(s16 angle, f32 framerate) {
	return angle * (framerate * ((2.f * WRENCH_PI) / 32768.f));
}

static s16 rotation_from_radians_per_second(f32 angle, f32 framerate) {
	return (u16) roundf(angle * ((32768.f / (2.f * WRENCH_PI)) / framerate));
}

static GLTF::Mesh read_sky_cluster(Buffer src, s64 offset, s32 texture_count, bool textured) {
	GLTF::Mesh cluster;
	
	SkyClusterHeader header = src.read<SkyClusterHeader>(offset, "sky cluster header");
	
	//printf("clustinfo :: size=%04d, verts=%04d, tris=%04d\n", header.data_size, header.vertex_count, header.tri_count);
	
	std::vector<Vertex> vertices(header.vertex_count);
	auto sky_vertices = src.read_multiple<SkyVertex>(header.data + header.vertex_offset, header.vertex_count, "vertex positions");
	auto sts = src.read_multiple<SkyTexCoord>(header.data + header.st_offset, header.vertex_count, "texture coordinates");
	for(s32 i = 0; i < header.vertex_count; i++) {
		vertices[i].pos.x = sky_vertices[i].x * (1 / 1024.f);
		vertices[i].pos.y = sky_vertices[i].y * (1 / 1024.f);
		vertices[i].pos.z = sky_vertices[i].z * (1 / 1024.f);
		vertices[i].tex_coord.s = vu_fixed12_to_float(sts[i].s);
		vertices[i].tex_coord.t = vu_fixed12_to_float(sts[i].t);
		vertices[i].colour.r = 255;
		vertices[i].colour.g = 255;
		vertices[i].colour.b = 255;
		if(sky_vertices[i].alpha == 0x80) {
			vertices[i].colour.a = 255;
		} else {
			vertices[i].colour.a = sky_vertices[i].alpha * 2;
		}
	}
	
	GLTF::MeshPrimitive* primitive = nullptr;
	Opt<s32> last_material = 0;
	s32 vertex_primitives[256];
	s32 vertex_dest_indices[256];
	s32 primitive_index = 0;
	memset(vertex_primitives, 0, sizeof(vertex_primitives));
	
	auto faces = src.read_multiple<SkyFace>(header.data + header.tri_offset, header.tri_count, "faces");
	for(const SkyFace& face : faces) {
		if(primitive == nullptr || face.texture != last_material) {
			primitive = &cluster.primitives.emplace_back();
			primitive->attributes_bitfield = GLTF::POSITION | GLTF::TEXCOORD_0 | GLTF::COLOR_0;
			if(face.texture != 0xff) {
				verify(face.texture < texture_count, "Sky has bad texture data.");
				primitive->material = face.texture;
			}
			last_material = face.texture;
			primitive_index++;
		}
		
		// Make sure we're only adding vertices we need to to each primitive.
		s32 indices[3];
		for(s32 i = 0; i < 3; i++) {
			if(vertex_primitives[face.indices[i]] == primitive_index) {
				indices[i] = vertex_dest_indices[face.indices[i]];
			} else {
				indices[i] = (s32) primitive->vertices.size();
				primitive->vertices.emplace_back(vertices.at(face.indices[i]));
				vertex_primitives[face.indices[i]] = primitive_index;
				vertex_dest_indices[face.indices[i]] = indices[i];
			}
		}
		
		// Reverse the winding order.
		primitive->indices.emplace_back(indices[2]);
		primitive->indices.emplace_back(indices[1]);
		primitive->indices.emplace_back(indices[0]);
	}
	
	return cluster;
}

static void write_sky_cluster(OutBuffer dest, SkyClusterHeader& header, const GLTF::Mesh& cluster) {
	std::vector<Vertex> vertices;
	std::vector<s32> submesh_base_indices;
	submesh_base_indices.reserve(cluster.primitives.size());
	for(const GLTF::MeshPrimitive& primitive : cluster.primitives) {
		submesh_base_indices.emplace_back((s32) vertices.size());
		vertices.insert(vertices.end(), BEGIN_END(primitive.vertices));
	}
	
	std::vector<s32> canonical_vertices(vertices.size());
	mark_duplicates(vertices,
		[](const Vertex& lhs, const Vertex& rhs) { return (lhs < rhs) ? -1 : 0; },
		[&](s32 index, s32 canonical) { canonical_vertices[index] = canonical; });
	
	std::vector<Vertex> unique_vertices;
	for(s32 i = 0; i < (s32) vertices.size(); i++) {
		if(canonical_vertices[i] == i) {
			canonical_vertices[i] = (s32) unique_vertices.size();
			unique_vertices.emplace_back(vertices[i]);
		} else {
			canonical_vertices[i] = canonical_vertices[canonical_vertices[i]];
		}
	}
	
	verify(unique_vertices.size() <= INT16_MAX, "Too many vertices in a cluster.");
	
	header.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(unique_vertices));
	header.vertex_count = (s16) unique_vertices.size();
	
	dest.pad(0x10);
	header.data = dest.tell();
	header.vertex_offset = dest.tell() - header.data;
	for(const Vertex& src : unique_vertices) {
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
	for(const Vertex& src : unique_vertices) {
		SkyTexCoord st;
		st.s = vu_float_to_fixed12(src.tex_coord.x);
		st.t = vu_float_to_fixed12(src.tex_coord.y);
		dest.write(st);
	}
	
	dest.pad(0x4);
	header.tri_offset = dest.tell() - header.data;
	for(size_t i = 0; i < cluster.primitives.size(); i++) {
		const GLTF::MeshPrimitive& primitive = cluster.primitives[i];
		s32 base_index = submesh_base_indices[i];
		
		for(size_t j = 0; j < primitive.indices.size() / 3; j++) {
			s32 indices[3];
			for(s32 k = 0; k < 3; k++) {
				s32 local_index = primitive.indices[j * 3 + k];
				indices[k] = canonical_vertices.at(base_index + local_index);
			}
			SkyFace face;
			if(primitive.material.has_value()) {
				verify(*primitive.material < 256, "Too many textures.");
				face.texture = (u8) *primitive.material;
			} else {
				face.texture = 0xff;
			}
			// Reverse the winding order.
			face.indices[0] = (u8) indices[2];
			face.indices[1] = (u8) indices[1];
			face.indices[2] = (u8) indices[0];
			dest.write(face);
			header.tri_count++;
		}
	}
	
	dest.pad(0x10);
	header.data_size = dest.tell() - header.data;
}
