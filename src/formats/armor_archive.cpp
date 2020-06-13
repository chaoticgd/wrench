/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include "armor_archive.h"

#include "../util.h"

armor_archive::armor_archive() {}

bool armor_archive::read(stream& iso, toc_table table) {
	std::size_t base_offset = table.header.base_offset.bytes();
	if(table.header.size > 0x1000) {
		return false;
	}
	
	for(std::size_t i = 0; i < table.data.size(); i += 16) {
		auto armor = table.data.read<armor_table_entry>(i);
		if(armor.texture.sectors == 0) {
			continue; // We're probably reading off the end of the array.
		}
		
		// Read the model.
		std::size_t submodel_table_offset =
			iso.peek<uint32_t>(base_offset + armor.model.bytes() + 0x4);
		std::size_t submodel_table_end =
			iso.peek<uint32_t>(base_offset + armor.model.bytes() + 0x10);
		std::size_t num_submodels =
			(submodel_table_end - submodel_table_offset) / 0x10;
		models.emplace_back(&iso, base_offset + armor.model.bytes(), submodel_table_offset, num_submodels);
		
		std::string set_name = std::string("set") + std::to_string(i);
		
		// Single texture.
		std::size_t fip_offset = base_offset + armor.texture.bytes();
		std::optional<texture> tex = create_fip_texture(&iso, fip_offset);
		if(tex) {
			textures.emplace_back(*tex);
			textures.back().name = set_name;
			continue;
		}
	
		// One or more textures.
		auto num_textures = iso.read<uint32_t>(base_offset + armor.texture.bytes());
		if(num_textures > 0x1000) {
			return false;
		}
		
		for(std::size_t j = 0; j < num_textures; j++) {
			auto rel_offset = iso.read<uint32_t>();
			std::size_t abs_offset = base_offset + armor.texture.bytes() + rel_offset;
			std::optional<texture> tex = create_fip_texture(&iso, abs_offset);
			if(tex) {
				textures.emplace_back(*tex);
				textures.back().name =
					set_name + "_part" + std::to_string(j);
			} else {
				std::cerr << "Failed to load 2FIP texture from ARMOR.WAD at "
				          << iso.resource_path().c_str()
				          << "+0x" << std::hex << fip_offset << "\n";
			}
		}
	}
	
	return true;
}
