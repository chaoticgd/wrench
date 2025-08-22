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

#include "vif.h"

#include <iomanip>
#include <sstream>

const char* VIF_VN_STRINGS[4] = {
	"ONE", "TWO", "THREE", "FOUR"
};

const char* VIF_VL_STRINGS[4] = {
	"QWORD", "DWORD", "BYTE", "B5551"
};

const char* VIF_VNVL_STRINGS[16] = {
	"S_32", "S_16", "ERR_0010", "ERR_0011",
	"V2_32", "V2_16", "V2_8", "ERR_0111",
	"V3_32", "V3_16", "V3_8", "ERR_1011",
	"V4_32", "V4_16", "V4_8", "V4_5"
};

const char* VIF_FLG_STRINGS[2] = {
	"DO_NOT_USE_VIF1_TOPS", "USE_VIF1_TOPS"
};

const char* VIF_USN_STRINGS[2] = {
	"SIGNED", "UNSIGNED"
};

u32 VifCode::encode_unpack() const
{
	verify_fatal(is_unpack());
	u32 value = 0;
	value |= (interrupt & 0b1) << 31;
	value |= (((u32) cmd) & 0b11111111) << 24;
	value |= (num & 0b11111111) << 16;
	value |= (static_cast<u32>(unpack.vnvl) & 0b1111) << 24;
	value |= (static_cast<u32>(unpack.flg) & 0b1) << 15;
	value |= (static_cast<u32>(unpack.usn) & 0b1) << 14;
	value |= (unpack.addr & 0b1111111111) << 0;
	return value;
}

bool VifCode::is_unpack() const
{
	return (static_cast<s32>(cmd) & 0b1100000) == 0b1100000;
}

bool VifCode::is_strow() const
{
	return (static_cast<s32>(cmd) & 0b0110000) == 0b0110000;
}

s64 VifCode::packet_size() const
{
	s64 result = 0;
	switch (cmd) {
		case VifCmd::NOP:
		case VifCmd::STCYCL:
		case VifCmd::OFFSET:
		case VifCmd::BASE:
		case VifCmd::ITOP:
		case VifCmd::STMOD:
		case VifCmd::MSKPATH3:
		case VifCmd::MARK:
		case VifCmd::FLUSHE:
		case VifCmd::FLUSH:
		case VifCmd::FLUSHA:
		case VifCmd::MSCAL:
		case VifCmd::MSCNT:
		case VifCmd::MSCALF:
			result = 1;
			break;
		case VifCmd::STMASK:
			result = 2;
			break;
		case VifCmd::STROW:
		case VifCmd::STCOL:
			result = 5;
			break;
		case VifCmd::MPG:
			result = 1 + num * 2;
			break;
		case VifCmd::DIRECT:
			result = 1 + direct.size * 4;
			break;
		case VifCmd::DIRECTHL:
			result = 1 + directhl.size * 4;
			break;
		default:
			if (is_unpack()) {
				s32 size = num * element_size();
				if (size % 4 != 0)
					size += 4 - (size % 4);

				result = 1 + (size / 4);
			}
	}
	verify_fatal(result != 0);
	return result * 4;
}

std::string VifCode::to_string() const
{
	std::stringstream ss;
	ss << std::hex;
	ss << std::setw(8) << std::setfill('0') << raw << " ";
	switch (cmd) {
		case VifCmd::NOP:      ss << "NOP"; break;
		case VifCmd::STCYCL:   ss << "STCYCL num=" << num << " wl=" << stcycl.wl << " cl=" << stcycl.cl; break;
		case VifCmd::OFFSET:   ss << "OFFSET offset=" << offset.offset; break;
		case VifCmd::BASE:     ss << "BASE base=" << base.base; break;
		case VifCmd::ITOP:     ss << "ITOP addr=" << itop.addr; break;
		case VifCmd::STMOD:    ss << "STMOD mode=" << stmod.mode; break;
		case VifCmd::MSKPATH3: ss << "MSKPATH3 mask=" << mskpath3.mask; break;
		case VifCmd::MARK:     ss << "MARK mark=" << mark.mark; break;
		case VifCmd::FLUSHE:   ss << "FLUSHE"; break;
		case VifCmd::FLUSH:    ss << "FLUSH"; break;
		case VifCmd::FLUSHA:   ss << "FLUSHA"; break;
		case VifCmd::MSCAL:    ss << "MSCAL execaddr=" << mscal.execaddr; break;
		case VifCmd::MSCNT:    ss << "MSCNT"; break;
		case VifCmd::MSCALF:   ss << "MSCALF execaddr=" << mscalf.execaddr; break;
		case VifCmd::STMASK:   ss << "STMASK"; break;
		case VifCmd::STROW:    ss << "STROW"; break;
		case VifCmd::STCOL:    ss << "STCOL"; break;
		case VifCmd::MPG:      ss << "MPG num=" << num << " loadaddr=" << mpg.loadaddr; break;
		case VifCmd::DIRECT:   ss << "DIRECT size=" << direct.size; break;
		case VifCmd::DIRECTHL: ss << "DIRECTHL size=" << directhl.size; break;
		default:
			if (!is_unpack()) {
				return "INVALID VIF CODE";
			}
			ss << "UNPACK vnvl=" << VIF_VNVL_STRINGS[(u32) unpack.vnvl & 0b1111]
			   << " num=" << num
			   << " flg=" << VIF_FLG_STRINGS[(u32) unpack.flg & 0b1]
			   << " usn=" << VIF_USN_STRINGS[(u32) unpack.usn & 0b1]
			   << " addr=" << unpack.addr;
	}
	ss << " interrupt=" << interrupt
	   << " SIZE=" << packet_size();
	return ss.str();
}

s32 VifCode::element_size() const
{
	// This is what PCSX2 does when wl <= cl.
	return ((32 >> vl()) * (vn() + 1)) / 8;
}

s32 VifCode::vn() const
{
	return ((s32) unpack.vnvl & 0b1100) >> 2;
}

s32 VifCode::vl() const
{
	return ((s32) unpack.vnvl) & 0b11;
}

Opt<VifCode> read_vif_code(u32 val)
{
	VifCode code;
	code.raw = val;
	code.interrupt = bit_range(val, 31, 31);
	code.cmd       = static_cast<VifCmd>(bit_range(val, 24, 30));
	code.num       = bit_range(val, 16, 23);
	code.num = code.num ? code.num : 256;
	
	switch (code.cmd) {
		case VifCmd::NOP:
			break;
		case VifCmd::STCYCL:
			code.stcycl.wl = bit_range(val, 8, 15);
			code.stcycl.cl = bit_range(val, 0, 7);
			break;
		case VifCmd::OFFSET:
			code.offset.offset = bit_range(val, 0, 9);
			break;
		case VifCmd::BASE:
			code.base.base = bit_range(val, 0, 9);
			break;
		case VifCmd::ITOP:
			code.itop.addr = bit_range(val, 0, 9);
			break;
		case VifCmd::STMOD:
			code.stmod.mode = bit_range(val, 0, 1);
			break;
		case VifCmd::MSKPATH3:
			code.mskpath3.mask = bit_range(val, 15, 15);
			break;
		case VifCmd::MARK:
			code.mark.mark = bit_range(val, 0, 15);
			break;
		case VifCmd::FLUSHE:
		case VifCmd::FLUSH:
		case VifCmd::FLUSHA:
			break;
		case VifCmd::MSCAL:
			code.mscal.execaddr = bit_range(val, 0, 15);
			break;
		case VifCmd::MSCNT:
			break;
		case VifCmd::MSCALF:
			code.mscalf.execaddr = bit_range(val, 0, 15);
			break;
		case VifCmd::STMASK:
		case VifCmd::STROW:
		case VifCmd::STCOL:
			break;
		case VifCmd::MPG:
			code.mpg.loadaddr = bit_range(val, 0, 15);
			break;
		case VifCmd::DIRECT:
			code.direct.size = bit_range(val, 0, 15);
			code.direct.size = code.direct.size ? code.direct.size : 65536;
			break;
		case VifCmd::DIRECTHL:
			code.directhl.size = bit_range(val, 0, 15);
			code.directhl.size = code.directhl.size ? code.directhl.size : 65536;
			break;
		default:
			if (!code.is_unpack()) {
				return {};
			}
			code.unpack.vnvl = (VifVnVl) bit_range(val, 24, 27);
			code.unpack.flg  = (VifFlg)  bit_range(val, 15, 15);
			code.unpack.usn  = (VifUsn)  bit_range(val, 14, 14);
			code.unpack.addr = bit_range(val, 0, 9);
	}
	
	return code;
}

std::vector<VifPacket> read_vif_command_list(Buffer src)
{
	std::vector<VifPacket> command_list;
	s64 ofs = 0;
	while (ofs < src.size()) {
		VifPacket packet;
		packet.offset = ofs;
		
		Opt<VifCode> code = read_vif_code(src.read<u32>(ofs, "vif code"));
		if (!code.has_value()) {
			packet.error = "failed to disassemble vif code";
			command_list.push_back(packet);
			break;
		}
		packet.code = *code;
		
		s64 packet_size = packet.code.packet_size();
		if (packet_size > 0x10000) {
			packet.error = "vif packet too big";
			command_list.push_back(packet);
			break;
		}
		
		if (ofs + packet_size > src.size()) {
			packet.error = "vif packet overruns buffer";
			command_list.push_back(packet);
			break;
		}
		
		packet.data = src.subbuf(ofs + 4, packet_size - 4);
		command_list.emplace_back(packet);
		
		ofs += packet_size;
	}
	return command_list;
}

std::vector<VifPacket> filter_vif_unpacks(std::vector<VifPacket>& src)
{
	std::vector<VifPacket> dest;
	for (VifPacket& packet : src) {
		if (packet.code.is_unpack()) {
			dest.emplace_back(std::move(packet));
		}
	}
	return dest;
}

void write_vif_packet(OutBuffer dest, const VifPacket& packet)
{
	if (packet.code.is_unpack()) {
		dest.write<u32>(packet.code.encode_unpack());
		dest.vec.insert(dest.vec.end(), packet.data.lo, packet.data.hi);
		dest.pad(4, 0);
	} else if (packet.code.cmd == VifCmd::NOP) {
		dest.write<u32>(0);
	} else {
		verify_not_reached_fatal("Failed to write VIF command list.");
	}
}
