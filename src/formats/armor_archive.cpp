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
		if(armor.texture == 0) {
			break; // We're probably reading off the end of the array.
		}
		
		std::size_t model_offset_bytes = offset + armor.model * SECTOR_SIZE;
		std::size_t model_size_bytes = armor.model_size * SECTOR_SIZE;
		_models.push_back(std::make_unique<game_model>
			(backing, model_offset_bytes, model_size_bytes));
		
		char magic[4];
		_backing.seek(armor.texture * SECTOR_SIZE);
		_backing.read_n(magic, 4);
		if(std::memcmp(magic, "2FIP", 4) == 0) {
			// Single texture.
			_textures.emplace_back(std::make_unique<fip_texture>
				(backing, offset + armor.texture * SECTOR_SIZE, -1));
			_texture_names[_textures.back().get()] =
				std::string("set") + std::to_string(i);
			continue;
		}
	
		// One or more textures.
		auto num_textures = _backing.read<uint32_t>(armor.texture * SECTOR_SIZE);
		if(num_textures > 0x1000) {
			throw std::runtime_error("Invalid armor_archive: num_textures too big.");
		}
		
		for(std::size_t j = 0; j < num_textures; j++) {
			auto rel_offset = _backing.read<uint32_t>();
			_textures.emplace_back(std::make_unique<fip_texture>
				(backing, offset + armor.texture * SECTOR_SIZE + rel_offset, -1));
			_texture_names[_textures.back().get()] =
				std::string("set") + std::to_string(i) + "_part" + std::to_string(j);
		}
	}
}

std::string armor_archive::display_name() const {
	return "Armor";
}

std::string armor_archive::display_name_of(texture* tex) const {
	return _texture_names.at(tex);
}

std::vector<texture*> armor_archive::textures() {
	return unique_to_raw<texture>(_textures);
}

std::vector<game_model*> armor_archive::models() {
	return unique_to_raw<game_model>(_models);
}
