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

#ifndef WAD_TEXTURE_H
#define WAD_TEXTURE_H

#include "../core/level.h"

packed_struct(GsRamEntry,
	s32 unknown_0; // Type?
	s16 width;
	s16 height;
	s32 offset_1;
	s32 offset_2; // Duplicate of offset_1?
)

packed_struct(TextureEntry,
	/* 0x0 */ s32 data_offset;
	/* 0x4 */ s16 width;
	/* 0x6 */ s16 height;
	/* 0x8 */ s16 unknown_8;
	/* 0xa */ s16 palette;
	/* 0xc */ s16 mipmap;
	/* 0xe */ s16 pad = 0xffff;
)

packed_struct(ParticleTextureEntry,
	/* 0x0 */ s32 palette;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 texture;
	/* 0xc */ s32 side;
)

packed_struct(FXTextureEntry,
	s32 palette;
	s32 texture;
	s32 width;
	s32 height;
)

// This is more of a work struct for the texture deduplicator/writer.
struct PalettedTexture {
	s32 width;
	s32 height;
	Palette palette;
	std::vector<u8> data;
	s32 texture_out_edge = -1;
	bool is_first_occurence = true;
	s32 palette_out_edge = -1;
	s32 texture_offset = -1;
	s32 palette_offset = -1;
	s32 mipmap_offset = -1;
#define TFRAG_TEXTURE_INDEX 0
#define MOBY_TEXTURE_INDEX 1
#define TIE_TEXTURE_INDEX 2
#define SHRUB_TEXTURE_INDEX 3
	std::optional<s32> indices[4];
	fs::path path;
};

struct FlattenedTextureLayout {
	size_t tfrags_begin;
	size_t mobies_begin;
	size_t ties_begin;
	size_t shrubs_begin;
};

std::vector<Texture> read_tfrag_textures(BufferArray<TextureEntry> texture_table, Buffer data, Buffer gs_ram);
std::vector<Texture> read_instance_textures(BufferArray<TextureEntry> texture_table, const u8 indices[16], Buffer data, Buffer gs_ram);
std::vector<Texture> read_particle_textures(BufferArray<ParticleTextureEntry> texture_table, Buffer src);
ArrayRange write_particle_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& src);
std::vector<Texture> read_fx_textures(BufferArray<FXTextureEntry> texture_table, Buffer data);
ArrayRange write_fx_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& src);
std::pair<std::vector<const Texture*>, FlattenedTextureLayout> flatten_textures(const LevelWad& wad);
PalettedTexture adapt_texture(const Texture& src);
void deduplicate_textures(std::vector<PalettedTexture>& textures);
void deduplicate_palettes(std::vector<PalettedTexture>& textures);
void encode_palette_indices(std::vector<PalettedTexture>& textures);

#endif
