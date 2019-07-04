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

#include "level_stream.h"
#include "moby_stream.h"
#include "texture.h"
#include "../worker_thread.h"

level_stream::level_stream(stream* iso_file, uint32_t level_offset, uint32_t level_size)
	: proxy_stream(iso_file, level_offset, level_size, "Level") {}

void level_stream::populate(app* a) {
	stream::populate(a);

	auto master_header = read<fmt::master_header>();
	uint32_t moby_wad_offset = locate_moby_wad();
	uint32_t secondary_header_offset =
		locate_secondary_header(master_header, moby_wad_offset);

	_moby_segment_stream = std::make_unique<wad_stream>(this, moby_wad_offset);
	auto moby_segment_header = _moby_segment_stream->read<fmt::moby_segment_header>(0);

	stream* mp = emplace_child<moby_provider>
		(_moby_segment_stream.get(), moby_segment_header.mobies.value);
	stream* tp = emplace_child<texture_provider>
		(this, secondary_header_offset);

	mp->populate(a);
	tp->populate(a);

	if(auto view = a->get_3d_view()) {
		(*view)->reset_camera(*a);
	}
}

std::vector<const point_object*> level_stream::point_objects() const {
	std::vector<const point_object*> result;
	for(auto [uid, moby] : mobies()) {
		result.push_back(moby);
	}
	return result;
}

std::map<uint32_t, moby*> level_stream::mobies() {

	auto providers = children_of_type<moby_provider>();
	if(providers.size() < 1) {
		return {};
	}

	std::map<uint32_t, moby*> result;
	for(moby* current : providers[0]->children_of_type<moby>()) {
		result.emplace(current->uid(), current);
	}
	return result;
}

std::map<uint32_t, const moby*> level_stream::mobies() const {
	auto moby_map = const_cast<level_stream*>(this)->mobies();
	std::map<uint32_t, const moby*> result;
	for(auto& [uid, moby] : moby_map) {
		result.emplace(uid, moby);
	}
	return result;
}

uint32_t level_stream::locate_moby_wad() {
	
	// For now just find the largest 0x100 byte-aligned WAD segment.
	// This should work for most levels.

	uint32_t result_offset = 1;
	long result_size = -1;
	for(uint32_t offset = 0; offset < size() - sizeof(wad_header); offset += 0x100) {
		wad_header header = read<wad_header>(offset);
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

uint32_t level_stream::locate_secondary_header(fmt::master_header header, uint32_t moby_wad_offset) {
	uint32_t secondary_header_delta =
		(header.secondary_moby_offset_part * 0x800 + 0xfff) & 0xfffffffffffff000;
	return moby_wad_offset - secondary_header_delta;
}
