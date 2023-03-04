/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef CORE_MEMORY_CARD_H
#define CORE_MEMORY_CARD_H

#include <core/buffer.h>

namespace memory_card {

enum SectionType : s32 {
	TERMINATOR = -1,
	SPECIAL = 6000
};

struct Section {
	SectionType type;
	std::vector<u8> data;
};

struct SaveGame {
	std::vector<Section> game;
	std::vector<std::vector<Section>> levels;
};

SaveGame read(Buffer src);
std::vector<Section> read_sections(Buffer src, s64& pos);
void write(OutBuffer dest, const SaveGame& save);
u32 checksum(Buffer src);

enum FieldType {
	U8, S8, BOOL8,
	U16, S16,
	U32, S32, FLOAT32
};

struct Field {
	FieldType type;
	union {
		u8 val_u8;
		s8 val_s8;
		bool val_bool8;
		u16 val_u16;
		s16 val_s16;
		u32 val_u32;
		s32 val_s32;
		u64 val_u64;
		s64 val_s64;
		f32 val_float32;
	};
};

}

#endif
