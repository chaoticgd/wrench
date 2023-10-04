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
static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, Game game, f32 framerate);
static s64 write_sky_shell(OutBuffer dest, const SkyShell& shell, Game game, f32 framerate);
static f32 rotation_to_radians_per_second(s16 angle, f32 framerate);
static s16 rotation_from_radians_per_second(f32 angle, f32 framerate);
static void read_sky_cluster(GLTF::Mesh& dest, Buffer src, s64 offset, s32 texture_count, bool textured);
static void write_sky_clusters(std::vector<SkyClusterHeader>& headers, OutBuffer data, const GLTF::Mesh& shell, f32 min_azimuth, f32 max_azimuth, f32 azimuth_bias, f32 min_elev, f32 max_elev);
static SkyClusterHeader write_sky_cluster(OutBuffer data, const std::vector<Vertex>& vertices, const std::vector<SkyFace>& faces);

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
		sky.shells.emplace_back(read_sky_shell(src, header.shells[i], header.texture_count, game, framerate));
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
		header.shells[i] = write_sky_shell(dest, sky.shells[i], game, framerate);
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

static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count, Game game, f32 framerate) {
	SkyShell shell;
	
	s32 cluster_count = 0;
	if(game == Game::RAC || game == Game::GC) {
		RacGcSkyShellHeader header = src.read<RacGcSkyShellHeader>(offset, "shell header");
		shell.textured = (header.flags & 1) == 0;
		cluster_count = header.cluster_count;
	} else {
		UyaDlSkyShellHeader header = src.read<UyaDlSkyShellHeader>(offset, "shell header");
		shell.textured = (header.flags & 1) == 0;
		shell.bloom = ((header.flags >> 1) & 1) == 1;
		shell.rotation.x = rotation_to_radians_per_second(header.rotation.x, framerate);
		shell.rotation.y = rotation_to_radians_per_second(header.rotation.y, framerate);
		shell.rotation.z = rotation_to_radians_per_second(header.rotation.z, framerate);
		shell.angular_velocity.x = rotation_to_radians_per_second(header.angular_velocity.x, framerate);
		shell.angular_velocity.y = rotation_to_radians_per_second(header.angular_velocity.y, framerate);
		shell.angular_velocity.z = rotation_to_radians_per_second(header.angular_velocity.z, framerate);
		cluster_count = header.cluster_count;
	}
	
	for(s32 i = 0; i < cluster_count; i++) {
		read_sky_cluster(shell.mesh, src, offset + 0x10 + i * sizeof(SkyClusterHeader), texture_count, shell.textured);
	}
	
	GLTF::deduplicate_vertices(shell.mesh);
	
	return shell;
}

static s64 write_sky_shell(OutBuffer dest, const SkyShell& shell, Game game, f32 framerate) {
	std::vector<SkyClusterHeader> cluster_headers;
	std::vector<u8> cluster_data;
	
	f32 mid_threshold = 20.f / 90.f;
	f32 high_threshold = 65.f / 90.f;
	for(s32 azimuth = -6; azimuth < 6; azimuth++) {
		f32 min_azimuth = azimuth * (1.f / 6.f);
		f32 max_azimuth = (azimuth + 1) * (1.f / 6.f);
		
		write_sky_clusters(cluster_headers, cluster_data, shell.mesh, min_azimuth, max_azimuth, 1 / 12.f, -mid_threshold, mid_threshold);
		write_sky_clusters(cluster_headers, cluster_data, shell.mesh, min_azimuth, max_azimuth, 0.f, -high_threshold, -mid_threshold);
		write_sky_clusters(cluster_headers, cluster_data, shell.mesh, min_azimuth, max_azimuth, 0.f, mid_threshold, high_threshold);
	}
	write_sky_clusters(cluster_headers, cluster_data, shell.mesh, -1.f, 1.f, 0.f, high_threshold, 1.f);
	write_sky_clusters(cluster_headers, cluster_data, shell.mesh, -1.f, 1.f, 0.f, -1.f, -high_threshold);
	
	verify(cluster_headers.size() < INT16_MAX, "Too many clusters in a shell.");
	
	dest.pad(0x10);
	s64 header_ofs = dest.tell();
	if(game == Game::RAC || game == Game::GC) {
		RacGcSkyShellHeader header = {};
		verify(cluster_headers.size() < INT32_MAX, "Too many clusters.");
		header.cluster_count = (s32) cluster_headers.size();
		header.flags |= !shell.textured;
		dest.write(header);
	} else {
		UyaDlSkyShellHeader header = {};
		verify(cluster_headers.size() < INT16_MAX, "Too many clusters.");
		header.cluster_count = (s16) cluster_headers.size();
		header.flags |= !shell.textured;
		header.flags |= shell.bloom << 1;
		header.rotation.x = rotation_from_radians_per_second(shell.rotation.x, framerate);
		header.rotation.y = rotation_from_radians_per_second(shell.rotation.y, framerate);
		header.rotation.z = rotation_from_radians_per_second(shell.rotation.z, framerate);
		header.angular_velocity.x = rotation_from_radians_per_second(shell.angular_velocity.x, framerate);
		header.angular_velocity.y = rotation_from_radians_per_second(shell.angular_velocity.y, framerate);
		header.angular_velocity.z = rotation_from_radians_per_second(shell.angular_velocity.z, framerate);
		dest.write(header);
	}
	dest.pad(0x10);
	for(SkyClusterHeader& cluster_header : cluster_headers) {
		cluster_header.data += dest.tell() + cluster_headers.size() * sizeof(SkyClusterHeader);
	}
	dest.write_multiple(cluster_headers);
	dest.write_multiple(cluster_data);
	
	return header_ofs;
}

static f32 rotation_to_radians_per_second(s16 angle, f32 framerate) {
	return angle * (framerate * ((2.f * WRENCH_PI) / 32768.f));
}

static s16 rotation_from_radians_per_second(f32 angle, f32 framerate) {
	return (u16) roundf(angle * ((32768.f / (2.f * WRENCH_PI)) / framerate));
}

static void read_sky_cluster(GLTF::Mesh& dest, Buffer src, s64 offset, s32 texture_count, bool textured) {
	SkyClusterHeader header = src.read<SkyClusterHeader>(offset, "sky cluster header");
	
	//printf("clustinfo :: size=%04d, verts=%04d, tris=%04d\n", header.data_size, header.vertex_count, header.tri_count);
	
	s32 base_index = (s32) dest.vertices.size();
	dest.vertices.resize(base_index + header.vertex_count);
	auto sky_vertices = src.read_multiple<SkyVertex>(header.data + header.vertex_offset, header.vertex_count, "vertex positions");
	auto sts = src.read_multiple<SkyTexCoord>(header.data + header.st_offset, header.vertex_count, "texture coordinates");
	for(s32 i = 0; i < header.vertex_count; i++) {
		dest.vertices[base_index + i].pos.x = sky_vertices[i].x * (1 / 1024.f);
		dest.vertices[base_index + i].pos.y = sky_vertices[i].y * (1 / 1024.f);
		dest.vertices[base_index + i].pos.z = sky_vertices[i].z * (1 / 1024.f);
		dest.vertices[base_index + i].tex_coord.s = vu_fixed12_to_float(sts[i].s);
		dest.vertices[base_index + i].tex_coord.t = vu_fixed12_to_float(sts[i].t);
		dest.vertices[base_index + i].colour.r = 255;
		dest.vertices[base_index + i].colour.g = 255;
		dest.vertices[base_index + i].colour.b = 255;
		if(sky_vertices[i].alpha == 0x80) {
			dest.vertices[base_index + i].colour.a = 255;
		} else {
			dest.vertices[base_index + i].colour.a = sky_vertices[i].alpha * 2;
		}
	}
	
	GLTF::MeshPrimitive* primitive = nullptr;
	Opt<s32> last_material = 0;
	
	auto faces = src.read_multiple<SkyFace>(header.data + header.tri_offset, header.tri_count, "faces");
	for(const SkyFace& face : faces) {
		if(primitive == nullptr || face.texture != last_material) {
			primitive = &dest.primitives.emplace_back();
			primitive->attributes_bitfield = GLTF::POSITION | GLTF::TEXCOORD_0 | GLTF::COLOR_0;
			if(face.texture != 0xff) {
				verify(face.texture < texture_count, "Sky has bad texture data.");
				primitive->material = face.texture;
			}
			last_material = face.texture;
		}
		
		// Reverse the winding order.
		primitive->indices.emplace_back((u32) base_index + face.indices[2]);
		primitive->indices.emplace_back((u32) base_index + face.indices[1]);
		primitive->indices.emplace_back((u32) base_index + face.indices[0]);
	}
}

static void write_sky_clusters(std::vector<SkyClusterHeader>& headers, OutBuffer data, const GLTF::Mesh& shell, f32 min_azimuth, f32 max_azimuth, f32 azimuth_bias, f32 min_elev, f32 max_elev) {
	std::vector<Vertex> vertices;
	std::vector<SkyFace> faces;
	std::vector<s32> mapping(shell.vertices.size(), -1);
	
	for(size_t i = 0; i < shell.primitives.size(); i++) {
		const GLTF::MeshPrimitive& primitive = shell.primitives[i];
		
		for(size_t j = 0; j < primitive.indices.size() / 3; j++) {
			glm::vec3 inverse_sphere_normal = glm::normalize(
				shell.vertices[primitive.indices[j * 3 + 0]].pos +
				shell.vertices[primitive.indices[j * 3 + 1]].pos +
				shell.vertices[primitive.indices[j * 3 + 2]].pos);
				
			f32 azimuth_radians = atan2f(inverse_sphere_normal.x, inverse_sphere_normal.y);
			f32 azimuth_half_turns = azimuth_radians * (1.f / WRENCH_PI);
			if(azimuth_bias != 0.f) {
				azimuth_half_turns = (fmodf((azimuth_half_turns + azimuth_bias) / 2.f + 0.5f, 1.f) - 0.5f) * 2.f;
			}
			if(azimuth_half_turns < min_azimuth || azimuth_half_turns > max_azimuth) {
				continue;
			}
			
			f32 elevation_radians = asinf(inverse_sphere_normal.z);
			f32 elevation_quarter_turns = elevation_radians * (2.f / WRENCH_PI);
			if(elevation_quarter_turns < min_elev || elevation_quarter_turns > max_elev) {
				continue;
			}
			
			size_t vertex_count = vertices.size();
			for(s32 k = 0; k < 3; k++) {
				s32 src_index = primitive.indices[j * 3 + k];
				if(mapping.at(src_index) == -1) {
					vertex_count++;
				}
			}
			
			// TODO: Should maybe check the cluster size too.
			if(vertex_count > INT8_MAX || faces.size() + 1 > INT16_MAX) {
				headers.emplace_back(write_sky_cluster(data, vertices, faces));
				vertices.clear();
				faces.clear();
				for(s32& m : mapping) {
					m = -1;
				}
			}
			
			s32 indices[3];
			for(s32 k = 0; k < 3; k++) {
				s32 src_index = primitive.indices[j * 3 + k];
				s32& dest_index = mapping.at(src_index);
				if(dest_index == -1) {
					dest_index = (s32) vertices.size();
					Vertex& vertex = vertices.emplace_back(shell.vertices.at(src_index));
					if((primitive.attributes_bitfield & GLTF::COLOR_0) == 0) {
						vertex.colour.a = 0xff;
					}
				}
				indices[k] = dest_index;
				verify(indices[k] < 256, "Too many vertices in a single cluster.");
			}
			
			SkyFace& face = faces.emplace_back();
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
		}
	}
	
	if(!faces.empty()) {
		headers.emplace_back(write_sky_cluster(data, vertices, faces));
	}
}

static SkyClusterHeader write_sky_cluster(OutBuffer data, const std::vector<Vertex>& vertices, const std::vector<SkyFace>& faces) {
	SkyClusterHeader header;
	header.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(vertices));
	header.vertex_count = (s16) vertices.size();
	
	data.pad(0x10);
	header.data = data.tell();
	header.vertex_offset = data.tell() - header.data;
	for(const Vertex& src : vertices) {
		SkyVertex vertex;
		vertex.x = (s16) roundf(src.pos.x * 1024.f);
		vertex.y = (s16) roundf(src.pos.y * 1024.f);
		vertex.z = (s16) roundf(src.pos.z * 1024.f);
		if(src.colour.a == 0xff) {
			vertex.alpha = 0x80;
		} else {
			vertex.alpha = src.colour.a / 2;
		}
		data.write(vertex);
	}
	
	data.pad(0x4);
	header.st_offset = data.tell() - header.data;
	for(const Vertex& src : vertices) {
		SkyTexCoord st;
		st.s = vu_float_to_fixed12(src.tex_coord.x);
		st.t = vu_float_to_fixed12(src.tex_coord.y);
		data.write(st);
	}
	
	data.pad(0x4);
	header.tri_offset = data.tell() - header.data;
	data.write_multiple(faces);
	header.tri_count = (s16) faces.size();
	
	data.pad(0x10);
	header.data_size = data.tell() - header.data;
	
	return header;
}
