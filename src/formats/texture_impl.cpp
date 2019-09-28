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

#include "../util.h"
#include "fip.h"
#include "level_impl.h"

texture_impl::texture_impl(stream* backing, offsets offsets_)
	: _backing(backing, 0, -1),
	  _offsets(offsets_) {}
	
vec2i texture_impl::size() const {
	return {
		_backing.peek<uint16_t>(_offsets.width),
		_backing.peek<uint16_t>(_offsets.height)
	};
}

void texture_impl::set_size(vec2i size_) {
	_backing.write<uint16_t>(_offsets.width,  size_.x);
	_backing.write<uint16_t>(_offsets.height, size_.y);
}

std::array<colour, 256> texture_impl::palette() const {
	char data[1024];
	_backing.peek_n(data, _offsets.palette, 1024);

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
	std::array<char, 1024> colours;
	for(int i =0 ; i < 256; i++) {
		colour c = palette_[decode_palette_index(i)];
		colours[i * 4 + 0] = c.r;
		colours[i * 4 + 1] = c.g;
		colours[i * 4 + 2] = c.b;
		colours[i * 4 + 3] = c.a;
	}
	_backing.seek(_offsets.palette);
	_backing.write_n(colours.data(), colours.size());
}

std::vector<uint8_t> texture_impl::pixel_data() const {
	vec2i size_ = size();
	std::vector<uint8_t> result(size_.x * size_.y);
	_backing.peek_n(reinterpret_cast<char*>(result.data()), _offsets.pixels, result.size());
	return result;
}

void texture_impl::set_pixel_data(std::vector<uint8_t> pixel_data_) {
	_backing.seek(_offsets.pixels);
	_backing.write_n(reinterpret_cast<char*>(pixel_data_.data()), pixel_data_.size());
}

std::string texture_impl::palette_path() const {
	return _backing.resource_path() + "+0x" + int_to_hex(_offsets.palette);
}

std::string texture_impl::pixel_data_path() const {
	return _backing.resource_path() + "+0x" + int_to_hex(_offsets.pixels);
}

level_texture_provider::level_texture_provider(
	stream* backing,
	std::string display_name_) {

	auto header = backing->read<level::fmt::primary_header>(0);
	std::size_t pixel_data_offet = header.tex_pixel_data_base;
	std::size_t snd_header_ptr = header.snd_header.value;
	auto snd_header = backing->read<level::fmt::secondary_header>(snd_header_ptr);
	
	std::size_t last_palette = pixel_data_offet;
	backing->seek(snd_header_ptr + snd_header.textures.value);
	
	for(std::size_t i = 0; i < snd_header.num_textures; i++) {
		std::size_t entry_offset = backing->tell();
		auto entry = backing->read<fmt::texture_entry>();
		texture_impl::offsets offsets { 
			last_palette,
			pixel_data_offet + entry.pixel_data,
			entry_offset + offsetof(fmt::texture_entry, width),
			entry_offset + offsetof(fmt::texture_entry, height)
		};
		_textures.emplace_back(std::make_unique<texture_impl>(backing, offsets));
		if(entry.height == 0) {
			last_palette = pixel_data_offet + entry.pixel_data;
		}
	}
}

std::string level_texture_provider::display_name() const {
	return _display_name;
}

std::vector<texture*> level_texture_provider::textures() {
	std::vector<texture*> result(_textures.size());
	std::transform(_textures.begin(), _textures.end(), result.begin(),
		[](auto& ptr) { return ptr.get(); });
	return result;
}

fip_scanner::fip_scanner(
		stream* backing,
		std::size_t offset,
		std::size_t size,
		std::string display_name,
		worker_logger& log)
	: _search_space(backing, offset, size),
	  _display_name(display_name) {

	log << "Importing " << display_name << "... ";

	char magic[4];
	for(std::size_t i = 0; i < _search_space.size(); i += 0x10) {
		_search_space.seek(i);
		_search_space.read_n(magic, 4);
		if(validate_fip(magic)) {
			texture_impl::offsets offsets {
				i + offsetof(fip_header, palette),
				i + sizeof(fip_header),
				i + offsetof(fip_header, width),
				i + offsetof(fip_header, height)
			};
			_textures.emplace_back(
				std::make_unique<texture_impl>(&_search_space, offsets));
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

racpak_fip_scanner::racpak_fip_scanner(
		iso_stream* iso,
		racpak* archive,
		std::string display_name_,
		worker_logger& log)
	: _display_name(display_name_) {
	
	log << "Importing textures from " << display_name_ << " racpak... ";
	
	for(std::size_t i = 0; i < archive->num_entries(); i++) {
		auto entry = archive->entry(i);
		stream* file;
		if(archive->is_compressed(entry)) {
			file = iso->get_decompressed(archive->base() + entry.offset);
		} else {
			file = archive->open(entry);
		}
		
		if(file == nullptr || file->size() < 0x14) {
			continue;
		}
		
		char magic[0x14];
		file->seek(0);
		file->read_n(magic, 0x14);
		
		std::optional<std::size_t> texture_offset;
		if(validate_fip(magic)) {
			texture_offset = 0;
		}
		if(validate_fip(magic + 0x10)) {
			texture_offset = 0x10;
		}
		
		if(texture_offset) {
			texture_impl::offsets offsets {
				*texture_offset + offsetof(fip_header, palette),
				*texture_offset + sizeof(fip_header),
				*texture_offset + offsetof(fip_header, width),
				*texture_offset + offsetof(fip_header, height)
			};
			_textures.emplace_back(
				std::make_unique<texture_impl>(file, offsets));
		}
	}
	
	log << "DONE!\n";
}

std::string racpak_fip_scanner::display_name() const {
	return _display_name;
}

std::vector<texture*> racpak_fip_scanner::textures() {
	std::vector<texture*> result(_textures.size());
	std::transform(_textures.begin(), _textures.end(), result.begin(),
		[](auto& ptr) { return ptr.get(); });
	return result;
}
