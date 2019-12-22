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

game_model::game_model(stream* backing, std::size_t offset)
	: _backing(backing, offset, backing->size() - offset) {
	
}

#include "../shapes.h"

std::vector<float> game_model::triangles() const {
	std::vector<float> result = cube_model().triangles();
	
	for(std::size_t submodel = 0; submodel < num_submodels(); submodel++) {
		auto chain = get_vif_chain(submodel);
		for(vif_packet& vpkt : chain) {
			if(vpkt.error != "") {
				continue;
			}
			
			if(vpkt.code.is_unpack() && vpkt.code.unpack.vnvl == +vif_vnvl::V2_16) {
				for(std::size_t i = 1; i < 4; i++) {
					int x = vpkt.data[i] & 0x00ff;
					int y = vpkt.data[i] & 0xff00;
					printf("xy %d %d\n", x, y);
				}
			}
		}
	}
	
	return result;
}

std::size_t game_model::num_submodels() const {
	return (_backing.peek<uint32_t>(0x10) - _backing.peek<uint32_t>(0x4)) / 0x10;
}

std::vector<vif_packet> game_model::get_vif_chain(std::size_t submodel) const {
	uint32_t table = _backing.peek<uint32_t>(0x4);
	if(_vif_chains.find(submodel) == _vif_chains.end()) {
		auto entry = _backing.peek<fmt::submodel_entry>
			(table + submodel * sizeof(fmt::submodel_entry));
		const_cast<vif_chains*>(&_vif_chains)->insert
			({submodel, parse_vif_chain(&_backing, entry.address, entry.qwc)});
	}
	
	//uint8_t vals[8] = { 0x21,  0x0,  0x0, 0x30, 0x30, 0x96, 0xf2, 0x1 };
	//uint64_t val = *(uint64_t*) vals;
	//printf("%s\n", dma_src_tag::parse(val).to_string().c_str());
	
	return _vif_chains.at(submodel);
}
