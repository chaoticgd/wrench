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

#ifndef FORMATS_ARMOR_ARCHIVE_H
#define FORMATS_ARMOR_ARCHIVE_H

#include <memory>
#include <string>
#include <vector>

#include "../stream.h"
#include "texture.h"
#include "game_model.h"

# /*
# 	Read ARMOR.WAD.
# */

packed_struct(armor_table_entry,
	Sector32 model;
	Sector32 model_size;
	Sector32 texture;
	Sector32 texture_size;
)

struct armor_archive {
	bool read(stream& file_);
	
	array_stream file;
	std::vector<moby_model> models;
	std::vector<texture> textures;
};

#endif
