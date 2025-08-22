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

enum GsPrimitiveType
{
	GS_PRIMITIVE_POINT = 0b000,
	GS_PRIMITIVE_LINE = 0b001,
	GS_PRIMITIVE_LINE_STRIP = 0b010,
	GS_PRIMITIVE_TRIANGLE = 0b011,
	GS_PRIMITIVE_TRIANGLE_STRIP = 0b100,
	GS_PRIMITIVE_TRIANGLE_FAN = 0b101,
	GS_PRIMITIVE_SPRITE = 0b110,
	GS_PRIMITIVE_MYSTERY = 0b111
};

packed_struct(GsPrimRegister,
	u32 val = 0;
	
	// Drawing primitive type
	s32 primitive() const { return bit_range(val, 0, 2); }
	void set_primitive(u32 field) { val |= field; }
	
	// Shading method
	s32 iip() const { return bit_range(val, 3, 3); }
	void set_iip(u32 field) { val |= (field << 3); }
	
	// Texture mapping
	s32 tme() const { return bit_range(val, 4, 4); }
	void set_tme(u32 field) { val |= (field << 4); }
	
	// Fogging
	s32 fge() const { return bit_range(val, 5, 5); }
	void set_fge(u32 field) { val |= (field << 5); }
	
	// Alpha blending
	s32 abe() const { return bit_range(val, 6, 6); }
	void set_abe(u32 field) { val |= (field << 6); }
	
	// Antialiasing
	s32 aa1() const { return bit_range(val, 7, 7); }
	void set_aa1(u32 field) { val |= (field << 7); }
	
	// Method of specifying texture coordinates
	s32 fst() const { return bit_range(val, 8, 8); }
	void set_fst(u32 field) { val |= (field << 8); }
	
	// Context
	s32 ctxt() const { return bit_range(val, 9, 9); }
	void set_ctxt(u32 field) { val |= (field << 9); }
	
	// Fragment value control
	s32 fix() const { return bit_range(val, 10, 10); }
	void set_fix(u32 field) { val |= (field << 10); }
)

enum GifDataFormat
{
	GIF_DATA_FORMAT_PACKED = 0b00,
	GIF_DATA_FORMAT_REGLIST = 0b01,
	GIF_DATA_FORMAT_IMAGE = 0b10,
	GIF_DATA_FORMAT_DISABLE = 0b11
};

packed_struct(GifTag12,
	/* 0x0 */ u64 low;
	/* 0x8 */ u32 regs;
	
	// Repeat count
	s32 nloop() const { return bit_range(low, 0, 14); }
	void set_nloop(u64 field) { low |= (field & 0b111111111111111); }
	
	// End of packet marker
	s32 eop() const { return bit_range(low, 15, 15); }
	void set_eop(u64 field) { low |= (!!field << 15); }
	
	// PRIM field enable bit
	s32 pre() const { return bit_range(low, 46, 46); }
	void set_pre(u64 field) { low |= (field << 46); }
	
	// PRIM register
	s32 prim() const { return bit_range(low, 47, 57); }
	void set_prim(u64 field) { low |= (field << 47); }
	
	// Data format
	s32 flg() const { return bit_range(low, 58, 59); }
	void set_flg(u64 field) { low |= (field << 58); }
	
	// Register descriptor count
	s32 nreg() const { return bit_range(low, 60, 63); }
	void set_nreg(u64 field) { low |= (field << 60); }
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

enum GifAdAddress
{
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
