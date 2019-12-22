/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef FORMATS_DMA_H
#define FORMATS_DMA_H

#include <string>
#include <optional>
#include <better-enums/enum.h>

#include "../stream.h"

# /*
#	Parse PS2 VIF DMA chains.
#	This is how models are stored on disc.
# */

enum class vif_cmd {
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

BETTER_ENUM(vif_vn, char,
	ONE   = 0b00,
	TWO   = 0b01,
	THREE = 0b10,
	FOUR  = 0b11
)

BETTER_ENUM(vif_vl, char,
	QWORD = 0b00,
	DWORD = 0b01,
	BYTE  = 0b10,
	B5551 = 0b11
)

BETTER_ENUM(vif_vnvl, char,
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
)

BETTER_ENUM(vif_flg, char,
	DO_NOT_USE_VIF1_TOPS = 0x0,
	USE_VIF1_TOPS        = 0x1
)

BETTER_ENUM(vif_usn, char,
	SIGNED   = 0x0,
	UNSIGNED = 0x1
)

struct vif_code {
	int interrupt;
	vif_cmd cmd;
	int num;
	union {
		struct { int wl; int cl; } stcycl;
		struct { int offset; } offset;
		struct { int base; } base;
		struct { int addr; } itop;
		struct { int mode; } stmod;
		struct { int mask; } mskpath3;
		struct { int mark; } mark;
		struct { int execaddr; } mscal;
		struct { int execaddr; } mscalf;
		struct { int loadaddr; } mpg;
		struct { int size; } direct;
		struct { int size; } directhl;
		struct {
			int vn;
			int vl;
			vif_vnvl vnvl;
			vif_flg flg;
			vif_usn usn;
			int addr;
		} unpack;
	};
	
	vif_code() {}
	static std::optional<vif_code> parse(uint32_t val);
	bool is_unpack() const;
	std::size_t packet_size() const; // In words.
	std::string to_string() const;
};

struct vif_packet {
	std::size_t address;
	vif_code code;
	std::vector<uint32_t> data; // Including the code.
	std::string error;
};

std::vector<vif_packet> parse_vif_chain(const stream* src, std::size_t base_address, std::size_t qwc);

uint64_t bit_range(uint64_t val, int lo, int hi);

#endif
