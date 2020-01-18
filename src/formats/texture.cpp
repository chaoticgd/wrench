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

#include "texture.h"

#include "../util.h" // int_to_hex
#include "fip.h" // decode_palette_index

texture::texture(stream* backing, std::size_t pixel_data_offset, std::size_t palette_offset, vec2i size)
	: _backing(backing), _pixel_data_offset(pixel_data_offset), _palette_offset(palette_offset), _size(size) {}

vec2i texture::size() const {
	return _size;
}

std::array<colour, 256> texture::palette() const {
	char data[1024];
	_backing->peek_n(data, _palette_offset, 1024);

	std::array<colour, 256> result;
	for(int i = 0; i < 256; i++) {
		result[decode_palette_index(i)] = {
			static_cast<uint8_t>(data[i * 4 + 0]),
			static_cast<uint8_t>(data[i * 4 + 1]),
			static_cast<uint8_t>(data[i * 4 + 2]),
			static_cast<uint8_t>(data[i * 4 + 3])
		};
	}
	return result;
}

void texture::set_palette(std::array<colour, 256> palette_) {
	std::array<char, 1024> colours;
	for(int i =0 ; i < 256; i++) {
		colour c = palette_[decode_palette_index(i)];
		colours[i * 4 + 0] = c.r;
		colours[i * 4 + 1] = c.g;
		colours[i * 4 + 2] = c.b;
		colours[i * 4 + 3] = c.a;
	}
	_backing->seek(_palette_offset);
	_backing->write_n(colours.data(), colours.size());
}

std::vector<uint8_t> texture::pixel_data() const {
	vec2i size_ = size();
	std::vector<uint8_t> result(size_.x * size_.y);
	_backing->peek_n(reinterpret_cast<char*>(result.data()), _pixel_data_offset, result.size());
	return result;
}

void texture::set_pixel_data(std::vector<uint8_t> pixel_data_) {
	_backing->seek(_pixel_data_offset);
	_backing->write_n(reinterpret_cast<char*>(pixel_data_.data()), pixel_data_.size());
}

std::string texture::palette_path() const {
	return _backing->resource_path() + "+0x" + int_to_hex(_palette_offset);
}

std::string texture::pixel_data_path() const {
	return _backing->resource_path() + "+0x" + int_to_hex(_pixel_data_offset);
}

std::optional<texture> create_fip_texture(stream* backing, std::size_t offset) {
	fip_header header = backing->peek<fip_header>(offset);
	if(!validate_fip(header.magic)) {
		return {};
	}
	vec2i size { header.width, header.height };
	std::size_t pixel_offset = offset + sizeof(fip_header);
	std::size_t palette_offset = offset + offsetof(fip_header, palette);
	return texture(backing, pixel_offset, palette_offset, size);
}

std::vector<const texture*> texture_provider::textures() const {
	auto textures_ = const_cast<texture_provider*>(this)->textures();
	return std::vector<const texture*>(textures_.begin(), textures_.end());
}
