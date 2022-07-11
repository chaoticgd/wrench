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

#ifndef ENGINE_SKY_H
#define ENGINE_SKY_H

#include <core/buffer.h>
#include <core/collada.h>
#include <core/texture.h>
#include <core/build_config.h>
#include <engine/basic_types.h>

struct SkyShell {
	std::vector<Mesh> clusters;
	bool textured;
	bool bloom;
	glm::vec3 rotation;
	glm::vec3 angular_velocity;
};

packed_struct(SkyBackgroundColour,
	u8 r;
	u8 g;
	u8 b;
	u8 a;
)

struct Sky {
	SkyBackgroundColour background_colour;
	bool clear_screen;
	std::vector<SkyShell> shells;
	std::vector<Texture> textures;
	s32 maximum_sprite_count;
	std::vector<u8> fx;
};

packed_struct(SkyHeader,
	/* 0x00 */ SkyBackgroundColour background_colour;
	/* 0x04 */ s16 clear_screen;
	/* 0x06 */ s16 shell_count;
	/* 0x08 */ s16 sprite_count;
	/* 0x0a */ s16 maximum_sprite_count;
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

packed_struct(Vec3u16,
	/* 0x0 */ u16 x;
	/* 0x2 */ u16 y;
	/* 0x4 */ u16 z;
)

packed_struct(SkyShellHeader,
	/* 0x0 */ s16 cluster_count;
	/* 0x2 */ s16 flags;
	/* 0x4 */ Vec3u16 rotation;
	/* 0xa */ Vec3u16 angular_velocity;
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

packed_struct(SkyVertex,
	/* 0x0 */ s16 x;
	/* 0x2 */ s16 y;
	/* 0x4 */ s16 z;
	/* 0x6 */ s16 alpha;
)

packed_struct(SkyTexCoord,
	/* 0x0 */ s16 s;
	/* 0x2 */ s16 t;
)

packed_struct(SkyFace,
	/* 0x0 */ u8 indices[3];
	/* 0x3 */ s8 texture;
)

Sky read_sky(Buffer src, Game game, f32 framerate);
void write_sky(OutBuffer dest, const Sky& sky, Game game, f32 framerate);
std::vector<Mesh> generate_clusters(const Mesh& mesh);

#endif
