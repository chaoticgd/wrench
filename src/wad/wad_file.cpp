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
		read_gameplay(dest, decompressed);
		return true;
	}
	
	static bool write(WadLumpDescription desc, std::vector<u8>& dest, const Gameplay& src) {
		std::vector<u8> uncompressed = write_gameplay(src);
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
	// e.g. if Field = LevelWad::binary_assets then ThisWad = LevelWad.
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
		{0x058, 1,   lt<GameplayLump>(&LevelWad::gameplay_core),  "gameplay_core"},
		{0x060, 128, lt<BinaryLump>(&LevelWad::binary_assets),    "gameplay_mission_instances"},
		{0x460, 128, lt<BinaryLump>(&LevelWad::binary_assets),    "gameplay_mission_data"},
		{0x860, 128, lt<SoundBankLump>(&LevelWad::binary_assets), "mission_banks"},
		{0xc60, 1,   lt<BinaryLump>(&LevelWad::binary_assets),    "art_instances"}
	}}
};
