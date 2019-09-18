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
}

texture_provider* level::get_texture_provider() {
	return &_textures.value();
}

std::size_t level::num_ties() const {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return _moby_stream->read<uint32_t>(seg_header.ties.value);
}

std::size_t level::num_shrubs() const {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return _moby_stream->read<uint32_t>(seg_header.shrubs.value);
}

std::size_t level::num_splines() const {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return _moby_stream->read<uint32_t>(seg_header.splines);
}

std::size_t level::num_mobies() const {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return _moby_stream->read<uint32_t>(seg_header.mobies.value);
}
	
tie level::tie_at(std::size_t i) {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return tie(
		_moby_stream, 
		seg_header.ties.value + 0x10 + i * 0x60
	);
}

shrub level::shrub_at(std::size_t i) {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return shrub(
		_moby_stream, 
		seg_header.shrubs.value + 0x10 + i * 0x70
	);
}

spline level::spline_at(std::size_t i) {
	
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
	
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	auto spline_table = _moby_stream->read<spline_table_header>(seg_header.splines);
	uint32_t entry_offset = _moby_stream->read<uint32_t>(seg_header.splines + 0x10 + i * 4);
	return spline(
		_moby_stream, 
		seg_header.splines + spline_table.data_offset + entry_offset
	);
}

moby level::moby_at(std::size_t i) {
	auto seg_header = _moby_stream->read<fmt::moby_segment::header>(0);
	return moby(
		_moby_stream, 
		seg_header.mobies.value + 0x10 + i * 0x88
	);
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

void level::for_each_game_object(std::function<void(game_object*)> callback) {
	for(std::size_t i = 0; i < num_ties(); i++) {
		tie object = tie_at(i);
		callback(&object);
	}
	for(std::size_t i = 0; i < num_shrubs(); i++) {
		shrub object = shrub_at(i);
		callback(&object);
	}
	for(std::size_t i = 0; i < num_splines(); i++) {
		spline object = spline_at(i);
		callback(&object);
	}
	for(std::size_t i = 0; i < num_mobies(); i++) {
		moby object = moby_at(i);
		callback(&object);
	}
}

void level::for_each_point_object(std::function<void(point_object*)> callback) {
	for(std::size_t i = 0; i < num_ties(); i++) {
		tie object = tie_at(i);
		callback(&object);
	}
	for(std::size_t i = 0; i < num_shrubs(); i++) {
		shrub object = shrub_at(i);
		callback(&object);
	}
	for(std::size_t i = 0; i < num_mobies(); i++) {
		moby object = moby_at(i);
		callback(&object);
	}
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

spline::spline(stream* backing, std::size_t offset)
	: _backing(backing, offset, 0), _base(offset) {}

std::size_t spline::base() const {
	return _base;
}

std::vector<glm::vec3> spline::points() const {
	std::vector<glm::vec3> result;
	
	// We do not consider the position indicator part of the object's logical state.
	proxy_stream& backing = const_cast<proxy_stream&>(_backing);
	
	uint32_t num_vertices = backing.read<uint32_t>(0);
	backing.seek(backing.tell() + 0xc);
	for(std::size_t i = 0; i < num_vertices; i++) {
		float x = backing.read<float>();
		float y = backing.read<float>();
		float z = backing.read<float>();
		result.emplace_back(x, y, z);
		backing.seek(backing.tell() + 4);
	}
	
	return result;
}

