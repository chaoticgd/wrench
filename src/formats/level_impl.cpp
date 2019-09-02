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

level_impl::level_impl(racpak* archive, std::string display_name, worker_logger& log)
	: _archive(archive) {
	
	log << "Importing level " << display_name << "...\n";

	if(archive->num_entries() < 4 || archive->num_entries() > 1024) {
		throw stream_format_error("Invalid number of entries in archive!");
	}

	_textures.emplace(archive->open(archive->entry(1)), display_name);
	log << "\tDetected " << _textures->textures().size() << " textures.\n";

	_moby_stream = archive->open_decompressed(archive->entry(3));
	auto segment_header = _moby_stream->read<fmt::moby_segment::header>(0);
	read_game_strings(segment_header, log);
	read_ties  (segment_header, log);
	read_shrubs(segment_header, log);
	read_mobies(segment_header, log);
}

texture_provider* level_impl::get_texture_provider() {
	return &_textures.value();
}

std::vector<tie*> level_impl::ties() {
	return unique_to_raw<tie>(_ties);
}

std::vector<shrub*> level_impl::shrubs() {
	return unique_to_raw<shrub>(_shrubs);
}

std::map<int32_t, moby*> level_impl::mobies() {
	std::map<int32_t, moby*> mobies_;
	for(auto& moby : _mobies) {
		mobies_.emplace(moby->uid(), moby.get());
	}
	return mobies_;
}

std::map<std::string, std::map<uint32_t, std::string>> level_impl::game_strings() {
	return _game_strings;
}

void level_impl::read_game_strings(fmt::moby_segment::header header, worker_logger& log) {
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
		auto table = _moby_stream->read<fmt::moby_segment::string_table_header>(lang_offset);
		std::map<uint32_t, std::string> strings;
		for(uint32_t i = 0; i < table.num_strings; i++) {
			_moby_stream->seek(
				lang_offset +
				sizeof(fmt::moby_segment::string_table_header) +
				sizeof(fmt::moby_segment::string_table_entry) * i);
			auto entry = _moby_stream->read<fmt::moby_segment::string_table_entry>();
			_moby_stream->seek(lang_offset + entry.string.value);
			strings[entry.id] =_moby_stream->read_string();
		}
		_game_strings[lang_name] = strings;
	}
}

void level_impl::read_ties(fmt::moby_segment::header header, worker_logger& log) {
	auto table = _moby_stream->read<fmt::moby_segment::obj_table_header>(header.ties.value);
	for(uint32_t i = 0; i < table.num_elements; i++) {
		_ties.emplace_back(std::make_unique<tie_impl>(
			_moby_stream,
			header.ties.value + sizeof(fmt::moby_segment::obj_table_header) + i * 0x60
		));
	}
	log << "\tDetected " << table.num_elements << " ties.\n";
}

void level_impl::read_shrubs(fmt::moby_segment::header header, worker_logger& log) {
	auto table = _moby_stream->read<fmt::moby_segment::obj_table_header>(header.shrubs.value);
	for(uint32_t i = 0; i < table.num_elements; i++) {
		_shrubs.emplace_back(std::make_unique<shrub_impl>(
			_moby_stream,
			header.shrubs.value + sizeof(fmt::moby_segment::obj_table_header) + i * 0x70
		));
	}
	log << "\tDetected " << table.num_elements << " shrubs.\n";
}

void level_impl::read_mobies(fmt::moby_segment::header header, worker_logger& log) {
	auto moby_header = _moby_stream->read<fmt::moby_segment::obj_table_header>(header.mobies.value);
	std::map<uint32_t, moby*> mobies_;
	for(uint32_t i = 0; i < moby_header.num_elements; i++) {
		_mobies.emplace_back(std::make_unique<moby_impl>(
			_moby_stream,
			header.mobies.value + sizeof(fmt::moby_segment::obj_table_header) + i * 0x88));
	}
	log << "\tDetected " << moby_header.num_elements << " mobies.\n";
}
