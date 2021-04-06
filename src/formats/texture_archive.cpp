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

#include "texture_archive.h"

#include "fip.h"
#include "wad.h"

packed_struct(texture_table_entry,
	sector32 offset;
	sector32 size;
)

std::vector<texture> enumerate_pif_textures(stream& iso) {
	/*std::vector<texture> textures;
	
	std::size_t bad_textures = 0;
	
	// Prevent crashes when the table.data.size() % sizeof(texture_table_entry) != 0.
	std::size_t table_size = table.data.size() - sizeof(texture_table_entry) + 1;
	
	for(std::size_t off = 0; off < table_size; off += sizeof(texture_table_entry)) {
		auto entry = table.data.peek<texture_table_entry>(off);
		std::size_t abs_offset = table.header.base_offset.bytes() + entry.offset.bytes();
		
		if(abs_offset > iso.size()) {
			return {};
		}
		
		if(entry.offset.sectors == 0 || entry.size.sectors == 0 || entry.size.bytes() > 0x1000000) {
			continue;
		}
		
		array_stream raw;
		raw.buffer.resize(entry.size.bytes());
		iso.seek(abs_offset);
		iso.read_v(raw.buffer);
		raw.seek(0);
		
		array_stream uncompressed;
		if(memcmp(raw.buffer.data(), "WAD", 3) == 0) {
			// TODO: Handle compressed segments here.
			continue;
		} else {
			uncompressed = std::move(raw);
		}
		
		if(uncompressed.size() < 0x14) {
			bad_textures++;
			continue;
		}
		
		std::optional<std::size_t> texture_offset;
		if(validate_fip(uncompressed.buffer.data())) {
			texture_offset = 0;
		}
		if(validate_fip(uncompressed.buffer.data() + 0x10)) {
			texture_offset = 0x10;
		}
		
		if(texture_offset) {
			std::optional<texture> tex = create_fip_texture(&uncompressed, *texture_offset);
			if(tex) {
				textures.emplace_back(std::move(*tex));
			} else {
				bad_textures++;
				fprintf(stderr, "error: Failed to read PIF texture at %lx\n", abs_offset);
			}
		}
	}
	
	if(bad_textures > 10) {
		return {};
	}
	
	return textures;*/ return {};
}
