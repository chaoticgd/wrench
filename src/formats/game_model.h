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

#ifndef FORMATS_GAME_MODEL_H
#define FORMATS_GAME_MODEL_H

#include <map>

#include "../model.h"
#include "../stream.h"
#include "vif.h"

# /*
#	Parse a game model.
# */

using vif_chains = std::map<std::size_t, std::vector<vif_packet>>;

class game_model : public model {
public:
	struct fmt {
		packed_struct(header,
			uint32_t unknown_0;
			uint32_t submodel_table;
			uint32_t unknown_8;
			uint32_t unknown_c;
		)
		
		packed_struct(submodel_entry,
			uint32_t address;
			uint16_t qwc;
			uint16_t unknown_6;
			uint32_t address_8;
			uint32_t unknown_c;
		)
		
		packed_struct(vertex,
			uint16_t unknown_0;
			uint8_t unknown_2;
			uint8_t unknown_3; // Always equal to 0xf4?
			uint32_t pad;
			uint16_t unknown_8;
			int16_t x;
			int16_t y;
			int16_t z;
		)
	};

	game_model(stream* backing, std::size_t submodel_table_offset, std::size_t num_submodels_);

	std::vector<float> triangles() const override;
	
	std::vector<vif_packet> get_vif_chain(std::size_t submodel) const;
	
	const std::size_t num_submodels;
private:
	std::vector<vif_packet> get_vif_chain_at(std::size_t offset, std::size_t qwc) const;
	fmt::submodel_entry get_submodel_entry(std::size_t submodel) const;
	
	proxy_stream _backing;
	vif_chains _vif_chains;
};

#endif
