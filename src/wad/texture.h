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
	s32 data_offset;
	s16 width;
	s16 height;
	s16 unknown_8;
	s16 palette;
	s32 unknown_c;
)

std::vector<Texture> read_textures(BufferArray<TextureEntry> texture_table, const u8 indices[16], Buffer data, Buffer palettes);

#endif
