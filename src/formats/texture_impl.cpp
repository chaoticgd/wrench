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

#include "texture_impl.h"

#include "../util.h"
#include "fip.h"
#include "level_impl.h"

std::vector<texture> enumerate_fip_textures(iso_stream* iso, racpak* archive) {	
	std::vector<texture> textures;
	
	for(std::size_t i = 0; i < archive->num_entries(); i++) {
		auto entry = archive->entry(i);
		stream* file;
		if(archive->is_compressed(entry)) {
			file = iso->get_decompressed(archive->base() + entry.offset);
		} else {
			file = archive->open(entry);
		}
		
		if(file == nullptr || file->size() < 0x14) {
			continue;
		}
		
		char magic[0x14];
		file->seek(0);
		file->read_n(magic, 0x14);
		
		std::optional<std::size_t> texture_offset;
		if(validate_fip(magic)) {
			texture_offset = 0;
		}
		if(validate_fip(magic + 0x10)) {
			texture_offset = 0x10;
		}
		
		if(texture_offset) {
			std::optional<texture> tex = create_fip_texture(file, *texture_offset);
			if(tex) {
				textures.emplace_back(*tex);
			} else {
				std::cerr << "Error: Failed to load 2FIP texture at "
				          << file->resource_path() << "\n";
			}
		}
	}
	
	return textures;
}
