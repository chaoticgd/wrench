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

#include "gameplay.h"

bool read_gameplay(Gameplay& gameplay, Buffer src) {
	for(const GameplayBlockDescription& block_desc : gameplay_blocks) {
		auto& pos = block_desc.rac4;
		s32 block_offset = src.read<s32>(pos.offset, "gameplay header");
		if(!block_desc.funcs.read(gameplay, src.subbuf(block_offset))) {
			return false;
		}
	}
	return true;
}
std::vector<u8> write_gameplay(const Gameplay& gameplay) { return {}; }

packed_struct(StringBlockHeader,
	s32 string_count;
	s32 size;
)

packed_struct(StringTableEntry,
	s32 offset;
	s32 id;
	s32 second_id;
	s16 unknown_c;
	s16 unknown_e;
)

struct StringBlock {
	static bool read(std::vector<std::vector<LevelString>>& dest, Buffer src) {
		auto& header = src.read<StringBlockHeader>(0, "string block header");
		auto table = src.read_multiple<StringTableEntry>(8, header.string_count, "string table");
		
		// TODO: For R&C3 and Deadlocked only.
		if(true) {
			src = src.subbuf(8);
		}
		
		std::vector<LevelString> strings;
		for(StringTableEntry entry : table) {
			LevelString string;
			string.id = entry.id;
			if(entry.offset == 0) {
				break;
			}
			string.string = src.read_string(entry.offset);
			strings.emplace_back(std::move(string));
		}
		
		dest.emplace_back(std::move(strings));
		return true;
	}
	
	static bool write(std::vector<u8>& dest, const std::vector<std::vector<LevelString>>& src) {
		return true;
	}
};

const s32 NONE = -1;

template <typename Block, typename Field>
static GameplayBlockFuncs bf(Field field) {
	GameplayBlockFuncs funcs;
	funcs.read = [field](Gameplay& gameplay, Buffer src) {
		return Block::read(gameplay.*field, src);
	};
	funcs.write = [field](std::vector<u8>& dest, const Gameplay& gameplay) {
		return Block::write(dest, gameplay.*field);
	};
	return funcs;
}

const std::vector<GameplayBlockDescription> gameplay_blocks = {
	//{{NONE, NONE}, {NONE, NONE}, {0x00, 0x01}, bf<StringBlock>(&Gameplay::strings), "Properties"},
	{{NONE, NONE}, {NONE, NONE}, {0x0c, 0x02}, bf<StringBlock>(&Gameplay::strings), "US english strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x10, 0x03}, bf<StringBlock>(&Gameplay::strings), "UK english strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x18, 0x04}, bf<StringBlock>(&Gameplay::strings), "french strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x1c, 0x05}, bf<StringBlock>(&Gameplay::strings), "german strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x20, 0x06}, bf<StringBlock>(&Gameplay::strings), "spanish strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x24, 0x07}, bf<StringBlock>(&Gameplay::strings), "italian strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x28, 0x08}, bf<StringBlock>(&Gameplay::strings), "japanese strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x2c, 0x09}, bf<StringBlock>(&Gameplay::strings), "korean strings"}
	//{{NONE, NONE}, {NONE, NONE}, {0x2c, 0x09}, bf<StringBlock>(&Gameplay::strings), "moby instances"}
};
