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

#include "../level.h"

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

// This is more of a work struct for the texture deduplicator/writer.
struct PalettedTexture {
	s32 width;
	s32 height;
	std::array<Colour32, 256> palette;
	std::vector<u8> data;
	bool duplicate_data = true;
	bool duplicate_palette = true;
	s64 data_offset;
	s64 palette_offset;
	s64 mipmap_offset;
};

struct FlattenedTextureLayout {
	size_t tfrags_begin;
	size_t mobies_begin;
	size_t ties_begin;
	size_t shrubs_begin;
};

// See write_assets in primary.cpp for usage.
std::vector<Texture> read_tfrag_textures(BufferArray<TextureEntry> texture_table, Buffer data, Buffer gs_ram);
std::vector<Texture> read_instance_textures(BufferArray<TextureEntry> texture_table, const u8 indices[16], Buffer data, Buffer gs_ram);
std::pair<std::vector<const Texture*>, FlattenedTextureLayout> flatten_textures(const LevelWad& wad);
PalettedTexture find_suboptimal_palette(const Texture& src);
std::vector<PalettedTexture*> deduplicate_textures(std::vector<PalettedTexture>& src);
std::vector<PalettedTexture*> deduplicate_palettes(std::vector<PalettedTexture>& src);

#endif
