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
#include "core/mesh.h"
#include <cstdint>

static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count);
static void read_sky_cluster(Mesh& dest, Buffer src, s64 offset, s32 texture_count, bool textured);

packed_struct(SkyHeader,
	/* 0x00 */ s32 pad;
	/* 0x04 */ s16 clear_screen;
	/* 0x06 */ s16 shell_count;
	/* 0x08 */ s16 sprite_count;
	/* 0x0a */ s16 sprite_max;
	/* 0x0c */ s16 texture_count;
	/* 0x0e */ s16 fx_count;
	/* 0x10 */ s32 texture_defs;
	/* 0x14 */ s32 texture_data;
	/* 0x18 */ s32 fx_list;
	/* 0x1c */ s32 sprites;
	/* 0x20 */ s32 shells[8];
)

packed_struct(RacGcUyaSkyTexture,
	/* 0x0 */ s32 palette_offset;
	/* 0x4 */ s32 texture_offset;
	/* 0x8 */ s32 width;
	/* 0xc */ s32 height;
)

packed_struct(DlSkyTexture,
	/* 0x0 */ u64 tex0;
	/* 0x8 */ u16 texture_offset;
	/* 0xa */ u16 palette_offset;
	/* 0xc */ s16 width;
	/* 0xe */ s16 height;
)

Sky read_sky(Buffer src, Game game) {
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
		sky.shells.emplace_back(read_sky_shell(src, header.shells[i], header.texture_count));
	}
	
	return sky;
}

void write_sky(OutBuffer dest, const Sky& sky, Game game) {
	
}

packed_struct(SkyShellHeader,
	/* 0x0 */ s16 cluster_count;
	/* 0x2 */ s16 flags;
	/* 0x4 */ s16 rotx;
	/* 0x6 */ s16 roty;
	/* 0x8 */ s16 rotz;
	/* 0xa */ s16 rotdeltax;
	/* 0xc */ s16 rotdeltay;
	/* 0xe */ s16 rotdeltaz;
)

packed_struct(SkyClusterHeader,
	/* 0x00 */ Vec4f bounding_sphere;
	/* 0x10 */ s32 data;
	/* 0x14 */ s16 vertex_count;
	/* 0x16 */ s16 tri_count;
	/* 0x18 */ s16 vertex_offset;
	/* 0x1a */ s16 st_offset;
	/* 0x1c */ s16 tri_offset;
	/* 0x1e */ s16 data_size;
)

static SkyShell read_sky_shell(Buffer src, s64 offset, s32 texture_count) {
	SkyShell shell;
	shell.mesh.flags |= MESH_HAS_TEX_COORDS;
	
	SkyShellHeader header = src.read<SkyShellHeader>(offset, "shell header");
	shell.textured = (header.flags & 1) == 0;
	shell.bloom = ((header.flags >> 1) & 1) == 1;
	shell.rotx = header.rotx;
	shell.roty = header.roty;
	shell.rotz = header.rotz;
	shell.rotdeltax = header.rotdeltax;
	shell.rotdeltay = header.rotdeltay;
	shell.rotdeltaz = header.rotdeltaz;
	
	printf("flags %d\n", header.flags);
	
	for(s32 i = 0; i < header.cluster_count; i++) {
		read_sky_cluster(shell.mesh, src, offset + sizeof(SkyShellHeader) + i * sizeof(SkyClusterHeader), texture_count, shell.textured);
	}
	
	shell.mesh = deduplicate_vertices(std::move(shell.mesh));
	
	return shell;
}

packed_struct(SkyVertex,
	s16 x;
	s16 y;
	s16 z;
	s16 alpha;
)

packed_struct(SkyTexCoord,
	s16 s;
	s16 t;
)

packed_struct(SkyFace,
	u8 indices[3];
	s8 texture;
)

static void read_sky_cluster(Mesh& dest, Buffer src, s64 offset, s32 texture_count, bool textured) {
	SkyClusterHeader header = src.read<SkyClusterHeader>(offset, "sky cluster header");
	
	size_t vertex_base = dest.vertices.size();
	
	auto vertices = src.read_multiple<SkyVertex>(header.data + header.vertex_offset, header.vertex_count, "vertex positions");
	auto sts = src.read_multiple<SkyTexCoord>(header.data + header.st_offset, header.vertex_count, "texture coordinates");
	for(s32 i = 0; i < header.vertex_count; i++) {
		auto position = glm::vec3(
			vertices[i].x / (INT16_MAX / 8.f),
			vertices[i].y / (INT16_MAX / 8.f),
			vertices[i].z / (INT16_MAX / 8.f));
		auto st = glm::vec2(sts[i].s / (INT16_MAX / 8.f), sts[i].t / (INT16_MAX / 8.f));
		ColourAttribute colour = {255, 255, 255};
		if(vertices[i].alpha == 0x80) {
			colour.a = 0xff;
		} else {
			colour.a = vertices[i].alpha * 2;
		}
		dest.vertices.emplace_back(position, colour, st);
	}
	
	SubMesh* submesh = nullptr;
	if(dest.submeshes.size() >= 1) {
		submesh = &dest.submeshes.back();
	}
	
	auto faces = src.read_multiple<SkyFace>(header.data + header.tri_offset, header.tri_count, "faces");
	for(const SkyFace& face : faces) {
		if(submesh == nullptr || submesh->material != face.texture) {
			verify(face.texture < texture_count, "Sky has bad texture data.");
			submesh = &dest.submeshes.emplace_back();
			if(textured) {
				verify(face.texture < texture_count, "Sky has bad texture data.");
				submesh->material = face.texture + 1;
			} else {
				submesh->material = 0;
			}
		}
		
		submesh->faces.emplace_back(
			vertex_base + face.indices[0],
			vertex_base + face.indices[1],
			vertex_base + face.indices[2]);
	}
}
