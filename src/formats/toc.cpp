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

static const std::size_t TOC_MAX_SIZE       = 0x100000;
static const std::size_t TOC_MAX_INDEX_SIZE = 0x10000;
static const std::size_t TOC_MAX_LEVELS     = 0x100;

std::size_t toc_get_level_table_offset(stream& iso, std::size_t toc_base);

table_of_contents read_toc(stream& iso, std::size_t toc_base) {
	table_of_contents toc;
	
	std::size_t level_table_offset = toc_get_level_table_offset(iso, toc_base);
	
	iso.seek(toc_base);
	while(iso.tell() + 4 * 6 < toc_base + level_table_offset) {
		toc_table table;
		table.offset_in_toc = iso.tell() - toc_base;
		table.header = iso.read<toc_table_header>();
		stream::copy_n(table.data, iso, table.header.size - sizeof(toc_table_header));
		toc.tables.push_back(table);
	}
	
	for(std::size_t i = 0; i < TOC_MAX_LEVELS; i++) {
		toc_level_table_entry entry = iso.read<toc_level_table_entry>
			(toc_base + level_table_offset + i * sizeof(toc_level_table_entry));
		
		toc_level level;
		bool has_main_part = false;
		bool has_audio_part = false;
		bool has_scene_part = false;
		
		// The games have the fields in different orders, so we check the type
		// of what each field points to so we can support them all.
		sector32 headers[] = { entry.header_1, entry.header_2, entry.header_3 };
		for(sector32 header : headers) {
			if(header.bytes() > iso.size()) {
				break;
			}
			uint32_t magic = iso.read<uint32_t>(header.bytes());
			if(magic == 0x60 || magic == 0x68) {
				level.main_part = *level_read_file_header(&iso, header.bytes());
				has_main_part = true;
			}
			
			if(magic == 0x1018 || magic == 0x1818) {
				level.audio_part = iso.read<sector32>(header.bytes() + 4);
				has_audio_part = true;
			}
			
			if(magic == 0x137c || magic == 0x26f0) {
				level.scene_part = iso.read<sector32>(header.bytes() + 4);
				has_scene_part = true;
			}
		}
		
		if(!has_main_part || !has_audio_part) {
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
	
	for(std::size_t i = 0; i < TOC_MAX_INDEX_SIZE; i += sizeof(uint32_t)) {
		toc_level_table_entry entry = *(toc_level_table_entry*) &buffer[i];
		sector32 headers[] = { entry.header_1, entry.header_2, entry.header_3 };
		
		static const uint32_t valid_magic_bytes[] = {
			0x60, 0x68, 0x1018, 0x1818, 0x137c, 0x26f0
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
			for(uint32_t expected : valid_magic_bytes) {
				if(magic == expected) {
					parts++;
				}
			}
		}
		
		if(parts == 3) {
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
