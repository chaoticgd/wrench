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

#include <wtf/wtf.h>

namespace memcard {

packed_struct(SaveSlotFileHeader,
	/* 0x0 */ s32 game_data_size;
	/* 0x4 */ s32 level_data_size;
)

packed_struct(ChecksumHeader,
	/* 0x0 */ s32 size;
	/* 0x4 */ s32 checksum;
)

packed_struct(SectionHeader,
	/* 0x0 */ s32 type;
	/* 0x4 */ s32 size;
)

File read(Buffer src, const fs::path& path) {
	File file;
	s64 pos = 0;
	
	file.path = path;
	file.type = identify(path.filename().string());
	
	switch(file.type) {
		case FileType::MAIN: {
			file.main.data = src.read_all<u8>().copy();
			break;
		}
		case FileType::NET: {
			file.net.blocks = read_blocks(&file.checksum_does_not_match, src, pos);
			break;
		}
		case FileType::PATCH: {
			file.patch.data = src.read_all<u8>().copy();
			break;
		}
		case FileType::SLOT: {
			pos += sizeof(SaveSlotFileHeader);
			
			file.slot.blocks = read_blocks(&file.checksum_does_not_match, src, pos);
			while(pos + 3 < src.size()) {
				file.slot.levels.emplace_back(read_blocks(&file.checksum_does_not_match, src, pos));
			}
			break;
		}
		case FileType::SYS: {
			file.sys.data = src.read_all<u8>().copy();
			break;
		}
	}
	
	return file;
}

FileType identify(std::string filename) {
	for(char& c : filename) c = tolower(c);
	if(filename.find("ratchet") != std::string::npos) {
		return FileType::MAIN;
	} if(filename.starts_with("net")) {
		return FileType::NET;
	} else if(filename.starts_with("patch")) {
		return FileType::PATCH;
	} else if(filename.starts_with("save")) {
		return FileType::SLOT;
	} else if(filename.starts_with("icon")) {
		return FileType::SYS;
	} else {
		verify_not_reached("Unable to identify file type.");
	}
}

std::vector<Block> read_blocks(bool* checksum_does_not_match_out, Buffer src, s64& pos) {
	std::vector<Block> blocks;
	
	const ChecksumHeader& checksum_header = src.read<ChecksumHeader>(pos, "checksum header");
	pos += sizeof(ChecksumHeader);
	
	u32 check_value = checksum(src.subbuf(pos, checksum_header.size));
	if(check_value != checksum_header.checksum) {
		*checksum_does_not_match_out |= true;
	}
	
	for(;;) {
		const SectionHeader& section_header = src.read<SectionHeader>(pos, "section header");
		pos += sizeof(SectionHeader);
		if(section_header.type == -1) {
			break;
		}
		
		// Preserve uninitialised padding.
		s32 read_size = align64(section_header.size, 4);
		
		Block& section = blocks.emplace_back();
		section.offset = (s32) pos;
		section.type = section_header.type;
		section.unpadded_size = section_header.size;
		section.data = src.read_bytes(pos, read_size, "section data");
		pos += read_size;
	}
	
	return blocks;
}

void write(OutBuffer dest, File& file) {
	switch(file.type) {
		case FileType::MAIN: {
			dest.write_multiple(file.main.data);
			break;
		}
		case FileType::NET: {
			write_blocks(dest, file.net.blocks);
			break;
		}
		case FileType::PATCH: {
			dest.write_multiple(file.patch.data);
			break;
		}
		case FileType::SLOT: {
			s64 file_header_ofs = dest.alloc<SaveSlotFileHeader>();
			SaveSlotFileHeader file_header;
			file_header.game_data_size = (s32) write_blocks(dest, file.slot.blocks);
			file_header.level_data_size = 0;
			for(std::vector<Block>& blocks : file.slot.levels) {
				u32 data_size = (s32) write_blocks(dest, blocks);
				if(file_header.level_data_size == 0) {
					file_header.level_data_size = data_size;
				} else {
					verify_fatal(data_size == file_header.level_data_size);
				}
			}
			dest.write(file_header_ofs, file_header);
			break;
		}
		case FileType::SYS: {
			dest.write_multiple(file.sys.data);
			break;
		}
	}
}

s64 write_blocks(OutBuffer dest, std::vector<Block>& blocks) {
	s64 checksum_header_ofs = dest.alloc<ChecksumHeader>();
	s64 checksum_start_ofs = dest.tell();
	
	for(Block& section : blocks) {
		s64 size_difference = section.data.size() - section.unpadded_size;
		verify_fatal(size_difference > -1 && size_difference < 4);
		
		SectionHeader header;
		header.type = section.type;
		header.size = section.unpadded_size;
		dest.write(header);
		section.offset = (s32) dest.tell();
		dest.write_multiple(section.data);
		dest.pad(4);
	}
	SectionHeader terminator;
	terminator.type = -1;
	terminator.size = 0;
	dest.write(terminator);
	
	s64 checksum_end_ofs = dest.tell();
	
	ChecksumHeader checksum_header;
	checksum_header.size = (s32) (checksum_end_ofs - checksum_start_ofs);
	checksum_header.checksum = checksum(Buffer(dest.vec).subbuf(checksum_start_ofs, checksum_header.size));
	dest.write(checksum_header_ofs, checksum_header);
	
	return dest.tell() - checksum_header_ofs;
}

u32 checksum(Buffer src) {
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

// *****************************************************************************

BlockSchema* FileSchema::block(s32 iff) {
	for(BlockSchema& block : blocks) {
		if(block.iff == iff) {
			return &block;
		}
	}
	return nullptr;
}

GameSchema* Schema::game(Game g) {
	switch(g) {
		case Game::RAC: return &rac;
		case Game::GC: return &gc;
		case Game::UYA: return &uya;
		case Game::DL: return &dl;
	}
	return nullptr;
}

Schema parse_schema(char* input) {
	char* error;
	WtfNode* root = wtf_parse(input, &error);
	verify(root, "%s", error);
	defer([&]() { wtf_free(root); });
	
	Schema schema;
	
	for(const WtfNode* game = root->first_child; game != nullptr; game = game->next_sibling) {
		Game g = game_from_string(game->tag);
		GameSchema* game_schema = schema.game(g);
		verify(game_schema, "Failed to parse memcard schema: Invalid game tag '%s'.", game->tag);
		
		for(const WtfNode* file = game->first_child; file != nullptr; file = file->next_sibling) {
			FileSchema* file_schema = nullptr;
			if(strcmp(file->tag, "net") == 0) {
				file_schema = &game_schema->net;
			} else if(strcmp(file->tag, "game") == 0) {
				file_schema = &game_schema->game;
			} else if(strcmp(file->tag, "level") == 0) {
				file_schema = &game_schema->level;
			}
			verify(file_schema, "Failed to parse memcard schema: Invalid file tag '%s'.", file->tag);
			
			for(const WtfNode* block = file->first_child; block != nullptr; block = block->next_sibling) {
				BlockSchema& block_schema = file_schema->blocks.emplace_back();
				
				block_schema.name = block->tag;
				
				const WtfAttribute* iff = wtf_attribute(block, "iff");
				verify(iff && iff->type == WTF_NUMBER, "Failed to parse memcard schema: Missing iff attribute on block node '%s'.", block->tag);
				block_schema.iff = iff->number.i;
				
				const WtfAttribute* type = wtf_attribute(block, "type");
				verify(type && type->type == WTF_STRING, "Failed to parse memcard schema: Missing iff attribute on block node '%s'.", block->tag);
				block_schema.type = type->string.begin;
			}
		}
	}
	
	return schema;
}

}
