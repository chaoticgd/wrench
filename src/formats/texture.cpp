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

#include "texture.h"

uint32_t locate_secondary_header(stream& level_file) {
	uint32_t moby_wad = level_data::locate_moby_wad(level_file);
	uint32_t secondary_hdr_offset = level_data::locate_secondary_header
		(level_file.read<level_data::master_header>(0), moby_wad);
	return secondary_hdr_offset;
}

texture::texture(stream* texture_segment, uint32_t entry_offset)
	: proxy_stream(texture_segment, 0),
	  _entry_offset(entry_offset) {}

texture_provider::texture_provider(stream* level_file)
	: proxy_stream(level_file, locate_secondary_header(*level_file)) {}

std::vector<std::unique_ptr<texture>> texture_provider::textures() {
	std::vector<std::unique_ptr<texture>> result;

	auto header = read<level_data::secondary_header>(0);
	auto tex_header = read<fmt::header>(header.textures_ptr);
	uint32_t texture_entry_offset = tex_header.textures.value;
	for(uint32_t i = 0; i < tex_header.num_textures; i++) {
		// TODO: Create texture streams.
	}

	return result;
}
