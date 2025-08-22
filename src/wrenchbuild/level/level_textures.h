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

#ifndef WRENCHBUILD_LEVEL_TEXTURES_H
#define WRENCHBUILD_LEVEL_TEXTURES_H

#include <core/texture.h>
#include <assetmgr/asset_types.h>

#define PSM_RGBA32 0x00
#define PSM_RGBA16 0x01
#define PSM_IDTEX8 0x13

packed_struct(GsRamEntry,
	s32 psm; // 0 == palette RGBA32, 1 == palette RGBA16, 0x13 == IDTEX8
	s16 width;
	s16 height;
	s32 address;
	s32 offset; // For stashed moby textures, this is relative to the start of the stash.
)

packed_struct(TextureEntry,
	/* 0x0 */ s32 data_offset;
	/* 0x4 */ s16 width;
	/* 0x6 */ s16 height;
	/* 0x8 */ s16 type;
	/* 0xa */ s16 palette;
	/* 0xc */ s16 mipmap = -1;
	/* 0xe */ s16 pad = -1;
)

packed_struct(ParticleTextureEntry,
	/* 0x0 */ s32 palette;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 texture;
	/* 0xc */ s32 side;
)

packed_struct(FxTextureEntry,
	/* 0x0 */ s32 palette;
	/* 0x4 */ s32 texture;
	/* 0x8 */ s32 width;
	/* 0xc */ s32 height;
)

packed_struct(ShrubBillboardInfo,
	/* 0x0 */ s16 texture_width = 0;
	/* 0x2 */ s16 texture_height = 0;
	/* 0x4 */ s16 maximum_mipmap_level = 0;
	/* 0x6 */ s16 palette_offset = 0;
	/* 0x8 */ s16 texture_offset = 0;
	/* 0xa */ s16 mipmap_1_offset = 0;
	/* 0xc */ s16 mipmap_2_offset = 0;
	/* 0xe */ s16 mipmap_3_offset = 0;
)

struct LevelTexture
{
	Opt<Texture> texture;
	bool stashed = false;
	s32 out_edge = -1;
	s32 palette_out_edge = -1;
	s32 texture_offset = -1;
	s32 palette_offset = -1;
	s32 mipmap_offset = -1;
#define TFRAG_TEXTURE_TABLE 0
#define MOBY_TEXTURE_TABLE 1
#define TIE_TEXTURE_TABLE 2
#define SHRUB_TEXTURE_TABLE 3
	Opt<s32> indices[4];
};

struct LevelTextureRange
{
	s32 table;
	s32 begin;
	s32 end;
};

struct SharedLevelTextures
{
	std::vector<LevelTexture> textures;
	LevelTextureRange tfrag_range;
	LevelTextureRange moby_range;
	LevelTextureRange tie_range;
	LevelTextureRange shrub_range;
};

void unpack_level_materials(
	CollectionAsset& dest,
	const u8 indices[16],
	const std::vector<TextureEntry>& textures,
	InputStream& data,
	InputStream& gs_ram,
	Game game,
	s32 moby_stash_addr = -1);
void unpack_level_material(
	MaterialAsset& dest,
	const TextureEntry& entry,
	InputStream& data,
	InputStream& gs_ram,
	Game game,
	s32 i,
	s32 moby_stash_addr = -1);

void unpack_shrub_billboard_texture(
	TextureAsset& dest, const ShrubBillboardInfo& billboard, InputStream& gs_ram, Game game);
SharedLevelTextures read_level_textures(
	const CollectionAsset& tfrag_materials,
	const CollectionAsset& mobies,
	const CollectionAsset& ties,
	const CollectionAsset& shrubs);
std::tuple<s32, s32> write_shared_level_textures(
	OutputStream& data,
	OutputStream& gs,
	std::vector<GsRamEntry>& gs_table,
	std::vector<LevelTexture>& textures);
ArrayRange write_level_texture_table(
	OutputStream& dest, std::vector<LevelTexture>& textures, LevelTextureRange range);
void write_level_texture_indices(
	u8 dest[16], const std::vector<LevelTexture>& textures, s32 begin, s32 table);

void unpack_particle_textures(
	CollectionAsset& dest,
	InputStream& defs,
	std::vector<ParticleTextureEntry>& entries,
	InputStream& bank,
	Game game);
std::tuple<ArrayRange, std::vector<u8>, s32> pack_particle_textures(
	OutputStream& index, OutputStream& data, const CollectionAsset& particles, Game game);
void unpack_fx_textures(
	LevelWadAsset& core,
	const std::vector<FxTextureEntry>& entries,
	InputStream& fx_bank,
	Game game);
std::tuple<ArrayRange, s32> pack_fx_textures(
	OutputStream& index, OutputStream& data, const CollectionAsset& collection, Game game);

void deduplicate_level_textures(std::vector<LevelTexture>& textures);
void deduplicate_level_palettes(std::vector<LevelTexture>& textures);

#endif
