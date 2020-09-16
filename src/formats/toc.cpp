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

#include "toc.h"

#include "../util.h"

table_of_contents read_toc(stream& iso, std::size_t toc_base) {
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
		if(table.header.size < sizeof(toc_table_header) || table.header.size > 0xffff) {
			break;
		}
		stream::copy_n(table.data, iso, table.header.size - sizeof(toc_table_header));
		toc.tables.emplace_back(std::move(table));
	}
	
	std::vector<toc_level_table_entry> level_table(TOC_MAX_LEVELS);
	iso.seek(toc_base + level_table_offset);
	iso.read_v(level_table);
	for(std::size_t i = 0; i < TOC_MAX_LEVELS; i++) {
		toc_level_table_entry entry = level_table[i];
		
		toc_level level;
		level.level_table_index = i;
		bool has_main_part = false;
		
		// The games have the fields in different orders, so we check the type
		// of what each field points to so we can support them all.
		sector32 headers[] = { entry.header_1, entry.header_2, entry.header_3 };
		sector32 sizes[] = { entry.header_1_size, entry.header_2_size, entry.header_3_size };
		for(std::size_t j = 0; j < sizeof(headers) / sizeof(sector32); j++) {
			if(headers[j].bytes() > iso.size()) {
				break;
			}
			
			uint32_t magic = iso.read<uint32_t>(headers[j].bytes());
			if(contains(TOC_MAIN_PART_MAGIC, magic)) {printf("main %ld\n", j);
				level.main_part = headers[j];
				level.main_part_size = sizes[j];
				has_main_part = true;
			}
			
			if(contains(TOC_AUDIO_PART_MAGIC, magic)) {printf("audio %ld\n", j);
				level.audio_part = headers[j];
				level.audio_part_size = sizes[j];
			}
			
			if(contains(TOC_SCENE_PART_MAGIC, magic)) {printf("scene %ld\n", j);
				level.scene_part = headers[j];
				level.scene_part_size = sizes[j];
			}
		}
		
		if(!has_main_part) {
			continue;
		}
		
		toc.levels.push_back(level);
	}
	
	return toc;
}

std::size_t toc_get_level_table_offset(stream& iso, std::size_t toc_base) {
	uint8_t buffer[TOC_MAX_SIZE];
	iso.seek(toc_base);
	iso.read_n((char*) buffer, sizeof(buffer));
	
	for(std::size_t i = 0; i < TOC_MAX_INDEX_SIZE - sizeof(toc_level_table_entry); i += sizeof(uint32_t)) {
		// Check that the two next entries are valid. This is necessary to
		// get past a false positive in Deadlocked.
		toc_level_table_entry entry1 = *(toc_level_table_entry*) &buffer[i];
		toc_level_table_entry entry2 = *(toc_level_table_entry*) &buffer[i + sizeof(toc_level_table_entry)];
		sector32 headers[] = {
			entry1.header_1, entry1.header_2, entry1.header_3,
			entry2.header_1, entry2.header_2, entry2.header_3
		};
		
		int parts = 0;
		for(sector32 header : headers) {
			if(header.sectors == 0) {
				break;
			}
			
			std::size_t magic_offset = header.bytes() - toc_base;
			
			if(magic_offset > TOC_MAX_SIZE - sizeof(uint32_t)) {
				break;
			}
			
			uint32_t magic = *(uint32_t*) &buffer[magic_offset];
			if(contains(TOC_MAIN_PART_MAGIC, magic) ||
			   contains(TOC_AUDIO_PART_MAGIC, magic) ||
			   contains(TOC_SCENE_PART_MAGIC, magic)) {
				parts++;
			}
		}
		
		if(parts == 6) {
			return i * sizeof(buffer[0]);
		}
	}
	return 0;
}

std::optional<level_file_header> level_read_file_header(stream* src, std::size_t offset) {
	level_file_header result;
	src->seek(offset);
	result.magic = src->peek<uint32_t>();
	switch(result.magic) {
		case 0x60: {
			auto file_header = src->read<level_file_header_60>();
			result.base_offset = file_header.base_offset.bytes();
			result.level_number = file_header.level_number;
			result.primary_header_offset = file_header.primary_header.bytes();
			result.moby_segment_offset = file_header.moby_segment.bytes();
			break;
		}
		case 0x68: {
			auto file_header = src->read<level_file_header_68>();
			result.base_offset = file_header.base_offset.bytes();
			result.level_number = file_header.level_number;
			result.primary_header_offset = file_header.primary_header.bytes();
			result.moby_segment_offset = file_header.moby_segment.bytes();
			break;
		}
		default: {
			return {};
		}
	}
	return result;
}
