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

#include "game_model.h"

#include "../util.h"
#include "dma.h"

game_model::game_model(stream* backing, std::size_t offset)
	: _backing(backing, offset, backing->size() - offset) {
	
}

#include "../shapes.h"

std::vector<float> game_model::triangles() const {
	std::vector<float> result = cube_model().triangles();
	
	proxy_stream& backing = const_cast<proxy_stream&>(_backing);
	fmt::header hdr = backing.read<fmt::header>(0);
	
	std::vector<fmt::dma_chain_entry> entries;
	while(backing.tell() < hdr.dma_tags_end) {
		entries.push_back(backing.read<fmt::dma_chain_entry>());
	}
	
	for(fmt::dma_chain_entry entry : entries) {
		dma_src_tag tag = dma_src_tag::parse(entry.tag);
		
		if(tag.id != +dma_src_id::REFE) {
			throw std::runtime_error("Encountered DMA tag where id != REFE!");
		}
		
		std::size_t offset = 0;
		for(int i = 0; i < 8; i++) {
			int val = backing.peek<uint32_t>(tag.addr + offset * 4);
			std::optional<vif_code> code = vif_code::parse(val);
			if(!code) { break; }
			offset += code->packet_size();
		}
	}
	
	return result;
}

std::vector<dma_packet_info> game_model::get_dma_debug_info() const {
	std::vector<dma_packet_info> result;
	
	proxy_stream& backing = const_cast<proxy_stream&>(_backing);
	fmt::header hdr = backing.read<fmt::header>(0);
	
	std::vector<fmt::dma_chain_entry> entries;
	std::vector<std::size_t> dma_tag_addresses;
	while(backing.tell() < hdr.dma_tags_end) {
		dma_tag_addresses.push_back(backing.tell());
		entries.push_back(backing.read<fmt::dma_chain_entry>());
	}
	
	for(std::size_t i = 0; i < entries.size(); i++) {
		dma_packet_info dma_info;
		dma_info.address = dma_tag_addresses[i];
		
		dma_src_tag tag = dma_src_tag::parse(entries[i].tag);
		dma_info.tag = tag.to_string();
		
		std::size_t offset = 0;
		while(offset < tag.qwc * 16) {
			vif_packet_info vif_info;
			
			int val = backing.peek<uint32_t>(tag.addr + offset);
			std::optional<vif_code> code = vif_code::parse(val);
			if(!code) {
				vif_info.code = "(invalid VIF code)";
				break;
			}
			vif_info.address = tag.addr + offset;
			vif_info.code = code->to_string();
			for(std::size_t i = 0; i < code->packet_size() * 4; i++) {
				vif_info.data.push_back(backing.peek<uint32_t>(tag.addr + offset + i));
			}
			dma_info.vif_packets.push_back(vif_info);
			offset += code->packet_size() * 4;
		}
		
		result.push_back(dma_info);
	}
	
	return result;
}
