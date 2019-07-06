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

#include "texture_stream.h"

#include "fip.h"

texture_stream::texture_stream(stream* pixel_data_base, uint32_t pixel_data_offset)
	: proxy_stream(pixel_data_base, 0, -1, "Texture"),
	  _pixel_data_offset(pixel_data_offset) {}

glm::vec2 texture_stream::size() const {
	return glm::vec2(32, 32); // TODO: Find actual size.
}

void texture_stream::set_size(glm::vec2 size_) {

}

std::array<colour, 256> texture_stream::palette() const {
	std::array<colour, 256> colours;
	for(uint32_t i = 0; i < 256; i++) {
		uint8_t entry = i;//decode_palette_index(i);
		colours[i] = { entry, entry, entry };
	}
	return colours;
}

void texture_stream::set_palette(std::array<colour, 256> palette_) {

}

std::vector<uint8_t> texture_stream::pixel_data() const {
	glm::vec2 size_ = size();
	std::vector<uint8_t> result(size_.x * size_.y);
	read_nc(reinterpret_cast<char*>(result.data()), result.size());
	return result;
}

void texture_stream::set_pixel_data(std::vector<uint8_t> pixel_data_) {

}

texture_provider_stream::texture_provider_stream(stream* level_file, uint32_t secondary_header_offset)
	: proxy_stream(level_file, secondary_header_offset, -1, "Textures") {}

void texture_provider_stream::populate(app* a) {
	stream::populate(a);

	auto snd_header = read<level_stream::fmt::secondary_header>(0);

	uint32_t textures_data_ptr = snd_header.texture_data_ptr;
	_pixel_data_base = std::make_unique<proxy_stream>(this, textures_data_ptr, -1);

	uint32_t textures_ptr = snd_header.textures.value;
	proxy_stream texture_header_segment(this, textures_ptr, -1);

	auto tex_header = read<fmt::header>(textures_ptr);
	uint32_t texture_entry_offset = tex_header.textures.value;

	for(uint32_t i = 0; i < tex_header.num_textures; i++) {
		uint32_t pixel_data_offset = texture_header_segment.read<uint32_t>(tex_header.textures.value + i * 16 + 12);
		emplace_child<texture_stream>(_pixel_data_base.get(), pixel_data_offset);
		//proxy_stream pixel_data(&pdata, pixel_data_offset, -1);
		//printf("respath %s\n", pixel_data.resource_path().c_str());
	}
}

std::vector<texture*> texture_provider_stream::textures() {
	return children_of_type<texture>();
}
