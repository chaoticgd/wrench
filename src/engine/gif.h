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

#ifndef ENGINE_GIF_H
#define ENGINE_GIF_H

#include <core/util.h>

packed_struct(GifTag12,
	/* 0x0 */ u32 low;
	/* 0x4 */ u32 mid;
	/* 0x8 */ u32 regs;
	
	u16 nloop() { return low & (0b111111111111111); }
	void set_nloop(u16 field) { low |= field; }
	
	bool eop() { return (low >> 15) & 0b1; }
	void set_eop(bool field) { low |= (!!field) << 15; }
)

packed_struct(GsAdData12,
	/* 0x0 */ s32 data_lo;
	/* 0x4 */ s32 data_hi;
	/* 0x8 */ u8 address;
	/* 0x9 */ u8 pad_9;
	/* 0xa */ u16 pad_a;
)

packed_struct(GsAdData16,
	/* 0x0 */ s32 data_lo;
	/* 0x4 */ s32 data_hi;
	/* 0x8 */ u8 address;
	/* 0x9 */ u8 pad_9;
	/* 0xa */ u16 pad_a;
	/* 0xc */ u32 pad_c;
)

#endif
