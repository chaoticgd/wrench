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

#include "racpak.h"

#include "wad.h"

racpak::racpak(stream* backing, std::size_t offset, std::size_t size)
	: _backing(backing, offset, size),
	  _base(offset) {}

std::size_t racpak::num_entries() {
	uint32_t value = _backing.peek<uint32_t>(0);
	if(value < 8) {
		value = _backing.peek<uint32_t>(4);
	}
	return value / 8 - 1;
}

std::size_t racpak::base() {
	return _base;
}

racpak_entry racpak::entry(std::size_t index) {
	_backing.seek((index + 1) * 8);
	return {
		_backing.read<uint32_t>() * 0x800,
		_backing.read<uint32_t>() * 0x800
	};
}

stream* racpak::open(racpak_entry entry) {
	_open_segments.emplace_back(
		std::make_unique<proxy_stream>(&_backing, entry.offset, entry.size));
	return _open_segments.back().get();
}

bool racpak::is_compressed(racpak_entry entry) {
	char magic[3];
	_backing.seek(entry.offset);
	_backing.read_n(magic, 3);
	return validate_wad(magic);
}
