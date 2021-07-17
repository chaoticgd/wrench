/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef ISO_STREAM_H
#define ISO_STREAM_H

#include <nlohmann/json.hpp>

#include "stream.h"
#include "worker_logger.h"

# /*
#	Generates patches from a series of write_n calls made by the
#	rest of the program and loading patches from .wrench files.
# */

class simple_wad_stream : public array_stream {
public:
	simple_wad_stream(stream* backing, size_t offset);
};

std::string md5_from_stream(stream& st);

#endif
