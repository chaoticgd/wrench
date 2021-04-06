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

bool armor_archive::read(stream& file_) {
	file.buffer.resize(file_.size());
	file_.seek(0);
	file_.read_n(file.buffer.data(), file.buffer.size());
	
	uint32_t header_size = file.read<uint32_t>(0);
	size_t base_offset = file.read<sector32>(4).bytes();
	if(header_size > 0x1000 || header_size < 8) {
		return false;
	}
	
	std::vector<armor_table_entry> lumps((header_size - 8) / sizeof(armor_table_entry));
	file.read_v(lumps);
	
	for(size_t i = 0; i < lumps.size(); i++) {
		armor_table_entry& armor = lumps[i];
		if(armor.texture.sectors == 0) {
			continue; // We're probably reading off the end of the array.
		}
		
		// Read the model.
		auto model_header = file.read<moby_model_armor_header>(base_offset + armor.model.bytes());
		uint32_t submodel_table_offset = model_header.submodel_table_offset;
		if(submodel_table_offset > 0x10) {
			if(models.size() > 10) {
				continue; // Hack to get R&C3's ARMOR.WAD loading.
			}
			return false;
		}
		if(armor.model.bytes() == 0) {
			continue;
		}
		
		moby_model& mdl = models.emplace_back(
			&file,
			base_offset + armor.model.bytes(),
			armor.model_size.bytes(),
			moby_model_header_type::ARMOR);
		mdl.set_name("armor " + std::to_string(i));
		mdl.read();
		
		std::string set_name = std::string("set") + std::to_string(i);
		
		// Single texture.
		size_t pif_offset = base_offset + armor.texture.bytes();
		std::optional<texture> tex = create_fip_texture(&file, pif_offset);
		if(tex) {
			mdl.texture_indices.push_back(textures.size());
			textures.emplace_back(std::move(*tex));
			textures.back().name = set_name;
			continue;
		}
	
		// One or more textures.
		auto num_textures = file.read<uint32_t>(base_offset + armor.texture.bytes());
		if(num_textures > 0x1000) {
			return false;
		}
		
		for(std::size_t j = 0; j < num_textures; j++) {
			auto rel_offset = file.read<uint32_t>();
			std::size_t abs_offset = base_offset + armor.texture.bytes() + rel_offset;
			std::optional<texture> tex = create_fip_texture(&file, abs_offset);
			if(tex) {
				mdl.texture_indices.push_back(textures.size());
				textures.emplace_back(std::move(*tex));
				textures.back().name =
					set_name + "_part" + std::to_string(j);
			} else {
				std::cerr << "Failed to load PIF texture from armor file at "
				          << std::hex << (base_offset + pif_offset) << ".";
			}
		}
	}
	return true;
}
