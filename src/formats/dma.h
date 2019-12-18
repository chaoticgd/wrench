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

BETTER_ENUM(dma_src_id, char,
	REFE = 0b000,
	CNT  = 0b001,
	NEXT = 0b010,
	REF  = 0b011,
	REFS = 0b100,
	CALL = 0b101,
	RET  = 0b110,
	END  = 0b111
)

BETTER_ENUM(dma_pce, char,
	NOTHING  = 0b00,
	RESERVED = 0b01,
	DISABLED = 0b10,
	ENABLED = 0b11
)

struct dma_src_tag {
	int spr;
	int addr;
	int irq;
	dma_src_id id;
	dma_pce pce;
	int qwc;
	
	dma_src_tag() : id(dma_src_id::END), pce(dma_pce::NOTHING) {}
	static dma_src_tag parse(uint64_t val);
	std::string to_string() const;
};

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

BETTER_ENUM(vif_flg, char,
	DO_NOT_USE_VIF1_TOPS = 0x0,
	USE_VIF1_TOPS        = 0x1
)

BETTER_ENUM(vif_usn, char,
	SIGNED   = 0x0,
	UNSIGNED = 0x1
)

struct vif_code {
	int i;
	vif_cmd cmd;
	int num;
	union {
		struct {
			int wl;
			int cl;
		} stcycl;
		struct {
			int offset;
		} offset;
		struct {
			int base;
		} base;
		struct {
			int addr;
		} itop;
		struct {
			int mode;
		} stmod;
		struct {
			int mask;
		} mskpath3;
		struct {
			int mark;
		} mark;
		struct {
			int execaddr;
		} mscal;
		struct {
			int execaddr;
		} mscalf;
		struct {
			int loadaddr;
		} mpg;
		struct {
			int size;
		} direct;
		struct {
			int size;
		} directhl;
		struct {
			vif_vn vn;
			vif_vl vl;
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

int bit_range(uint64_t val, int lo, int hi);

#endif
