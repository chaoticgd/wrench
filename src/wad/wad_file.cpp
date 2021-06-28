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
	static constexpr const char* extension = "bin";
	
	static bool read(WadLumpDescription desc, std::map<std::string, BinaryAsset>& dest, std::vector<u8>& src) {
		auto& asset = dest[desc.name];
		asset.is_array = desc.count != 1;
		asset.buffers.push_back(std::move(src));
		return true;
	}
	
	static bool write(WadLumpDescription desc, std::vector<u8>& dest, const std::map<std::string, BinaryAsset>& src) {
		//auto iter = src.find(desc.name);
		//verify(iter != src.end(), "Missing %s.", desc.name);
		//dest = iter->second.copy();
		return true;
	}
};

struct SoundBankLump : BinaryLump {
	static constexpr const char* extension = "bnk";
};

struct GameplayLump {
	static constexpr const char* extension = "json";
	
	static bool read(WadLumpDescription desc, Gameplay& dest, std::vector<u8>& src) {
		std::vector<u8> decompressed;
		if(!decompress_wad(decompressed, src)) {
			return false;
		}
		read_gameplay(dest, decompressed, GAMEPLAY_CORE_BLOCKS);
		return true;
	}
	
	static bool write(WadLumpDescription desc, std::vector<u8>& dest, const Gameplay& src) {
		std::vector<u8> uncompressed = write_gameplay(src, GAMEPLAY_CORE_BLOCKS);
		compress_wad(dest, uncompressed, 8);
		return true;
	}
};

struct ArtInstancesLump {
	static constexpr const char* extension = "json";
	
	static bool read(WadLumpDescription desc, Gameplay& dest, std::vector<u8>& src) {
		std::vector<u8> decompressed;
		if(!decompress_wad(decompressed, src)) {
			return false;
		}
		read_gameplay(dest, decompressed, ART_INSTANCE_BLOCKS);
		return true;
	}
	
	static bool write(WadLumpDescription desc, std::vector<u8>& dest, const Gameplay& src) {
		std::vector<u8> uncompressed = write_gameplay(src, ART_INSTANCE_BLOCKS);
		compress_wad(dest, uncompressed, 8);
		return true;
	}
};

template <typename ThisWad> 
static std::unique_ptr<Wad> create_wad() {
	return std::make_unique<ThisWad>();
}

template <typename Lump, typename Field>
static LumpType lt(Field field) {
	// e.g. if field = &LevelWad::binary_assets then ThisWad = LevelWad.
	using ThisWad = MemberTraits<Field>::instance_type;
	
	LumpType type;
	type.extension = Lump::extension;
	type.read = [field](WadLumpDescription desc, Wad& dest, std::vector<u8>& src) {
		ThisWad* this_wad = dynamic_cast<ThisWad*>(&dest);
		assert(this_wad);
		return Lump::read(desc, this_wad->*field, src);
	};
	type.write = [field](WadLumpDescription desc, std::vector<u8>& dest, const Wad& src) {
		const ThisWad* this_wad = dynamic_cast<const ThisWad*>(&src);
		assert(this_wad);
		return Lump::write(desc, dest, this_wad->*field);
	};
	return type;
}

const std::vector<WadFileDescription> wad_files = {
	{"level", 0xc68, &create_wad<LevelWad>, {
		{0x018, 1,   lt<BinaryLump>(&LevelWad::binary_assets),    "data"},
		{0x020, 1,   lt<BinaryLump>(&LevelWad::binary_assets),    "core_bank"},
		{0x028, 3,   lt<BinaryLump>(&LevelWad::binary_assets),    "chunk"},
		{0x040, 3,   lt<SoundBankLump>(&LevelWad::binary_assets), "chunkbank"},
		{0x058, 1,   lt<GameplayLump>(&LevelWad::gameplay),       "gameplay_core"},
		{0x060, 128, lt<BinaryLump>(&LevelWad::binary_assets),    "gameplay_mission_instances"},
		{0x460, 128, lt<BinaryLump>(&LevelWad::binary_assets),    "gameplay_mission_data"},
		{0x860, 128, lt<SoundBankLump>(&LevelWad::binary_assets), "mission_banks"},
		{0xc60, 1,   lt<ArtInstancesLump>(&LevelWad::gameplay),   "art_instances"}
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
