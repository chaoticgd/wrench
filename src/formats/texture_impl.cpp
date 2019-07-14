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

#include "texture_impl.h"

#include "fip.h"
#include "level_impl.h"
#include "fip.h"

texture_impl::texture_impl(stream* backing, vec2i size, uint32_t palette_offset, uint32_t pixel_offset)
	: _backing(backing, 0, -1),
	  _size(size),
	  _palette_offset(palette_offset),
	  _pixel_offset(pixel_offset) {}
	
vec2i texture_impl::size() const {
	return _size;
}

void texture_impl::set_size(vec2i size_) {

}

std::array<colour, 256> texture_impl::palette() const {
	char data[1024];
	_backing.read_nc(data, _palette_offset, 1024);

	std::array<colour, 256> result;
	for(int i = 0; i < 256; i++) {
		result[decode_palette_index(i)] = {
			static_cast<uint8_t>(data[i * 4 + 0]),
			static_cast<uint8_t>(data[i * 4 + 1]),
			static_cast<uint8_t>(data[i * 4 + 2]),
			static_cast<uint8_t>(data[i * 4 + 3])
		};
	} return result;
}

void texture_impl::set_palette(std::array<colour, 256> palette_) {

}

std::vector<uint8_t> texture_impl::pixel_data() const {
	vec2i size_ = size();
	std::vector<uint8_t> result(size_.x * size_.y);
	_backing.read_nc(reinterpret_cast<char*>(result.data()), _pixel_offset, result.size());
	return result;
}

void texture_impl::set_pixel_data(std::vector<uint8_t> pixel_data_) {

}

std::string texture_impl::palette_path() const {
	std::stringstream hex;
	hex << _palette_offset;
	return _backing.resource_path() + "+0x" + hex.str();
}

std::string texture_impl::pixel_data_path() const {
	std::stringstream hex;
	hex << _pixel_offset;
	return _backing.resource_path() + "+0x" + hex.str();
}

texture_provider_impl::texture_provider_impl(
		stream* backing,
		uint32_t table_offset,
		uint32_t data_offset,
		uint32_t num_textures,
		std::string display_name_)
	: _display_name(display_name_) {

	backing->seek(table_offset);

	uint32_t last_palette = data_offset;

	for(uint32_t i = 0; i < num_textures; i++) {
		auto entry = backing->read<fmt::texture_entry>();
		_textures.emplace_back(std::make_unique<texture_impl>
			(backing, vec2i { entry.width, entry.height}, last_palette, data_offset + entry.pixel_data));
		if(entry.height == 0) {
			last_palette = data_offset + entry.pixel_data;
		}
	}
}

std::string texture_provider_impl::display_name() const {
	return _display_name;
}

std::vector<texture*> texture_provider_impl::textures() {
	std::vector<texture*> result(_textures.size());
	std::transform(_textures.begin(), _textures.end(), result.begin(),
		[](auto& ptr) { return ptr.get(); });
	return result;
}

level_texture_provider::level_texture_provider(
	stream* level_file,
	uint32_t secondary_header_offset,
	std::string display_name_) {

	auto snd_header = level_file->read<level_impl::fmt::secondary_header>(secondary_header_offset);
	uint32_t pixel_data_offet = snd_header.tex_pixel_data_base;
	uint32_t textures_ptr = snd_header.textures.value;
	auto tex_header = level_file->read<fmt::header>(secondary_header_offset + textures_ptr);

	_impl.emplace(
		level_file,
		secondary_header_offset + textures_ptr + tex_header.textures.value,
		secondary_header_offset + pixel_data_offet,
		tex_header.num_textures,
		display_name_
	);
}

std::string level_texture_provider::display_name() const {
	return _impl->display_name();
}

std::vector<texture*> level_texture_provider::textures() {
	return _impl->textures();
}

fip_scanner::fip_scanner(
	stream* backing, 
	uint32_t offset,
	uint32_t size,
	std::string display_name,
	worker_logger& log)
	: _search_space(backing, offset, size),
	  _display_name(display_name) {
		
		log << "Importing " << display_name << "... ";

		char magic[4];
		for(uint32_t i = 0; i < _search_space.size(); i += 0x10) {
			_search_space.seek(i);
			_search_space.read_n(magic, 4);
			if(validate_fip(magic)) {
				auto header = _search_space.read<fip_header>(i);
				_textures.emplace_back(std::make_unique<texture_impl>(
					&_search_space,
					vec2i { static_cast<int>(header.width), static_cast<int>(header.height) },
					i + 32,
					i + sizeof(fip_header)));
			}
		}

		log << "DONE!\n";
}

std::string fip_scanner::display_name() const {
	return _display_name;
}

std::vector<texture*> fip_scanner::textures() {
	std::vector<texture*> result(_textures.size());
	std::transform(_textures.begin(), _textures.end(), result.begin(),
		[](auto& ptr) { return ptr.get(); });
	return result;
}
