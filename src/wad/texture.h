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

struct TextureDedupeRecord {
	const Texture* texture;
	s32 texture_out_edge = -1;
	s32 palette_out_edge = -1;
	s32 texture_offset = -1;
	s32 palette_offset = -1;
	s32 mipmap_offset = -1;
#define TFRAG_TEXTURE_INDEX 0
#define MOBY_TEXTURE_INDEX 1
#define TIE_TEXTURE_INDEX 2
#define SHRUB_TEXTURE_INDEX 3
	Opt<s32> indices[4];
};

Texture read_shared_texture(Buffer texture, Buffer palette, TextureEntry entry, Game game);
s64 write_shared_texture_data(OutBuffer ee, OutBuffer gs, std::vector<GsRamEntry>& table, std::vector<TextureDedupeRecord>& records);
std::vector<Texture> read_particle_textures(BufferArray<ParticleTextureEntry> texture_table, Buffer src, Game game);
ArrayRange write_particle_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& textures);
std::vector<Texture> read_fx_textures(BufferArray<FXTextureEntry> texture_table, Buffer data, Game game);
ArrayRange write_fx_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& textures);
struct TextureDedupeOutput {
	std::vector<TextureDedupeRecord> records;
	s32 tfrags_begin;
	s32 mobies_begin;
	s32 ties_begin;
	s32 shrubs_begin;
};
struct TextureDedupeInput {
	const std::vector<Texture>& tfrag_textures;
	const std::vector<MobyClass>& moby_classes;
	const std::vector<TieClass>& tie_classes;
	const std::vector<ShrubClass>& shrub_classes;
};
TextureDedupeOutput prepare_texture_dedupe_records(TextureDedupeInput& input);
void deduplicate_textures(std::vector<TextureDedupeRecord>& records);
void deduplicate_palettes(std::vector<TextureDedupeRecord>& records);
s64 write_palette(OutBuffer dest, const Palette& palette);

#endif
