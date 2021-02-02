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

#include "vif.h"

#include <iomanip>
#include <sstream>

#include "../util.h"

std::optional<vif_code> vif_code::parse(uint32_t val) {
	vif_code code;
	code.raw = val;
	code.interrupt = bit_range(val, 31, 31);
	code.cmd       = static_cast<vif_cmd>(bit_range(val, 24, 30));
	code.num       = bit_range(val, 16, 23);
	code.num = code.num ? code.num : 256;
	
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
			code.direct.size = code.direct.size ? code.direct.size : 65536;
			break;
		case vif_cmd::DIRECTHL:
			code.directhl.size = bit_range(val, 0, 15);
			code.directhl.size = code.directhl.size ? code.directhl.size : 65536;
			break;
		default:
			if(!code.is_unpack()) {
				return {};
			}
			code.unpack.vn = bit_range(val, 26, 27);
			code.unpack.vl = bit_range(val, 24, 25);
			code.unpack.vnvl = static_cast<vif_vnvl>(bit_range(val, 24, 27));
			code.unpack.flg  = static_cast<vif_flg>(bit_range(val, 15, 15));
			code.unpack.usn  = static_cast<vif_usn>(bit_range(val, 14, 14));
			code.unpack.addr = bit_range(val, 0, 9);
	}
	
	return code;
}

uint32_t vif_code::encode_unpack() {
	if(!is_unpack()) {
		throw std::runtime_error("vif_code::encode_unpack called on a VIF code where cmd != UNPACK.");
	}
	
	uint32_t value;
	value |= (interrupt & 0b1) << 31;
	value |= (((uint32_t) cmd) & 0b11111111) << 24;
	value |= (num & 0b11111111) << 16;
	value |= (static_cast<uint32_t>(unpack.vnvl) & 0b1111) << 24;
	value |= (static_cast<uint32_t>(unpack.flg) & 0b1) << 15;
	value |= (static_cast<uint32_t>(unpack.usn) & 0b1) << 14;
	value |= (unpack.addr & 0b1111111111) << 0;
	return value;
}

bool vif_code::is_unpack() const {
	return (static_cast<int>(cmd) & 0b1100000) == 0b1100000;
}

bool vif_code::is_strow() const {
	return (static_cast<int>(cmd) & 0b0110000) == 0b0110000;
}

std::size_t vif_code::packet_size() const {
	std::size_t result = 0;
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
			result = 1;
			break;
		case vif_cmd::STMASK:
			result = 2;
			break;
		case vif_cmd::STROW:
		case vif_cmd::STCOL:
			result = 5;
			break;
		case vif_cmd::MPG:
			result = 1 + num * 2;
			break;
		case vif_cmd::DIRECT:
			result = 1 + direct.size * 4;
			break;
		case vif_cmd::DIRECTHL:
			result = 1 + directhl.size * 4;
			break;
		default:
			if(is_unpack()) {
				// This is what PCSX2 does when wl <= cl.
				// Assume wl = cl = 4.
				int gsize = ((32 >> unpack.vl) * (unpack.vn + 1)) / 8;
				int size = num * gsize;
				if (size % 4 != 0)
					size += 4 - (size % 4);

				result = 1 + (size / 4);
			}
	}
	
	if(result == 0) {
		throw std::runtime_error("Called vif_code::packet_size() on an invalid vif_code!");
	}
	
	return result * 4;
}

std::string vif_code::to_string() const {
	std::stringstream ss;
	ss << std::hex;
	ss << std::setw(8) << std::setfill('0') << raw << " ";
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
		case vif_cmd::MPG:      ss << "MPG num=" << num << " loadaddr=" << mpg.loadaddr; break;
		case vif_cmd::DIRECT:   ss << "DIRECT size=" << direct.size; break;
		case vif_cmd::DIRECTHL: ss << "DIRECTHL size=" << directhl.size; break;
		default:
			if(!is_unpack()) {
				return "INVALID VIF CODE";
			}
			ss << "UNPACK vnvl=" << enum_to_string({VIF_VNVL_STRINGS}, unpack.vnvl)
			   << " num=" << num
			   << " flg=" << enum_to_string({VIF_FLG_STRINGS}, unpack.flg)
			   << " usn=" << enum_to_string({VIF_USN_STRINGS}, unpack.usn)
			   << " addr=" << unpack.addr;
	}
	ss << " interrupt=" << interrupt
	   << " SIZE=" << packet_size();
	return ss.str();
}

std::vector<vif_packet> parse_vif_chain(const stream* src, std::size_t base_address, std::size_t qwc) {
	std::vector<vif_packet> chain;
	
	std::size_t offset = 0;
	while(offset < qwc * 16) {
		vif_packet vpkt;
		vpkt.address = base_address + offset + 4;
		
		uint32_t val = src->peek<uint32_t>(base_address + offset);
		std::optional<vif_code> code = vif_code::parse(val);
		if(!code) {
			vpkt.error = "failed to parse VIF code";
			chain.push_back(vpkt);
			break;
		}
		
		vpkt.code = *code;
		
		std::size_t packet_size = vpkt.code.packet_size();
		if(packet_size > 0x10000) {
			vpkt.error = "packet_size > 0x10000";
			chain.push_back(vpkt);
			break;
		}
		
		// Skip VIFcode.
		for(std::size_t j = 4; j < packet_size; j++) {
			vpkt.data.push_back(src->peek<uint8_t>(base_address + offset + j));
		}
		
		offset += packet_size;
		if(offset > qwc * 16) {
			vpkt.error = "offset > qwc * 16";
		}
		
		chain.push_back(vpkt);
	}
	
	return chain;
}

int bit_range(uint64_t val, int lo, int hi) {
	return (val >> lo) & ((1 << (hi - lo + 1)) - 1);
}
