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

#include "iso.h"

#include "level_stream.h"

iso_stream::iso_stream(std::string path)
	: file_stream(path, "Root") {}

void iso_stream::populate(app* a) {
	// For now just load G/LEVEL4.WAD from a hardcoded (PAL) offset.
	emplace_child<level_stream>(this, 0x8d794800, 0x17999dc);
	stream::populate(a);
}
