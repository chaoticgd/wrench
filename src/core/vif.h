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

#ifndef WAD_VIF_H
#define WAD_VIF_H

#include <core/buffer.h>

#define vu_fixed12_to_float(i) (((s16)i) * (1.f / 4096.f))
#define vu_float_to_fixed12(f) ((u16) roundf((f) * 4096.f))

enum class VifCmd
{
	NOP      = 0b0000000,
	STCYCL   = 0b0000001,
	OFFSET   = 0b0000010,
	BASE     = 0b0000011,
	ITOP     = 0b0000100,
	STMOD    = 0b0000101,
	MSKPATH3 = 0b0000110,
	MARK     = 0b0000111,
	FLUSHE   = 0b0010000,
	FLUSH    = 0b0010001,
	FLUSHA   = 0b0010011,
	MSCAL    = 0b0010100,
	MSCNT    = 0b0010111,
	MSCALF   = 0b0010101,
	STMASK   = 0b0100000,
	STROW    = 0b0110000,
	STCOL    = 0b0110001,
	MPG      = 0b1001010,
	DIRECT   = 0b1010000,
	DIRECTHL = 0b1010001
};

enum class VifVnVl
{
	S_32     = 0b0000,
	S_16     = 0b0001,
	ERR_0010 = 0b0010,
	ERR_0011 = 0b0011,
	V2_32    = 0b0100,
	V2_16    = 0b0101,
	V2_8     = 0b0110,
	ERR_0111 = 0b0111,
	V3_32    = 0b1000,
	V3_16    = 0b1001,
	V3_8     = 0b1010,
	ERR_1011 = 0b1011,
	V4_32    = 0b1100,
	V4_16    = 0b1101,
	V4_8     = 0b1110,
	V4_5     = 0b1111
};
extern const char* VIF_VNVL_STRINGS[16];

enum class VifFlg
{
	DO_NOT_USE_VIF1_TOPS = 0x0,
	USE_VIF1_TOPS        = 0x1
};
extern const char* VIF_FLG_STRINGS[2];

enum class VifUsn
{
	SIGNED   = 0x0,
	UNSIGNED = 0x1
};
extern const char* VIF_USN_STRINGS[2];

struct VifCode
{
	u32 raw = 0;
	s32 interrupt;
	VifCmd cmd;
	s32 num;
	union {
		struct { s32 wl; s32 cl; } stcycl;
		struct { s32 offset; } offset;
		struct { s32 base; } base;
		struct { s32 addr; } itop;
		struct { s32 mode; } stmod;
		struct { s32 mask; } mskpath3;
		struct { s32 mark; } mark;
		struct { s32 execaddr; } mscal;
		struct { s32 execaddr; } mscalf;
		struct { s32 loadaddr; } mpg;
		struct { s32 size; } direct;
		struct { s32 size; } directhl;
		struct {
			VifVnVl vnvl;
			VifFlg flg;
			VifUsn usn;
			s32 addr;
		} unpack;
	};
	
	u32 encode_unpack() const;
	bool is_unpack() const;
	bool is_strow() const;
	s64 packet_size() const; // In bytes.
	std::string to_string() const;
	
	s32 element_size() const;
	s32 vn() const;
	s32 vl() const;
};

struct VifPacket
{
	s64 offset;
	VifCode code;
	Buffer data;
	std::string error;
};

packed_struct(VifSTROW,
	/* 0x0 */ s32 vif1_r0;
	/* 0x4 */ s32 vif1_r1;
	/* 0x8 */ s32 vif1_r2;
	/* 0xc */ s32 vif1_r3;
)

Opt<VifCode> read_vif_code(u32 val);
std::vector<VifPacket> read_vif_command_list(Buffer src);
std::vector<VifPacket> filter_vif_unpacks(std::vector<VifPacket>& src);
void write_vif_packet(OutBuffer dest, const VifPacket& packet);

#endif
