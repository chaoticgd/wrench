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

#ifndef SAVEEDITOR_MEMORY_CARD_H
#define SAVEEDITOR_MEMORY_CARD_H

#include <core/buffer.h>
#include <core/filesystem.h>
#include <core/build_config.h>

namespace memcard {

// *****************************************************************************
// Container format
// *****************************************************************************

struct Block
{
	s32 offset;
	s32 iff;
	s32 unpadded_size;
	std::vector<u8> data;
};

enum class FileType
{
	MAIN,
	NET,
	PATCH,
	SLOT,
	SYS
};

struct File
{
	fs::path path;
	bool checksum_does_not_match = false;
	FileType type;
	std::vector<Block> blocks; // NET, SLOT
	std::vector<std::vector<Block>> levels; // SLOT
	std::vector<u8> data; // MAIN, PATCH, SYS
};

File read(Buffer src, const fs::path& path);
FileType identify(std::string filename);
std::vector<Block> read_blocks(bool* checksum_does_not_match_out, Buffer src, s64& pos);
void write(OutBuffer dest, File& file);
s64 write_blocks(OutBuffer dest, std::vector<Block>& blocks);
u32 checksum(Buffer src);

// *****************************************************************************
// Schema format
// *****************************************************************************

enum class PageLayout
{
	TREE,
	TABLE,
	LEVEL_TABLE,
	DATA_BLOCKS
};

struct Page
{
	std::string tag;
	std::string name;
	PageLayout layout;
	std::string element_names;
	bool display_stored_totals = false;
	bool display_calculated_int_totals = false;
};

struct BlockSchema
{
	s32 iff;
	std::string name;
	std::string page;
	std::string buddy; // Links two blocks so they can be displayed in the same column.
};

struct FileSchema
{
	std::vector<BlockSchema> blocks;
	
	BlockSchema* block(s32 iff);
	BlockSchema* block(const std::string& name);
};

struct GameSchema
{
	FileSchema net;
	FileSchema game;
	FileSchema level;
};

struct Schema
{
	std::vector<Page> pages;
	GameSchema rac;
	GameSchema gc;
	GameSchema uya;
	GameSchema dl;
	
	GameSchema* game(Game g);
};

Schema parse_schema(char* input);

}

#endif
