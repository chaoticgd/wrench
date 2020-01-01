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

#include <glm/glm.hpp>

#include "../util.h"

game_model::game_model(stream* backing, std::size_t base_offset, std::size_t submodel_table_offset, std::size_t num_submodels_)
	:  num_submodels(num_submodels_),
	  _backing(backing, base_offset, 0),
	  _submodel_table_offset(submodel_table_offset) {}

std::vector<float> game_model::triangles() const {
	std::vector<float> result;
	
	// I'm not entirely sure how the vertex data is stored, and this code
	// doesn't work very well. More research is needed.
	for(std::size_t i = 0; i < num_submodels; i++) {
		auto entry = get_submodel_entry(i);
		uint32_t base = entry.address_8;
		bool last_is_vertex = false;
		for(std::size_t j = 0; j < 0x10000; j++) {
			auto vertex = _backing.peek<fmt::vertex>(base + j * 0x10);
			
			bool is_vertex = vertex.unknown_3 == 0xf4;
			if(last_is_vertex && !is_vertex) {
				break;
			}
			
			if(is_vertex) {
				result.push_back(vertex.x / (float) INT16_MAX);
				result.push_back(vertex.y / (float) INT16_MAX);
				result.push_back(vertex.z / (float) INT16_MAX);
			}
			
			last_is_vertex = is_vertex;
		}
	}
	
	return result;
}

std::vector<vif_packet> game_model::get_vif_chain(std::size_t submodel) const {
	auto entry = get_submodel_entry(submodel);
	return get_vif_chain_at(entry.address, entry.qwc);
}

std::vector<vif_packet> game_model::get_vif_chain_at(std::size_t offset, std::size_t qwc) const {
	if(_vif_chains.find(offset) == _vif_chains.end()) {
		const_cast<vif_chains*>(&_vif_chains)->insert
			({offset, parse_vif_chain(&_backing, offset, qwc)});
	}
	
	return _vif_chains.at(offset);
}

game_model::fmt::submodel_entry game_model::get_submodel_entry(std::size_t submodel) const {
	uint32_t offset = _submodel_table_offset + submodel * sizeof(fmt::submodel_entry);
	return _backing.peek<fmt::submodel_entry>(offset);
}
