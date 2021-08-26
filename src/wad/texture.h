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

std::vector<Texture> read_tfrag_textures(BufferArray<TextureEntry> texture_table, Buffer data, Buffer gs_ram);
std::vector<Texture> read_instance_textures(BufferArray<TextureEntry> texture_table, const u8 indices[16], Buffer data, Buffer gs_ram);

#endif
