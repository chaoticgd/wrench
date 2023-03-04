/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include "memory_card.h"

namespace memory_card {

packed_struct(FileHeader,
	s32 game_data_size;
	s32 level_data_size;
)

packed_struct(ChecksumHeader,
	s32 size;
	s32 checksum;
)

packed_struct(SectionHeader,
	SectionType type;
	s32 size;
)

SaveGame read(Buffer src) {
	SaveGame save;
	s64 pos = 0;
	
	const FileHeader& file_header = src.read<FileHeader>(pos, "file header");
	pos += sizeof(FileHeader);
	
	save.game = read_sections(src, pos);
	while(pos + 3 < src.size()) {
		save.levels.emplace_back(read_sections(src, pos));
	}
	
	return save;
}

std::vector<Section> read_sections(Buffer src, s64& pos) {
	std::vector<Section> sections;
	
	const ChecksumHeader& checksum_header = src.read<ChecksumHeader>(pos, "checksum header");
	pos += sizeof(ChecksumHeader);
	
	u32 check_value = checksum(src.subbuf(pos, checksum_header.size));
	if(check_value != checksum_header.checksum) {
		fprintf(stderr, "warning: Save game checksum doesn't match!\n");
	}
	
	for(;;) {
		const SectionHeader& section_header = src.read<SectionHeader>(pos, "section header");
		pos += sizeof(SectionHeader);
		if(section_header.type == TERMINATOR) {
			break;
		}
		
		Section& section = sections.emplace_back();
		section.type = section_header.type;
		section.data = src.read_bytes(pos, section_header.size, "section data");
		pos = align64(pos + section_header.size, 4);
	}
	
	return sections;
}

void write(OutBuffer dest, const SaveGame& save) {
	
}

u32 checksum(Buffer src) {
	const u8* ptr = src.lo;
	u32 value = 0xedb88320;
	for(const u8* ptr = src.lo; ptr < src.hi; ptr++) {
		value ^= (u32) *ptr << 8;
		for(s32 repeat = 0; repeat < 8; repeat++) {
			if((value & 0x8000) == 0) {
				value <<= 1;
			} else {
				value = (value << 1) ^ 0x1f45;
			}
		}
	}
	return value & 0xffff;
}

}
