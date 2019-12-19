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

#include "../model.h"
#include "../stream.h"

# /*
#	Parse a game model.
# */

struct vif_packet_info {
	std::size_t address;
	std::string code;
	std::vector<uint8_t> data;
};

struct dma_packet_info {
	std::size_t address;
	std::string tag;
	std::vector<vif_packet_info> vif_packets;
};

class game_model : public model {
public:
	struct fmt {
		packed_struct(header,
			uint32_t unknown_0;
			uint32_t unknown_4;
			uint32_t unknown_8;
			uint32_t unknown_c;
			uint32_t dma_tags_end;
		)
		
		packed_struct(dma_chain_entry,
			uint64_t tag;
			uint32_t unknown_8;
			uint32_t unknown_c;
		)
	};

	game_model(stream* backing, std::size_t offset);

	std::vector<float> triangles() const override;
	
	// Returns a list in the form:
	// - dma_src_tag ...
	//   - vif_code ...
	//   - vif_code ...
	//     ...
	// - dma_src_tag ...
	//   - vif_code ...
	//   - vif_code ...
	//     ...
	// where the DMA tag is the first element of each sublist.
	std::vector<dma_packet_info> get_dma_debug_info() const;

private:
	proxy_stream _backing;
};

class model_provider {
public:
	virtual ~model_provider() = default;
	
	virtual std::vector<game_model*> models() = 0;
};

#endif
