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

#include "../buffer.h"
#include "../editor/util.h"

static std::size_t get_rac234_level_table_offset(Buffer src);

table_of_contents read_table_of_contents_rac1(FILE* file) {
	s32 magic, toc_size;
	fseek(file, RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE, SEEK_SET);
	verify(fread(&magic, 4, 1, file) == 1, "Failed to read R&C1 table of contents.");
	verify(fread(&toc_size, 4, 1, file) == 1, "Failed to read R&C1 table of contents.");
	verify(toc_size > 0 && toc_size < 1024 * 1024 * 1024, "Invalid R&C1 table of contents.");
	verify(magic == 1, "Invalid R&C1 table of contents.");
	std::vector<u8> bytes = read_file(file, RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE, toc_size);
	Buffer buffer(bytes);
	
	level_file_info rac1_level = LEVEL_FILE_TYPES.at(0x2434);
	
	table_of_contents toc;
	for(s64 ofs = 8; ofs < buffer.size(); ofs += 8) {
		Sector32 lsn = buffer.read<Sector32>(ofs, "sector");
		Sector32 size = buffer.read<Sector32>(ofs + 4, "size");
		if(lsn.sectors != 0) {
			std::vector<u8> header_bytes = read_file(file, lsn.bytes(), 0x2434);
			Buffer header(header_bytes);
			if(header.read<s32>(4, "file header") == 0x2434) {
				toc_level_part part;
				part.header_lba = {0};
				part.magic = 0x2434;
				part.file_lba = lsn;
				part.info = rac1_level;
				
				toc_level level;
				level.level_table_index = header.read<uint32_t>(0, "level number");
				level.parts[0] = part;
				toc.levels.emplace_back(std::move(level));
			}
		}
	}
	if(toc.levels.size() > 0) {
		for(size_t i = 0; i < toc.levels.size() - 1; i++) {
			toc.levels[i].parts[0]->file_size = {toc.levels[i + 1].parts[0]->file_lba.sectors - toc.levels[i].parts[0]->file_lba.sectors};
		}
		toc.levels.back().parts[0]->file_size = {file_size_in_bytes(file) / SECTOR_SIZE - toc.levels.back().parts[0]->file_lba.sectors};
	}
	return toc;
}

table_of_contents read_table_of_contents(FILE* file) {
	std::vector<u8> bytes = read_file(file, RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE, TOC_MAX_SIZE);
	Buffer buffer(bytes);
	
	table_of_contents toc;
	
	std::size_t level_table_offset = get_rac234_level_table_offset(buffer);
	if(level_table_offset == 0x0) {
		// We've failed to find the level table, at least try to find some of the other tables.
		level_table_offset = 0xffff;
	}
	
	std::size_t table_index = 0;
	s64 ofs = 0;
	while(ofs + 4 * 6 < level_table_offset) {
		toc_table table;
		table.index = table_index++;
		table.offset_in_toc = ofs;
		s32 header_size = buffer.read<s32>(ofs, "global header");
		if(header_size < 8 || header_size > 0xffff) {
			break;
		}
		table.sector = buffer.read<Sector32>(ofs + 4, "global header");
		table.header = buffer.read_multiple<u8>(ofs, header_size, "global header").copy();
		toc.tables.emplace_back(std::move(table));
		ofs += header_size;
	}
	
	// This fixes an off-by-one error with R&C3 where since the first entry of
	// the level table is supposed to be zeroed out, this code would otherwise
	// think that the level table starts 0x18 bytes later than it actually does.
	if(ofs + 0x18 == level_table_offset) {
		level_table_offset -= 0x18;
	}
	
	auto level_table = buffer.read_multiple<toc_level_table_entry>(level_table_offset, TOC_MAX_LEVELS, "level table");
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
			
			Sector32 sector = {part.header_lba.sectors - RAC234_TABLE_OF_CONTENTS_LBA};
			if(part.header_lba.sectors == 0) {
				continue;
			}
			if(sector.bytes() > buffer.size()) {
				break;
			}
			
			part.magic = buffer.read<uint32_t>(sector.bytes(), "level header size");
			part.file_lba = buffer.read<Sector32>(sector.bytes() + 4, "level sector number");
			
			auto info = LEVEL_FILE_TYPES.find(part.magic);
			if(info == LEVEL_FILE_TYPES.end()) {
				continue;
			}
			part.info = info->second;
			
			part.header = buffer.read_multiple<u8>(sector.bytes(), part.magic, "level header").copy();
			
			has_level_part |= part.info.type == level_file_type::LEVEL;
			level.parts[j] = part;
		}
		
		if(has_level_part) {
			toc.levels.push_back(level);
		}
	}
	
	return toc;
}

static std::size_t get_rac234_level_table_offset(Buffer src) {
	// Check that the two next entries are valid. This is necessary to
	// get past a false positive in Deadlocked.
	for(size_t i = 0; i < src.size() / 4 - 12; i++) {
		int parts = 0;
		for(int j = 0; j < 6; j++) {
			Sector32 lsn = src.read<Sector32>((i + j * 2) * 4, "table of contents");
			size_t header_offset = lsn.bytes() - RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE;
			if(lsn.sectors == 0 || header_offset > TOC_MAX_SIZE - 4) {
				break;
			}
			uint32_t header_size = src.read<uint32_t>(header_offset, "level header size");
			if(LEVEL_FILE_TYPES.find(header_size) != LEVEL_FILE_TYPES.end()) {
				parts++;
			}
		}
		if(parts == 6) {
			return i * 4;
		}
	}
	return 0;
}
