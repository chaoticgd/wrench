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

armor_archive::armor_archive(stream* backing, std::size_t offset, std::size_t size)
	: _backing(backing, offset, size) {
	
	uint32_t header_size = _backing.read<fmt::header>(0).header_size;
	if(header_size > 0x1000) {
		throw std::runtime_error("Invalid armor_archive: header_size too big.");
	}
	std::vector<fmt::armor> armor_list((header_size - 8) / 16);
	_backing.read_v(armor_list);
	for(std::size_t i = 0; i < armor_list.size(); i++) {
		auto armor = armor_list[i];
		if(armor.texture.sectors == 0) {
			continue; // We're probably reading off the end of the array.
		}
		
		// Read the model.
		std::size_t submodel_table_offset =
			_backing.peek<uint32_t>(armor.model.bytes() + 0x4);
		std::size_t submodel_table_end =
			_backing.peek<uint32_t>(armor.model.bytes() + 0x10);
		std::size_t num_submodels =
			(submodel_table_end - submodel_table_offset) / 0x10;
		models.emplace_back(backing, offset + armor.model.bytes(), submodel_table_offset, num_submodels);
		
		std::string set_name = std::string("set") + std::to_string(i);
		
		// Single texture.
		std::size_t fip_offset = offset + armor.texture.bytes();
		std::optional<texture> tex = create_fip_texture(backing, fip_offset);
		if(tex) {
			textures.emplace_back(*tex);
			textures.back().name = set_name;
			continue;
		}
	
		// One or more textures.
		auto num_textures = _backing.read<uint32_t>(armor.texture.bytes());
		if(num_textures > 0x1000) {
			throw std::runtime_error("Invalid armor_archive: num_textures too big.");
		}
		
		for(std::size_t j = 0; j < num_textures; j++) {
			auto rel_offset = _backing.read<uint32_t>();
			std::size_t abs_offset = offset + armor.texture.bytes() + rel_offset;
			std::optional<texture> tex = create_fip_texture(backing, abs_offset);
			if(tex) {
				textures.emplace_back(*tex);
				textures.back().name =
					set_name + "_part" + std::to_string(j);
			} else {
				std::cerr << "Failed to load 2FIP texture from ARMOR.WAD at "
				          << backing->resource_path().c_str()
				          << "+0x" << std::hex << fip_offset << "\n";
			}
		}
	}
}
