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

bool game_world::is_selected(object_id id) const {
	return std::find(selection.begin(), selection.end(), id) != selection.end();
}

void game_world::read(stream* src) {
	auto header = src->read<fmt::header>(0);
	
	// Read game strings.
	auto read_language = [=](uint32_t offset) {
		std::vector<game_string> language;
	
		auto table = src->read<fmt::string_table_header>(offset);
		std::vector<fmt::string_table_entry> entries(table.num_strings);
		src->read_v(entries);
		
		for(fmt::string_table_entry& entry : entries) {
			src->seek(offset + entry.string.value);
			language.push_back({ entry.id, src->read_string() });
		}
		
		return language;
	};
	_languages["English"] = read_language(header.english_strings);
	_languages["French" ] = read_language(header.french_strings);
	_languages["German" ] = read_language(header.german_strings);
	_languages["Spanish"] = read_language(header.spanish_strings);
	_languages["Italian"] = read_language(header.italian_strings);
	
	// Read point objects.
	auto tie_table = src->read<fmt::object_table>(header.ties);
	_ties.resize(tie_table.num_elements);
	src->read_v(_ties);

	auto shrub_table = src->read<fmt::object_table>(header.shrubs);
	_shrubs.resize(shrub_table.num_elements);
	src->read_v(_shrubs);
	
	auto moby_table = src->read<fmt::object_table>(header.mobies);
	_mobies.resize(moby_table.num_elements);
	src->read_v(_mobies);
	
	// Read splines.
	auto spline_table = src->read<fmt::spline_table_header>(header.splines);
	for(std::size_t i = 0; i < spline_table.num_splines; i++) {
		uint32_t spline_offset = src->read<uint32_t>(header.splines + 0x10 + i * 4);
		uint32_t num_vertices = src->read<uint32_t>
			(header.splines + spline_table.data_offset + spline_offset);
		
		spline object;
		
		src->seek(src->tell() + 0xc);
		for(std::size_t i = 0; i < num_vertices; i++) {
			float x = src->read<float>();
			float y = src->read<float>();
			float z = src->read<float>();
			object.emplace_back(x, y, z);
			src->seek(src->tell() + 4);
		}
		
		_splines.push_back(object);
	}
}

void game_world::write() {
	// TODO
}

level::level(iso_stream* iso, std::size_t offset, std::size_t size, std::string display_name)
	: offset(offset),
	  _backing(iso, offset, size),
	  _textures(&_backing, display_name) {
	
	auto file_header = _backing.read<fmt::file_header>(0);
	auto primary_header = _backing.read<fmt::primary_header>(file_header.primary_header.bytes());

	_moby_stream = iso->get_decompressed(offset + file_header.moby_segment.bytes());
	world.read(_moby_stream);
}

texture_provider* level::get_texture_provider() {
	return &_textures;
}

stream* level::moby_stream() {
	return _moby_stream;
}
