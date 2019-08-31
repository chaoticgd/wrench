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

#include <thread>

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
	std::stringstream hex;
	hex << _offsets.palette;
	return _backing.resource_path() + "+0x" + hex.str();
}

std::string texture_impl::pixel_data_path() const {
	std::stringstream hex;
	hex << _offsets.pixels;
	return _backing.resource_path() + "+0x" + hex.str();
}

texture_provider_impl::texture_provider_impl(
		stream* backing,
		std::size_t table_offset,
		std::size_t data_offset,
		std::size_t num_textures,
		std::string display_name_)
	: _display_name(display_name_) {

	backing->seek(table_offset);

	std::size_t last_palette = data_offset;

	for(std::size_t i = 0; i < num_textures; i++) {
		std::size_t entry_offset = backing->tell();
		auto entry = backing->read<fmt::texture_entry>();
		texture_impl::offsets offsets { 
			last_palette,
			data_offset + entry.pixel_data,
			entry_offset + offsetof(fmt::texture_entry, width),
			entry_offset + offsetof(fmt::texture_entry, height)
		};
		_textures.emplace_back(std::make_unique<texture_impl>(backing, offsets));
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
	stream* secondary_header,
	std::string display_name_) {

	auto snd_header = secondary_header->read<level_impl::fmt::secondary_header>(0);
	std::size_t pixel_data_offet = snd_header.tex_pixel_data_base;
	std::size_t textures_ptr = snd_header.textures.value;
	auto tex_header = secondary_header->read<fmt::header>(textures_ptr);

	_impl.emplace(
		secondary_header,
		textures_ptr + tex_header.textures.value,
		pixel_data_offet,
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
		racpak* archive,
		std::string display_name_,
		worker_logger& log)
	: _display_name(display_name_) {
	
	log << "Importing textures from " << display_name_ << " racpak... ";
	
	const static std::size_t NUM_THREADS = 8;
	std::map<std::size_t, std::unique_ptr<array_stream>> compressed_streams;
	std::array<std::vector<std::function<void()>>, NUM_THREADS> callbacks;
	int num_errors = 0;
	
	for(std::size_t i = 0; i < archive->num_entries(); i++) {
		auto entry = archive->entry(i);
		
		if(archive->is_compressed(entry)) {
			stream* file = archive->open(entry);
			file->seek(0);
			
			auto compressed_stream = std::make_unique<array_stream>();
			stream::copy_n(*compressed_stream.get(), *file, file->size());
			auto decompressed_stream = std::make_unique<array_stream>();
			
			auto dest = decompressed_stream.get();
			auto src = compressed_stream.get();
			callbacks[i % NUM_THREADS].emplace_back(
				[dest, src, &num_errors]() {
					try {
						decompress_wad(*dest, *src);
					} catch(stream_error) {
						*dest = array_stream();
						if(num_errors++ > 8) {
							// Linux systems tend to run out of memory
							// and freeze up if there are too many errors.
							throw stream_format_error("Too many errors!");
						}
					}
				});
			
			compressed_streams.emplace(i, std::move(compressed_stream));
			_decompressed_streams.emplace(i, std::move(decompressed_stream));
		}
	}
	
	std::array<std::thread, NUM_THREADS> thread_pool;
	
	for(std::size_t i = 0; i < thread_pool.size(); i++) {
		thread_pool[i] = std::thread(
			[](std::vector<std::function<void()>>& callbacks) {
				for(auto& callback : callbacks) {
					callback();
				}
			}, std::ref(callbacks[i]));
	}
	
	for(auto& thread : thread_pool) {
		thread.join();
	}
	
	for(std::size_t i = 0; i < archive->num_entries(); i++) {
		auto entry = archive->entry(i);
		
		bool has_decompressed =
			_decompressed_streams.find(i) != _decompressed_streams.end() &&
			_decompressed_streams.at(i).get() != nullptr &&
			_decompressed_streams.at(i)->size() > 0;
		
		stream* file;
		if(has_decompressed) {
			file = _decompressed_streams.at(i).get();
		} else if(!archive->is_compressed(entry)) {
			file = archive->open(entry);
		} else {
			log << "\tFailed to load entry " << i << "!\n";
			continue;
		}
		
		char magic[4];
		file->seek(0);
		file->read_n(magic, 4);
		if(validate_fip(magic)) {
			texture_impl::offsets offsets {
				offsetof(fip_header, palette),
				sizeof(fip_header),
				offsetof(fip_header, width),
				offsetof(fip_header, height)
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
