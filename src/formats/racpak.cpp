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

racpak::racpak(stream* backing)
	: _backing(backing) {}

uint32_t racpak::num_entries() {
	return _backing->peek<uint32_t>(0) / 8 - 1;
}

racpak_entry racpak::entry(uint32_t index) {
	_backing->seek((index + 1) * 8);
	return {
		_backing->read<uint32_t>() * 0x800,
		_backing->read<uint32_t>() * 0x800
	};
}

stream* racpak::open(racpak_entry entry) {
	_open_segments.emplace_back(
		std::make_unique<proxy_stream>(_backing, entry.offset, entry.size));
	return _open_segments.back().get();
}
	
bool racpak::is_compressed(racpak_entry entry) {
	char magic[3];
	_backing->peek_n(magic, entry.offset, 3);
	return validate_wad(magic);
}

stream* racpak::open_decompressed(racpak_entry entry) {
	stream* proxy = open(entry);
	_wad_segments.emplace_back(std::make_unique<wad_stream>(proxy, 0));
	return _wad_segments.back().get();
}
