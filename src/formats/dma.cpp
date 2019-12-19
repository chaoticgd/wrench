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

#include "dma.h"

#include <sstream>

#include "../util.h"

dma_src_tag dma_src_tag::parse(uint64_t val) {
	dma_src_tag tag;
	tag.spr  = bit_range(val, 63, 63);
	tag.addr = bit_range(val, 32, 62);
	tag.irq  = bit_range(val, 31, 31);
	tag.id   = dma_src_id::_from_integral(bit_range(val, 28, 30));
	tag.pce  = dma_pce::_from_integral(bit_range(val, 26, 27));
	tag.qwc  = bit_range(val, 0, 15);
	return tag;
}

std::string dma_src_tag::to_string() const {
	std::stringstream ss;
	ss << std::hex;
	ss << "dma_src_tag"
	   << " qwc=" << qwc
	   << " pce=" << pce
	   << " id=" << id
	   << " irq=" << irq
	   << " addr=" << int_to_hex(addr)
	   << " spr=" << spr;
	return ss.str();
}

std::optional<vif_code> vif_code::parse(uint32_t val) {
	vif_code code;
	code.i   = bit_range(val, 31, 31);
	code.cmd = static_cast<vif_cmd>(bit_range(val, 24, 30));
	code.num = bit_range(val, 16, 23);
	
	switch(code.cmd) {
		case vif_cmd::NOP:
			break;
		case vif_cmd::STCYCL:
			code.stcycl.wl = bit_range(val, 8, 15);
			code.stcycl.cl = bit_range(val, 0, 7);
			break;
		case vif_cmd::OFFSET:
			code.offset.offset = bit_range(val, 0, 9);
			break;
		case vif_cmd::BASE:
			code.base.base = bit_range(val, 0, 9);
			break;
		case vif_cmd::ITOP:
			code.itop.addr = bit_range(val, 0, 9);
			break;
		case vif_cmd::STMOD:
			code.stmod.mode = bit_range(val, 0, 1);
			break;
		case vif_cmd::MSKPATH3:
			code.mskpath3.mask = bit_range(val, 15, 15);
			break;
		case vif_cmd::MARK:
			code.mark.mark = bit_range(val, 0, 15);
			break;
		case vif_cmd::FLUSHE:
		case vif_cmd::FLUSH:
		case vif_cmd::FLUSHA:
			break;
		case vif_cmd::MSCAL:
			code.mscal.execaddr = bit_range(val, 0, 15);
			break;
		case vif_cmd::MSCNT:
			break;
		case vif_cmd::MSCALF:
			code.mscalf.execaddr = bit_range(val, 0, 15);
			break;
		case vif_cmd::STMASK:
		case vif_cmd::STROW:
		case vif_cmd::STCOL:
			break;
		case vif_cmd::MPG:
			code.mpg.loadaddr = bit_range(val, 0, 15);
			break;
		case vif_cmd::DIRECT:
			code.direct.size = bit_range(val, 0, 15);
			break;
		case vif_cmd::DIRECTHL:
			code.directhl.size = bit_range(val, 0, 15);
			break;
		default:
			if(!code.is_unpack()) {
				return {};
			}
			code.unpack.vn   = vif_vn ::_from_integral(bit_range(val, 26, 27));
			code.unpack.vl   = vif_vl ::_from_integral(bit_range(val, 24, 25));
			code.unpack.flg  = vif_flg::_from_integral(bit_range(val, 15, 15));
			code.unpack.usn  = vif_usn::_from_integral(bit_range(val, 14, 14));
			code.unpack.addr = bit_range(val, 0, 9);
	}
	
	return code;
}

bool vif_code::is_unpack() const {
	return static_cast<int>(cmd) & 0b1100000;
}

std::size_t vif_code::packet_size() const {
	switch(cmd) {
		case vif_cmd::NOP:
		case vif_cmd::STCYCL:
		case vif_cmd::OFFSET:
		case vif_cmd::BASE:
		case vif_cmd::ITOP:
		case vif_cmd::STMOD:
		case vif_cmd::MSKPATH3:
		case vif_cmd::MARK:
		case vif_cmd::FLUSHE:
		case vif_cmd::FLUSH:
		case vif_cmd::FLUSHA:
		case vif_cmd::MSCAL:
		case vif_cmd::MSCNT:
		case vif_cmd::MSCALF:
			return 1;
		case vif_cmd::STMASK:
			return 2;
		case vif_cmd::STROW:
		case vif_cmd::STCOL:
			return 5;
		case vif_cmd::MPG:
			return 1 + num * 2;
		case vif_cmd::DIRECT:
			return 1 + direct.size * 4;
		case vif_cmd::DIRECTHL:
			return 1 + directhl.size * 4;
		default:
			if(is_unpack()) {
				return 1 + (((32 >> unpack.vl) * (unpack.vn + 1)) * static_cast<int>(std::ceil(num / 32.f)));
			}
	}
	throw std::runtime_error("Called vif_code::packet_size() on an invalid vif_code!");
}

std::string vif_code::to_string() const {
	std::stringstream ss;
	ss << std::hex;
	ss << "vif_code cmd=";
	switch(cmd) {
		case vif_cmd::NOP:      ss << "NOP"; break;
		case vif_cmd::STCYCL:   ss << "STCYCL num=" << num << " wl=" << stcycl.wl << " cl=" << stcycl.cl; break;
		case vif_cmd::OFFSET:   ss << "OFFSET offset=" << offset.offset; break;
		case vif_cmd::BASE:     ss << "BASE base=" << base.base; break;
		case vif_cmd::ITOP:     ss << "ITOP addr=" << itop.addr; break;
		case vif_cmd::STMOD:    ss << "STMOD mode=" << stmod.mode; break;
		case vif_cmd::MSKPATH3: ss << "MSKPATH3 mask=" << mskpath3.mask; break;
		case vif_cmd::MARK:     ss << "MARK mark=" << mark.mark; break;
		case vif_cmd::FLUSHE:   ss << "FLUSHE"; break;
		case vif_cmd::FLUSH:    ss << "FLUSH"; break;
		case vif_cmd::FLUSHA:   ss << "FLUSHA"; break;
		case vif_cmd::MSCAL:    ss << "MSCAL execaddr=" << mscal.execaddr; break;
		case vif_cmd::MSCNT:    ss << "MSCNT"; break;
		case vif_cmd::MSCALF:   ss << "MSCALF execaddr=" << mscalf.execaddr; break;
		case vif_cmd::STMASK:   ss << "STMASK"; break;
		case vif_cmd::STROW:    ss << "STROW"; break;
		case vif_cmd::STCOL:    ss << "STCOL"; break;
		case vif_cmd::MPG:      ss << "MPG loadaddr=" << mpg.loadaddr; break;
		case vif_cmd::DIRECT:   ss << "DIRECT size=" << direct.size; break;
		case vif_cmd::DIRECTHL: ss << "DIRECTHL size=" << directhl.size; break;
		default:
			if(!is_unpack()) {
				return "INVALID VIF CODE";
			}
			ss << "UNPACK vn=" << unpack.vn
			   << " vl=" << unpack.vl
			   << " num=" << num
			   << " flg=" << unpack.flg
			   << " usn=" << unpack.usn 
			   << " addr=" << unpack.addr
			   << " SIZE=" << packet_size();
	}
	return ss.str();
}

int bit_range(uint64_t val, int lo, int hi) {
	return (val >> lo) & ((1 << (hi - lo + 1)) - 1);
}
