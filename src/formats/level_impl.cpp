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

#include "level_impl.h"

level_impl::level_impl(stream* iso_file, uint32_t offset, uint32_t size, std::string display_name, worker_logger& log)
	: _level_file(iso_file, offset, size) {
	
	log << "Importing level " << display_name << "... ";

	auto master_header = _level_file.read<fmt::master_header>(0);
	uint32_t moby_wad_offset = locate_moby_wad();
	
	_moby_segment_stream.emplace(&_level_file, moby_wad_offset);

	uint32_t secondary_header_offset =
		locate_secondary_header(master_header, moby_wad_offset);

	_textures.emplace(&_level_file, secondary_header_offset, display_name);

	auto segment_header = _moby_segment_stream->read<fmt::moby_segment::header>(0);
	auto moby_header = _moby_segment_stream->read<fmt::moby_segment::moby_table>(segment_header.mobies.value);

	std::map<uint32_t, moby*> mobies_;
	for(uint32_t i = 0; i < moby_header.num_mobies; i++) {
		_mobies.emplace_back(std::make_unique<moby_impl>(&_moby_segment_stream.value(),
			segment_header.mobies.value + sizeof(fmt::moby_segment::moby_table) + i * 0x88));
	}

	read_game_strings(segment_header);

	log << "DONE!\n";
}

texture_provider* level_impl::get_texture_provider() {
	return &_textures.value();
}

std::map<uint32_t, moby*> level_impl::mobies() {
	std::map<uint32_t, moby*> mobies_;
	for(auto& moby : _mobies) {
		mobies_.emplace(moby->uid(), moby.get());
	}
	return mobies_;
}

std::map<std::string, std::map<uint32_t, std::string>> level_impl::game_strings() {
	return _game_strings;
}

void level_impl::read_game_strings(fmt::moby_segment::header header) {
	// Work around structure packing.
	auto english   = header.english_strings.value,
	     french    = header.french_strings.value,
		 german    = header.german_strings.value,
		 italian   = header.italian_strings.value,
		 null_lang = header.null_strings.value;
	const std::map<std::string, uint32_t> languages {
		{ "English", english   },
		{ "French",  french    },
		{ "German",  german    },
		{ "Italian", italian   },
		{ "Null",    null_lang }
	};

	for(auto& [lang_name, lang_offset] : languages) {
		auto table = _moby_segment_stream->read<fmt::moby_segment::string_table_header>(lang_offset);
		std::map<uint32_t, std::string> strings;
		for(uint32_t i = 0; i < table.num_strings; i++) {
			_moby_segment_stream->seek(
				lang_offset +
				sizeof(fmt::moby_segment::string_table_header) +
				sizeof(fmt::moby_segment::string_table_entry) * i);
			auto entry = _moby_segment_stream->read<fmt::moby_segment::string_table_entry>();
			_moby_segment_stream->seek(lang_offset + entry.string.value);
			strings[entry.id] =_moby_segment_stream->read_string();
		}
		_game_strings[lang_name] = strings;
	}
}

uint32_t level_impl::locate_moby_wad() {
	
	// For now just find the largest 0x100 byte-aligned WAD segment.
	// This should work for most levels.

	uint32_t result_offset = 1;
	long result_size = -1;
	for(uint32_t offset = 0; offset < _level_file.size() - sizeof(wad_header); offset += 0x100) {
		wad_header header = _level_file.read<wad_header>(offset);
		if(validate_wad(header.magic) && header.total_size > result_size) {
			result_offset = offset;
			result_size = header.total_size;
		}
	}

	if(result_offset == 1) {
		throw stream_format_error("File does not contain a valid WAD segment.");
	}
	
	return result_offset;
}

uint32_t level_impl::locate_secondary_header(const fmt::master_header& header, uint32_t moby_wad_offset) {
	uint32_t secondary_header_delta =
		(header.secondary_moby_offset_part * 0x800 + 0xfff) & 0xfffffffffffff000;
	return moby_wad_offset - secondary_header_delta;
}
