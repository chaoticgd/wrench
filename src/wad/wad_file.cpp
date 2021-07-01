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

#include "wad_file.h"

struct BinaryLump {
	static void read(WadLumpDescription desc, std::map<std::string, BinaryAsset>& dest, std::vector<u8>& src, Game& game) {
		auto& asset = dest[desc.name];
		asset.is_array = desc.count != 1;
		asset.buffers.push_back(std::move(src));
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const std::map<std::string, BinaryAsset>& src, Game& game) {
		
	}
};

struct Rac23PrimaryLump {
	static void read(WadLumpDescription desc, std::map<std::string, BinaryAsset>& dest, std::vector<u8>& src, Game& game) {
		game = detect_game(src);
		BinaryLump::read(desc, dest, src, game);
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const std::map<std::string, BinaryAsset>& src, Game& game) {
		BinaryLump::write(desc, index, dest, src, game);
	}
	
	static Game detect_game(std::vector<u8>& src) {
		for(size_t i = 0; i < src.size() - 0x10; i++) {
			if(memcmp((char*) &src[i], "IOPRP300.IMG", 12) == 0) {
				return Game::RAC3;
			}
		}
		for(size_t i = 0; i < src.size() - 0x10; i++) {
			if(memcmp((char*) &src[i], "IOPRP255.IMG", 12) == 0) {
				return Game::RAC2;
			}
		}
		verify_not_reached("Unable to detect game!");
	}
};

struct DlPrimaryLump {
	static void read(WadLumpDescription desc, std::map<std::string, BinaryAsset>& dest, std::vector<u8>& src, Game& game) {
		game = Game::DL;
		BinaryLump::read(desc, dest, src, game);
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const std::map<std::string, BinaryAsset>& src, Game& game) {
		BinaryLump::write(desc, index, dest, src, game);
	}
};

struct Rac23GameplayLump {
	static void read(WadLumpDescription desc, Gameplay& dest, std::vector<u8>& src, Game& game) {
		assert(game == Game::RAC2 || game == Game::RAC3);
		std::vector<u8> decompressed;
		verify(decompress_wad(decompressed, src), "Failed to decompress gameplay lump.");
		read_gameplay(dest, decompressed, game, RAC23_GAMEPLAY_BLOCKS);
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const Gameplay& src, Game& game) {
		assert(game == Game::RAC2 || game == Game::RAC3);
		std::vector<u8> uncompressed = write_gameplay(src, game, RAC23_GAMEPLAY_BLOCKS);
		compress_wad(dest, uncompressed, 8);
	}
};

struct DlGameplayLump {
	static void read(WadLumpDescription desc, Gameplay& dest, std::vector<u8>& src, Game& game) {
		assert(game == Game::DL);
		std::vector<u8> decompressed;
		verify(decompress_wad(decompressed, src), "Failed to decompress gameplay lump.");
		read_gameplay(dest, decompressed, Game::DL, DL_GAMEPLAY_CORE_BLOCKS);
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const Gameplay& src, Game& game) {
		assert(game == Game::DL);
		std::vector<u8> uncompressed = write_gameplay(src, Game::DL, DL_GAMEPLAY_CORE_BLOCKS);
		compress_wad(dest, uncompressed, 8);
	}
};

struct ArtInstancesLump {
	static void read(WadLumpDescription desc, Gameplay& dest, std::vector<u8>& src, Game& game) {
		assert(game == Game::DL);
		std::vector<u8> decompressed;
		verify(decompress_wad(decompressed, src), "Failed to decompress art instances WAD.");
		read_gameplay(dest, decompressed, Game::DL, DL_ART_INSTANCE_BLOCKS);
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const Gameplay& src, Game& game) {
		assert(game == Game::DL);
		std::vector<u8> uncompressed = write_gameplay(src, Game::DL, DL_ART_INSTANCE_BLOCKS);
		compress_wad(dest, uncompressed, 8);
	}
};

struct GameplayMissionInstancesLump {
	static void read(WadLumpDescription desc, std::vector<Gameplay>& dest, std::vector<u8>& src, Game& game) {
		assert(game == Game::DL);
		Gameplay mission_instances;
		read_gameplay(mission_instances, src, Game::DL, DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS);
		dest.emplace_back(std::move(mission_instances));
	}
	
	static void write(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const std::vector<Gameplay>& src, Game& game) {
		assert(game == Game::DL);
		dest = write_gameplay(src.at(index), Game::DL, DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS);
	}
};

template <typename ThisWad> 
static std::unique_ptr<Wad> create_wad() {
	return std::make_unique<ThisWad>();
}

template <typename Lump, typename Field>
static LumpFuncs lf(Field field) {
	// e.g. if field = &LevelWad::binary_assets then ThisWad = LevelWad.
	using ThisWad = MemberTraits<Field>::instance_type;
	
	LumpFuncs funcs;
	funcs.read = [field](WadLumpDescription desc, Wad& dest, std::vector<u8>& src, Game& game) {
		ThisWad* this_wad = dynamic_cast<ThisWad*>(&dest);
		assert(this_wad);
		Lump::read(desc, this_wad->*field, src, game);
	};
	funcs.write = [field](WadLumpDescription desc, s32 index, std::vector<u8>& dest, const Wad& src, Game& game) {
		const ThisWad* this_wad = dynamic_cast<const ThisWad*>(&src);
		assert(this_wad);
		Lump::write(desc, index, dest, this_wad->*field, game);
	};
	return funcs;
}

const std::vector<WadFileDescription> wad_files = {
	{"level", 0x60, &create_wad<LevelWad>, {
		{0x10, 1, lf<Rac23PrimaryLump>(&LevelWad::binary_assets), "data"},
		{0x18, 1, lf<BinaryLump>(&LevelWad::binary_assets), "core_bank"},
		{0x20, 1, lf<Rac23GameplayLump>(&LevelWad::gameplay), "gameplay_core"},
		{0x28, 1, lf<BinaryLump>(&LevelWad::binary_assets), "unknown_28"},
		{0x30, 3, lf<BinaryLump>(&LevelWad::binary_assets), "chunk"},
		{0x48, 3, lf<BinaryLump>(&LevelWad::binary_assets), "chunkbank"}
	}},
	{"level", 0x68, &create_wad<LevelWad>, {
		{0x0c, 1, lf<Rac23PrimaryLump>(&LevelWad::binary_assets), "data"},
		{0x14, 1, lf<BinaryLump>(&LevelWad::binary_assets), "core_bank"},
		{0x1c, 1, lf<Rac23GameplayLump>(&LevelWad::gameplay), "gameplay_1"},
		{0x24, 1, lf<Rac23GameplayLump>(&LevelWad::gameplay), "gameplay_2"},
		{0x2c, 1, lf<BinaryLump>(&LevelWad::binary_assets), "unknown_2c"},
		{0x34, 3, lf<BinaryLump>(&LevelWad::binary_assets), "chunk"},
		{0x50, 3, lf<BinaryLump>(&LevelWad::binary_assets), "chunkbank"}
	}},
	{"level", 0xc68, &create_wad<LevelWad>, {
		{0x018, 1,   lf<DlPrimaryLump>(&LevelWad::binary_assets), "data"},
		{0x020, 1,   lf<BinaryLump>(&LevelWad::binary_assets), "core_bank"},
		{0x028, 3,   lf<BinaryLump>(&LevelWad::binary_assets), "chunk"},
		{0x040, 3,   lf<BinaryLump>(&LevelWad::binary_assets), "chunkbank"},
		{0x058, 1,   lf<DlGameplayLump>(&LevelWad::gameplay), "gameplay_core"},
		{0x060, 128, lf<GameplayMissionInstancesLump>(&LevelWad::gameplay_mission_instances), "gameplay_mission_instances"},
		{0x460, 128, lf<BinaryLump>(&LevelWad::binary_assets), "gameplay_mission_data"},
		{0x860, 128, lf<BinaryLump>(&LevelWad::binary_assets), "mission_banks"},
		{0xc60, 1,   lf<ArtInstancesLump>(&LevelWad::gameplay), "art_instances"}
	}}
};

std::vector<u8> read_header(FILE* file) {
	const char* ERR_READ_HEADER = "Failed to read header.";
	s32 header_size;
	verify(fseek(file, 0, SEEK_SET) == 0, ERR_READ_HEADER);
	verify(fread(&header_size, 4, 1, file) == 1, ERR_READ_HEADER);
	verify(header_size < 0x10000, "Invalid header.");
	std::vector<u8> header(header_size);
	verify(fseek(file, 0, SEEK_SET) == 0, ERR_READ_HEADER);
	verify(fread(header.data(), header.size(), 1, file) == 1, ERR_READ_HEADER);
	return header;
}

WadFileDescription match_wad(FILE* file, const std::vector<u8>& header) {
	for(const WadFileDescription& desc : wad_files) {
		if(desc.header_size == header.size()) {
			return desc;
		}
	}
	verify_not_reached("Unable to identify WAD file.");
}

std::vector<u8> read_lump(FILE* file, Sector32 offset, Sector32 size) {
	const char* ERR_READ_BLOCK = "Failed to read lump.";
	
	std::vector<u8> lump(size.bytes());
	verify(fseek(file, offset.bytes(), SEEK_SET) == 0, ERR_READ_BLOCK);
	verify(fread(lump.data(), lump.size(), 1, file) == 1, ERR_READ_BLOCK);
	return lump;
}

void write_file(const char* path, Buffer buffer) {
	FILE* file = fopen(path, "wb");
	verify(file, "Failed to open file '%s' for writing.", path);
	assert(buffer.size() > 0);
	verify(fwrite(buffer.lo, buffer.size(), 1, file) == 1, "Failed to write output file '%s'.", path);
	fclose(file);
	printf("Wrote %s (%ld KiB)\n", path, buffer.size() / 1024);
}
