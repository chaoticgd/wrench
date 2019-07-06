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

#include "level_texture.h"

#include "fip.h"
#include "level_impl.h"

level_texture::level_texture(stream* backing, uint32_t pixel_data_offset)
	: _pixel_data(backing, pixel_data_offset, -1) {}

glm::vec2 level_texture::size() const {
	return glm::vec2(32, 32); // TODO: Find actual size.
}

void level_texture::set_size(glm::vec2 size_) {

}

std::array<colour, 256> level_texture::palette() const {
	std::array<colour, 256> colours;
	for(uint32_t i = 0; i < 256; i++) {
		uint8_t entry = i;//decode_palette_index(i);
		colours[i] = { entry, entry, entry };
	}
	return colours;
}

void level_texture::set_palette(std::array<colour, 256> palette_) {

}

std::vector<uint8_t> level_texture::pixel_data() const {
	glm::vec2 size_ = size();
	std::vector<uint8_t> result(size_.x * size_.y);
	_pixel_data.read_nc(reinterpret_cast<char*>(result.data()), 0, result.size());
	return result;
}

void level_texture::set_pixel_data(std::vector<uint8_t> pixel_data_) {

}

level_texture_provider::level_texture_provider(stream* level_file, uint32_t secondary_header_offset)
	: _backing(level_file, secondary_header_offset, -1, "Textures") {

	auto snd_header = _backing.read<level_impl::fmt::secondary_header>(0);

	uint32_t pixel_data_base = snd_header.tex_pixel_data_base;
	uint32_t textures_ptr = snd_header.textures.value;
	proxy_stream texture_header_segment(&_backing, textures_ptr, -1);

	auto tex_header = _backing.read<fmt::header>(textures_ptr);
	uint32_t texture_entry_offset = tex_header.textures.value;

	for(uint32_t i = 0; i < tex_header.num_textures; i++) {
		uint32_t pixel_data_offset = texture_header_segment.read<uint32_t>(tex_header.textures.value + i * 16 + 12);
		_textures.emplace_back(std::make_unique<level_texture>
			(&_backing, pixel_data_base + pixel_data_offset));
		//proxy_stream pixel_data(&pdata, pixel_data_offset, -1);
		//printf("respath %s\n", pixel_data.resource_path().c_str());
	}
}

std::vector<texture*> level_texture_provider::textures() {
	std::vector<texture*> result(_textures.size());
	std::transform(_textures.begin(), _textures.end(), result.begin(),
		[](auto& ptr) { return ptr.get(); });
	return result;
}
