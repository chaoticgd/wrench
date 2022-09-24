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

packed_struct(GifAdData12,
	/* 0x0 */ s32 data_lo;
	/* 0x4 */ s32 data_hi;
	/* 0x8 */ u8 address;
	/* 0x9 */ u8 pad_9;
	/* 0xa */ u16 pad_a;
)

packed_struct(GifAdData16,
	/* 0x0 */ s32 data_lo;
	/* 0x4 */ s32 data_hi;
	/* 0x8 */ u8 address;
	/* 0x9 */ u8 pad_9;
	/* 0xa */ u16 pad_a;
	/* 0xc */ u32 pad_c;
)

enum GifAdAddress {
	GIF_AD_PRIM       = 0x00,
	GIF_AD_RGBAQ      = 0x01,
	GIF_AD_ST         = 0x02,
	GIF_AD_UV         = 0x03,
	GIF_AD_XYZF2      = 0x04,
	GIF_AD_XYZ2       = 0x05,
	GIF_AD_TEX0_1     = 0x06,
	GIF_AD_TEX0_2     = 0x07,
	GIF_AD_CLAMP_1    = 0x08,
	GIF_AD_CLAMP_2    = 0x09,
	GIF_AD_FOG        = 0x0a,
	GIF_AD_XYZF3      = 0x0c,
	GIF_AD_XYZ3       = 0x0d,
	GIF_AD_NOP        = 0x0f,
	GIF_AD_TEX1_1     = 0x14,
	GIF_AD_TEX1_2     = 0x15,
	GIF_AD_TEX2_1     = 0x16,
	GIF_AD_TEX2_2     = 0x17,
	GIF_AD_XYOFFSET_1 = 0x18,
	GIF_AD_XYOFFSET_2 = 0x19,
	GIF_AD_PRMODECONT = 0x1a,
	GIF_AD_PRMODE     = 0x1b,
	GIF_AD_TEXCLUT    = 0x1c,
	GIF_AD_SCANMSK    = 0x22,
	GIF_AD_MIPTBP1_1  = 0x34,
	GIF_AD_MIPTBP1_2  = 0x35,
	GIF_AD_MIPTBP2_1  = 0x36,
	GIF_AD_MIPTBP2_2  = 0x37,
	GIF_AD_TEXA       = 0x3b,
	GIF_AD_FOGCOL     = 0x3d,
	GIF_AD_TEXFLUSH   = 0x3f,
	GIF_AD_SCISSOR_1  = 0x40,
	GIF_AD_SCISSOR_2  = 0x41,
	GIF_AD_ALPHA_1    = 0x42,
	GIF_AD_ALPHA_2    = 0x43,
	GIF_AD_DIMX       = 0x44,
	GIF_AD_DTHE       = 0x45,
	GIF_AD_COLCLAMP   = 0x46,
	GIF_AD_TEST_1     = 0x47,
	GIF_AD_TEST_2     = 0x48,
	GIF_AD_PABE       = 0x49,
	GIF_AD_FBA_1      = 0x4a,
	GIF_AD_FBA_2      = 0x4b,
	GIF_AD_FRAME_1    = 0x4c,
	GIF_AD_FRAME_2    = 0x4d,
	GIF_AD_ZBUF_1     = 0x4e,
	GIF_AD_ZBUF_2     = 0x4f,
	GIF_AD_BITBLTBUF  = 0x50,
	GIF_AD_TRXPOS     = 0x51,
	GIF_AD_TRXREG     = 0x52,
	GIF_AD_TRXDIR     = 0x53,
	GIF_AD_HWREG      = 0x54,
	GIF_AD_SIGNAL     = 0x60,
	GIF_AD_FINISH     = 0x61,
	GIF_AD_LABEL      = 0x62,
};

#endif
