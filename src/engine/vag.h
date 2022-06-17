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

#ifndef ENGINE_VAG_H
#define ENGINE_VAG_H

#include <core/stream.h>

packed_struct(VagHeader,
	/* 0x00 */ char magic[4]; // "VAGp"
	/* 0x04 */ s32 version;
	/* 0x08 */ s32 reserved_8;
	/* 0x0c */ s32 data_size;
	/* 0x10 */ s32 frequency;
	/* 0x14 */ u8 reserved_14[10];
	/* 0x1e */ u8 channel_count;
	/* 0x1f */ u8 reserved_1f;
	/* 0x20 */ char name[16];
)
static_assert(sizeof(VagHeader) == 0x30);

Sector32 get_vag_size(InputStream& src, Sector32 sector);

#endif
