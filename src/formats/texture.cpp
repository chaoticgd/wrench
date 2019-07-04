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

texture::texture(stream* texture_segment, uint32_t entry_offset)
	: proxy_stream(texture_segment, 0, -1, "Texture"),
	  _entry_offset(entry_offset) {}

texture_provider::texture_provider(stream* level_file, uint32_t secondary_header_offset)
	: proxy_stream(level_file, secondary_header_offset, -1, "Textures") {}

std::vector<std::unique_ptr<texture>> texture_provider::textures() {
	std::vector<std::unique_ptr<texture>> result;

	auto header = read<level_stream::fmt::secondary_header>(0);
	auto tex_header = read<fmt::header>(header.textures.value);
	uint32_t texture_entry_offset = tex_header.textures.value;
	for(uint32_t i = 0; i < tex_header.num_textures; i++) {
		// TODO: Create texture streams.
	}

	return result;
}
