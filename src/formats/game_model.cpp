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

#include "dma.h"

game_model::game_model(stream* backing, std::size_t offset)
	: _backing(backing, offset, backing->size() - offset) {
	
}

#include "../shapes.h"

std::vector<float> game_model::triangles() const {
	std::vector<float> result = cube_model().triangles();
	
	proxy_stream& backing = const_cast<proxy_stream&>(_backing);
	printf("%s\n", backing.resource_path().c_str());
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

std::vector<std::vector<std::string>> game_model::get_dma_debug_info() const {
	std::vector<std::vector<std::string>> result;
	
	proxy_stream& backing = const_cast<proxy_stream&>(_backing);
	fmt::header hdr = backing.read<fmt::header>(0);
	
	std::vector<fmt::dma_chain_entry> entries;
	while(backing.tell() < hdr.dma_tags_end) {
		entries.push_back(backing.read<fmt::dma_chain_entry>());
	}
	
	for(fmt::dma_chain_entry entry : entries) {
		std::vector<std::string> packet_info;
		
		dma_src_tag tag = dma_src_tag::parse(entry.tag);
		packet_info.push_back(tag.to_string());
		
		std::size_t offset = 0;
		while(offset < tag.qwc * 16) {
			int val = backing.peek<uint32_t>(tag.addr + offset);
			std::optional<vif_code> code = vif_code::parse(val);
			if(!code) {
				packet_info.push_back("(invalid VIF code)");
				break;
			}
			packet_info.push_back(code->to_string());
			offset += code->packet_size() * 4;
		}
		
		result.push_back(packet_info);
	}
	
	return result;
}
