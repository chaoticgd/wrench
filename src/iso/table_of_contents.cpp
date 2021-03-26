/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "table_of_contents.h"

#include "../util.h"

table_of_contents read_table_of_contents(stream& iso, std::size_t toc_base) {
	table_of_contents toc;
	
	std::size_t level_table_offset = toc_get_level_table_offset(iso, toc_base);
	if(level_table_offset == 0x0) {
		// We've failed to find the level table, at least try to find some of the other tables.
		level_table_offset = 0xffff;
	}
	
	iso.seek(toc_base);
	std::size_t table_index = 0;
	while(iso.tell() + 4 * 6 < toc_base + level_table_offset) {
		toc_table table;
		table.index = table_index++;
		table.offset_in_toc = iso.tell() - toc_base;
		table.header = iso.read<toc_table_header>();
		if(table.header.header_size < sizeof(toc_table_header) || table.header.header_size > 0xffff) {
			break;
		}
		table.lumps.resize((table.header.header_size - sizeof(toc_table_header)) / sizeof(sector_range));
		iso.read_v(table.lumps);
		toc.tables.emplace_back(std::move(table));
	}
	
	std::vector<toc_level_table_entry> level_table(TOC_MAX_LEVELS);
	iso.seek(toc_base + level_table_offset);
	iso.read_v(level_table);
	for(size_t i = 0; i < TOC_MAX_LEVELS; i++) {
		toc_level_table_entry entry = level_table[i];
		
		toc_level level;
		level.level_table_index = i;
		bool has_level_part = false;
		
		// The games have the fields in different orders, so we check the type
		// of what each field points to so we can support them all.
		for(size_t j = 0; j < 3; j++) {
			toc_level_part part;
			part.header_lba = entry.parts[j].offset;
			part.file_size = entry.parts[j].size;
			
			if(part.header_lba.sectors == 0) {
				continue;
			}
			if(part.header_lba.bytes() > iso.size()) {
				break;
			}
			
			part.magic = iso.read<uint32_t>(part.header_lba.bytes());
			part.file_lba = iso.read<sector32>();
			
			auto info = LEVEL_FILE_TYPES.find(part.magic);
			if(info == LEVEL_FILE_TYPES.end()) {
				continue;
			}
			part.info = info->second;
			
			size_t header_size = part.info.header_size_sectors * SECTOR_SIZE;
			part.lumps.resize((header_size - 8) / sizeof(sector_range));
			iso.read_v(part.lumps);
			
			has_level_part |= part.info.type == level_file_type::LEVEL;
			level.parts[j] = part;
		}
		
		if(has_level_part) {
			toc.levels.push_back(level);
		}
	}
	
	return toc;
}

std::size_t toc_get_level_table_offset(stream& iso, std::size_t toc_base) {
	std::vector<uint32_t> buffer(TOC_MAX_SIZE / 4);
	iso.seek(toc_base);
	iso.read_v(buffer);
	
	// Check that the two next entries are valid. This is necessary to
	// get past a false positive in Deadlocked.
	for(size_t i = 0; i < buffer.size() - 6; i++) {
		int parts = 0;
		for(int j = 0; j < 6; j++) {
			sector32 lba = {buffer[i + j * 2]};
			size_t header_offset = lba.bytes() - toc_base;
			if(lba.sectors == 0 || header_offset > TOC_MAX_SIZE - 4) {
				break;
			}
			uint32_t magic = *(uint32_t*) &buffer[header_offset / 4];
			if(LEVEL_FILE_TYPES.find(magic) != LEVEL_FILE_TYPES.end()) {
				parts++;
			}
		}
		if(parts == 6) {
			return i * 4;
		}
	}
	return 0;
}
