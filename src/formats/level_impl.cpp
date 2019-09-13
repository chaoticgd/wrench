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

level::level(iso_stream* iso, racpak* archive, std::string display_name, worker_logger& log)
	: _archive(archive) {
	
	log << "Importing level " << display_name << "...\n";

	if(archive->num_entries() < 4 || archive->num_entries() > 1024) {
		throw stream_format_error("Invalid number of entries in archive!");
	}

	_textures.emplace(archive->open(archive->entry(1)), display_name);
	log << "\tDetected " << _textures->textures().size() << " textures.\n";

	_moby_stream = iso->get_decompressed(archive->base() + archive->entry(3).offset);
	auto segment_header = _moby_stream->read<fmt::moby_segment::header>(0);
	read_game_strings(segment_header, log);
	read_ties  (segment_header, log);
	read_shrubs(segment_header, log);
	read_splines(segment_header, log);
	read_mobies(segment_header, log);
}

texture_provider* level::get_texture_provider() {
	return &_textures.value();
}

std::size_t level::num_ties() const {
	return _ties.size();
}

std::size_t level::num_shrubs() const {
	return _shrubs.size();
}

std::size_t level::num_splines() const {
	return _splines.size();
}

std::size_t level::num_mobies() const {
	return _mobies.size();
}
	
tie level::tie_at(std::size_t i) {
	return _ties[i];
}

shrub level::shrub_at(std::size_t i) {
	return _shrubs[i];
}

spline level::spline_at(std::size_t i) {
	return _splines[i];
}

moby level::moby_at(std::size_t i) {
	return _mobies[i];
}

const tie level::tie_at(std::size_t i) const {
	return const_cast<level*>(this)->tie_at(i);
}

const shrub level::shrub_at(std::size_t i) const {
	return const_cast<level*>(this)->shrub_at(i);
}

const spline level::spline_at(std::size_t i) const {
	return const_cast<level*>(this)->spline_at(i);
}

const moby level::moby_at(std::size_t i) const {
	return const_cast<level*>(this)->moby_at(i);
}

std::map<std::string, std::map<uint32_t, std::string>> level::game_strings() {
	return _game_strings;
}

stream* level::moby_stream() {
	return _moby_stream;
}

void level::read_game_strings(fmt::moby_segment::header header, worker_logger& log) {
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

void level::read_ties(fmt::moby_segment::header header, worker_logger& log) {
	auto table = _moby_stream->read<fmt::moby_segment::obj_table_header>(header.ties.value);
	for(uint32_t i = 0; i < table.num_elements; i++) {
		_ties.emplace_back(
			_moby_stream,
			header.ties.value + sizeof(fmt::moby_segment::obj_table_header) + i * 0x60
		);
	}
	log << "\tDetected " << table.num_elements << " ties.\n";
}

void level::read_shrubs(fmt::moby_segment::header header, worker_logger& log) {
	auto table = _moby_stream->read<fmt::moby_segment::obj_table_header>(header.shrubs.value);
	for(uint32_t i = 0; i < table.num_elements; i++) {
		_shrubs.emplace_back(
			_moby_stream,
			header.shrubs.value + sizeof(fmt::moby_segment::obj_table_header) + i * 0x70
		);
	}
	log << "\tDetected " << table.num_elements << " shrubs.\n";
}

void level::read_splines(fmt::moby_segment::header header, worker_logger& log) {
	
	packed_struct(spline_table_header,
		uint32_t num_splines;
		uint32_t data_offset;
		uint32_t unknown_08;
		uint32_t unknown_0c;
	)
	
	packed_struct(spline_entry,
		uint32_t num_vertices;
		uint32_t pad[3];
	)
	
	auto table = _moby_stream->read<spline_table_header>(header.splines);
	std::vector<uint32_t> spline_offsets;
	for(uint32_t i = 0; i < table.num_splines; i++) {
		spline_offsets.push_back(_moby_stream->read<uint32_t>());
	}
	
	for(uint32_t rel_offset : spline_offsets) {
		std::size_t offset = header.splines + table.data_offset + rel_offset;
		auto entry = _moby_stream->read<spline_entry>(offset);
		_splines.emplace_back(
			_moby_stream, offset + 0x10, entry.num_vertices * 0x10
		);
	}
}

void level::read_mobies(fmt::moby_segment::header header, worker_logger& log) {
	auto moby_header = _moby_stream->read<fmt::moby_segment::obj_table_header>(header.mobies.value);
	std::map<uint32_t, moby*> mobies_;
	for(uint32_t i = 0; i < moby_header.num_elements; i++) {
		_mobies.emplace_back(
			_moby_stream,
			header.mobies.value + sizeof(fmt::moby_segment::obj_table_header) + i * 0x88
		);
	}
	log << "\tDetected " << moby_header.num_elements << " mobies.\n";
}

spline::spline(stream* backing, std::size_t offset, std::size_t size)
	: _backing(backing, offset, size), _base(offset) {}

std::size_t spline::base() const {
	return _base;
}

std::vector<glm::vec3> spline::points() const {
	std::vector<glm::vec3> result;
	
	// We do not consider the position indicator part of the object's logical state.
	proxy_stream& backing = const_cast<proxy_stream&>(_backing);
	
	backing.seek(0);
	for(std::size_t i = 0; i < backing.size(); i += 0x10) {
		float x = backing.read<float>();
		float y = backing.read<float>();
		float z = backing.read<float>();
		result.emplace_back(x, y, z);
		backing.seek(backing.tell() + 4);
	}
	
	return result;
}
