/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2024 chaoticgd

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

File read(Buffer src, const fs::path& path)
{
	File file;
	s64 pos = 0;
	
	file.path = path;
	file.type = identify(path.filename().string());
	
	switch (file.type) {
		case FileType::MAIN: {
			file.data = src.read_all<u8>().copy();
			break;
		}
		case FileType::NET: {
			file.blocks = read_blocks(&file.checksum_does_not_match, src, pos);
			break;
		}
		case FileType::PATCH: {
			file.data = src.read_all<u8>().copy();
			break;
		}
		case FileType::SLOT: {
			pos += sizeof(SaveSlotFileHeader);
			
			file.blocks = read_blocks(&file.checksum_does_not_match, src, pos);
			while (pos + 3 < src.size()) {
				file.levels.emplace_back(read_blocks(&file.checksum_does_not_match, src, pos));
			}
			break;
		}
		case FileType::SYS: {
			file.data = src.read_all<u8>().copy();
			break;
		}
	}
	
	return file;
}

FileType identify(std::string filename)
{
	for (char& c : filename) c = tolower(c);
	if (filename.find("ratchet") != std::string::npos) {
		return FileType::MAIN;
	} if (filename.starts_with("net")) {
		return FileType::NET;
	} else if (filename.starts_with("patch")) {
		return FileType::PATCH;
	} else if (filename.starts_with("save")) {
		return FileType::SLOT;
	} else if (filename.starts_with("icon")) {
		return FileType::SYS;
	} else {
		verify_not_reached("Unable to identify file type.");
	}
}

std::vector<Block> read_blocks(bool* checksum_does_not_match_out, Buffer src, s64& pos)
{
	std::vector<Block> blocks;
	
	const ChecksumHeader& checksum_header = src.read<ChecksumHeader>(pos, "checksum header");
	pos += sizeof(ChecksumHeader);
	
	u32 check_value = checksum(src.subbuf(pos, checksum_header.size));
	if (check_value != checksum_header.checksum) {
		*checksum_does_not_match_out |= true;
	}
	
	for (;;) {
		const SectionHeader& section_header = src.read<SectionHeader>(pos, "section header");
		pos += sizeof(SectionHeader);
		if (section_header.type == -1) {
			break;
		}
		
		// Preserve uninitialised padding.
		s32 read_size = align64(section_header.size, 4);
		
		Block& section = blocks.emplace_back();
		section.offset = (s32) pos;
		section.iff = section_header.type;
		section.unpadded_size = section_header.size;
		section.data = src.read_bytes(pos, read_size, "section data");
		pos += read_size;
	}
	
	return blocks;
}

void write(OutBuffer dest, File& file)
{
	switch (file.type) {
		case FileType::MAIN: {
			dest.write_multiple(file.data);
			break;
		}
		case FileType::NET: {
			write_blocks(dest, file.blocks);
			break;
		}
		case FileType::PATCH: {
			dest.write_multiple(file.data);
			break;
		}
		case FileType::SLOT: {
			s64 file_header_ofs = dest.alloc<SaveSlotFileHeader>();
			SaveSlotFileHeader file_header;
			file_header.game_data_size = (s32) write_blocks(dest, file.blocks);
			file_header.level_data_size = 0;
			for (std::vector<Block>& blocks : file.levels) {
				u32 data_size = (s32) write_blocks(dest, blocks);
				if (file_header.level_data_size == 0) {
					file_header.level_data_size = data_size;
				} else {
					verify_fatal(data_size == file_header.level_data_size);
				}
			}
			dest.write(file_header_ofs, file_header);
			break;
		}
		case FileType::SYS: {
			dest.write_multiple(file.data);
			break;
		}
	}
}

s64 write_blocks(OutBuffer dest, std::vector<Block>& blocks)
{
	s64 checksum_header_ofs = dest.alloc<ChecksumHeader>();
	s64 checksum_start_ofs = dest.tell();
	
	for (Block& section : blocks) {
		s64 size_difference = section.data.size() - section.unpadded_size;
		verify_fatal(size_difference > -1 && size_difference < 4);
		
		SectionHeader header;
		header.type = section.iff;
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

u32 checksum(Buffer src)
{
	u32 value = 0xedb88320;
	for (const u8* ptr = src.lo; ptr < src.hi; ptr++) {
		value ^= (u32) *ptr << 8;
		for (s32 repeat = 0; repeat < 8; repeat++) {
			if ((value & 0x8000) == 0) {
				value <<= 1;
			} else {
				value = (value << 1) ^ 0x1f45;
			}
		}
	}
	return value & 0xffff;
}

// *****************************************************************************

BlockSchema* FileSchema::block(s32 iff)
{
	for (BlockSchema& block : blocks) {
		if (block.iff == iff) {
			return &block;
		}
	}
	return nullptr;
}

BlockSchema* FileSchema::block(const std::string& name)
{
	for (BlockSchema& block : blocks) {
		if (block.name == name) {
			return &block;
		}
	}
	return nullptr;
}

GameSchema* Schema::game(Game g)
{
	switch (g) {
		case Game::RAC: return &rac;
		case Game::GC: return &gc;
		case Game::UYA: return &uya;
		case Game::DL: return &dl;
	}
	return nullptr;
}

#define VERIFY_SCHEMA(cond, ...) verify(cond, "Failed to parse memcard schema: " __VA_ARGS__)
#define VERIFY_NOT_REACHED_SCHEMA(...) verify_not_reached("Failed to parse memcard schema: " __VA_ARGS__)

Schema parse_schema(char* input)
{
	char* error;
	WtfNode* root = wtf_parse(input, &error);
	verify(root, "%s", error);
	defer([&]() { wtf_free(root); });
	
	Schema schema;
	
	for (const WtfNode* page = root->first_child; page != nullptr; page = page->next_sibling) {
		if (strcmp(page->type_name, "Page") != 0) {
			continue;
		}
		
		Page& page_schema = schema.pages.emplace_back();
		
		page_schema.tag = page->tag;
		
		const WtfAttribute* name = wtf_attribute(page, "name");
		VERIFY_SCHEMA(name && name->type == WTF_STRING, "Missing name attribute on page node '%s'.", page->tag);
		page_schema.name = name->string.begin;
	
		const WtfAttribute* layout = wtf_attribute(page, "layout");
		VERIFY_SCHEMA(layout && layout->type == WTF_STRING, "Missing layout attribute on page node '%s'.", page->tag);
		if (strcmp(layout->string.begin, "tree") == 0) {
			page_schema.layout = PageLayout::TREE;
		} else if (strcmp(layout->string.begin, "table") == 0) {
			page_schema.layout = PageLayout::TABLE;
		} else if (strcmp(layout->string.begin, "leveltable") == 0) {
			page_schema.layout = PageLayout::LEVEL_TABLE;
		} else if (strcmp(layout->string.begin, "datablocks") == 0) {
			page_schema.layout = PageLayout::DATA_BLOCKS;
		} else {
			VERIFY_NOT_REACHED_SCHEMA("Invalid layout attribute on page node '%s'.", page->tag);
		}
		
		const WtfAttribute* element_names = wtf_attribute(page, "element_names");
		if (element_names && element_names->type == WTF_STRING) {
			page_schema.element_names = element_names->string.begin;
		}
		
		const WtfAttribute* display_stored_totals = wtf_attribute(page, "display_stored_totals");
		if (display_stored_totals && display_stored_totals->type == WTF_BOOLEAN) {
			page_schema.display_stored_totals = display_stored_totals->boolean;
		}
		
		const WtfAttribute* display_calculated_int_totals = wtf_attribute(page, "display_calculated_int_totals");
		if (display_calculated_int_totals && display_calculated_int_totals->type == WTF_BOOLEAN) {
			page_schema.display_calculated_int_totals = display_calculated_int_totals->boolean;
		}
	}
	
	for (const WtfNode* game = root->first_child; game != nullptr; game = game->next_sibling) {
		if (strcmp(game->type_name, "Game") != 0) {
			continue;
		}
		
		Game g = game_from_string(game->tag);
		GameSchema* game_schema = schema.game(g);
		VERIFY_SCHEMA(game_schema, "Invalid game tag '%s'.", game->tag);
		
		for (const WtfNode* file = game->first_child; file != nullptr; file = file->next_sibling) {
			FileSchema* file_schema = nullptr;
			if (strcmp(file->tag, "net") == 0) {
				file_schema = &game_schema->net;
			} else if (strcmp(file->tag, "game") == 0) {
				file_schema = &game_schema->game;
			} else if (strcmp(file->tag, "level") == 0) {
				file_schema = &game_schema->level;
			}
			VERIFY_SCHEMA(file_schema, "Invalid file tag '%s'.", file->tag);
			
			for (const WtfNode* block = file->first_child; block != nullptr; block = block->next_sibling) {
				BlockSchema& block_schema = file_schema->blocks.emplace_back();
				
				block_schema.name = block->tag;
				
				const WtfAttribute* iff = wtf_attribute(block, "iff");
				VERIFY_SCHEMA(iff && iff->type == WTF_NUMBER, "Missing iff attribute on block node '%s'.", block->tag);
				block_schema.iff = iff->number.i;
				
				const WtfAttribute* page = wtf_attribute(block, "page");
				if (page && page->type == WTF_STRING) {
					block_schema.page = page->string.begin;
					
					bool valid_page = false;
					for (Page& page : schema.pages) {
						if (page.tag == block_schema.page) {
							valid_page = true;
						}
					}
					
					VERIFY_SCHEMA(valid_page, "Invalid page '%s'.", block_schema.page.c_str());
				}
				
				const WtfAttribute* buddy = wtf_attribute(block, "buddy");
				if (buddy && buddy->type == WTF_STRING) {
					block_schema.buddy = buddy->string.begin;
				}
			}
		}
	}
	
	return schema;
}

}
