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

static std::vector<u8> read_compressed_lump(FILE* file, SectorRange range, const char* name);
static std::map<s32, Chunk> read_chunks(FILE* file, SectorRange chunk_ranges[3], SectorRange chunk_bank_ranges[3]);
static std::map<s32, Mission> read_missions(FILE* file, SectorRange mission_ranges[128], SectorRange mission_bank_ranges[128]);

std::unique_ptr<Wad> read_wad(FILE* file) {
	s32 header_size;
	verify(fread(&header_size, 4, 1, file) == 1, "Failed to read WAD header.");
	switch(header_size) {
		case sizeof(Rac23LevelWadHeader): {
			auto header = read_header<Rac23LevelWadHeader>(file);
			LevelWad wad;
			wad.type = WadType::LEVEL;
			wad.level_number = header.level_number;
			s32 reverb = header.reverb;
			wad.reverb = reverb;
			wad.primary = read_lump(file, header.primary, "primary");
			wad.game = detect_game_rac23(wad.primary);
			wad.core_bank = read_lump(file, header.core_bank, "core bank");
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay, "gameplay");
			read_gameplay(wad.gameplay, gameplay_lump, wad.game, RAC23_GAMEPLAY_BLOCKS);
			wad.chunks = read_chunks(file, header.chunks, header.chunk_banks);
			return std::make_unique<LevelWad>(std::move(wad));
		}
		case sizeof(Rac23LevelWadHeader68): {
			auto header = read_header<Rac23LevelWadHeader68>(file);
			LevelWad wad;
			wad.game = Game::RAC2;
			wad.type = WadType::LEVEL;
			wad.level_number = header.level_number;
			s32 reverb = header.reverb;
			wad.reverb = reverb;
			wad.primary = read_lump(file, header.primary, "primary");
			wad.core_bank = read_lump(file, header.core_bank, "core bank");
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay_1, "gameplay");
			read_gameplay(wad.gameplay, gameplay_lump, wad.game, RAC23_GAMEPLAY_BLOCKS);
			wad.chunks = read_chunks(file, header.chunks, header.chunk_banks);
			return std::make_unique<LevelWad>(std::move(wad));
		}
		case sizeof(DeadlockedLevelWadHeader): {
			auto header = read_header<DeadlockedLevelWadHeader>(file);
			LevelWad wad;
			wad.game = Game::DL;
			wad.type = WadType::LEVEL;
			wad.level_number = header.level_number;
			s32 reverb = header.reverb;
			wad.reverb = reverb;
			wad.primary = read_lump(file, header.primary, "primary");
			wad.core_bank = read_lump(file, header.core_bank, "core bank");
			wad.chunks = read_chunks(file, header.chunks, header.chunk_banks);
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay_core, "gameplay core");
			read_gameplay(wad.gameplay, gameplay_lump, wad.game, DL_GAMEPLAY_CORE_BLOCKS);
			wad.missions = read_missions(file, header.gameplay_mission_data, header.mission_banks);
			std::vector<u8> art_instances_lump = read_compressed_lump(file, header.art_instances, "art instances");
			read_gameplay(wad.gameplay, art_instances_lump, wad.game, DL_ART_INSTANCE_BLOCKS);
			return std::make_unique<LevelWad>(std::move(wad));
		}
	}
	verify_not_reached("Failed to identify WAD.");
}

std::vector<u8> read_lump(FILE* file, SectorRange range, const char* name) {
	std::vector<u8> buffer(range.size.bytes());
	verify(fseek(file, range.offset.bytes(), SEEK_SET) == 0, "Failed to seek to %s lump.", name);
	if(buffer.size() > 0) {
		verify(fread(buffer.data(), buffer.size(), 1, file) == 1, "Failed to read %s lump.", name);
	}
	return buffer;
}

Game detect_game_rac23(std::vector<u8>& src) {
	for(size_t i = 0; i < src.size() - 0x10; i++) {
		if(memcmp((char*) &src[i], "IOPRP300.IMG", 12) == 0) {
			return Game::RAC3;
		}
		if(memcmp((char*) &src[i], "DNAS300.IMG", 11) == 0) {
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

static std::vector<u8> read_compressed_lump(FILE* file, SectorRange range, const char* name) {
	std::vector<u8> compressed_lump = read_lump(file, range, name);
	std::vector<u8> decompressed_lump;
	verify(decompress_wad(decompressed_lump, compressed_lump), "Failed to decompress %s lump.", name);
	return decompressed_lump;
}

static WadBuffer wad_buffer(Buffer buf) {
	return {buf.lo, buf.hi};
}

static std::map<s32, Chunk> read_chunks(FILE* file, SectorRange chunk_ranges[3], SectorRange chunk_bank_ranges[3]) {
	std::map<s32, Chunk> chunks;
	for(s32 i = 0; i < 3; i++) {
		if(chunk_ranges[i].size.sectors > 0 && chunk_bank_ranges[i].size.sectors > 0) {
			Chunk chunk;
			std::vector<u8> chunk_lump_vec = read_lump(file, chunk_ranges[i], "chunk");
			Buffer chunk_lump(chunk_lump_vec);
			auto& header = chunk_lump.read<ChunkHeader>(0, "chunk header");
			Buffer tfrag_buffer = chunk_lump.subbuf(header.tfrags);
			Buffer collision_buffer = chunk_lump.subbuf(header.collision);
			verify(decompress_wad(chunk.tfrags, wad_buffer(tfrag_buffer)), "Failed to decompress chunk tfrags.");
			verify(decompress_wad(chunk.collision, wad_buffer(collision_buffer)), "Failed to decompress chunk collision.");
			chunk.sound_bank = read_lump(file, chunk_bank_ranges[i], "chunk bank");
			chunks.emplace(i, std::move(chunk));
		}
	}
	return chunks;
}

static std::map<s32, Mission> read_missions(FILE* file, SectorRange mission_ranges[128], SectorRange mission_bank_ranges[128]) {
	std::map<s32, Mission> missions;
	for(s32 i = 0; i < 128; i++) {
		if(mission_ranges[i].size.sectors > 0 && mission_bank_ranges[i].size.sectors > 0) {
			Mission mission;
			std::vector<u8> mission_lump_vec = read_lump(file, mission_ranges[i], "mission lump");
			Buffer mission_lump(mission_lump_vec);
			auto& header = mission_lump.read<MissionHeader>(0, "mission header");
			if(header.instances.offset > 0) {
				Buffer instances_buffer = mission_lump.subbuf(header.instances.offset - mission_ranges[i].offset.bytes());
				verify(decompress_wad(mission.instances, wad_buffer(instances_buffer)), "Failed to decompress mission instances.");
			}
			if(header.classes.offset > 0) {
				Buffer classes_buffer = mission_lump.subbuf(header.classes.offset - mission_ranges[i].offset.bytes());
				verify(decompress_wad(mission.classes, wad_buffer(classes_buffer)), "Failed to decompress mission classes.");
			}
			mission.sound_bank = read_lump(file, mission_bank_ranges[i], "mission bank lump");
			missions.emplace(i, std::move(mission));
		}
	}
	return missions;
}

static std::vector<u8> build_level_wad(const LevelWad& wad);
static SectorRange write_lump(OutBuffer dest, const std::vector<u8>& buffer);
template <typename Header>
static void write_chunks(OutBuffer dest, Header& header, const std::map<s32, Chunk>& chunks);

void write_wad(FILE* file, const Wad* wad) {
	if(wad->type == WadType::LEVEL) {
		std::vector<u8> level = build_level_wad(*dynamic_cast<const LevelWad*>(wad));
		fwrite(level.data(), level.size(), 1, file);
	}
}

static std::vector<u8> build_level_wad(const LevelWad& wad) {
	std::vector<u8> dest_vec;
	OutBuffer dest(dest_vec);
	switch(wad.game) {
		case Game::RAC1: {
			verify_not_reached("R&C1 not supported.");
			break;
		}
		case Game::RAC2:
		case Game::RAC3: {
			Rac23LevelWadHeader header = {0};
			header.header_size = sizeof(Rac23LevelWadHeader);
			header.level_number = wad.level_number;
			header.reverb = *wad.reverb;
			dest.alloc<Rac23LevelWadHeader>();
			header.core_bank = write_lump(dest, wad.core_bank);
			header.primary = write_lump(dest, wad.primary);
			std::vector<u8> gameplay = write_gameplay(wad.gameplay, wad.game, RAC23_GAMEPLAY_BLOCKS);
			std::vector<u8> compressed_gameplay;
			compress_wad(compressed_gameplay, gameplay, 8);
			header.gameplay = write_lump(dest, compressed_gameplay);
			header.occlusion = write_lump(dest, write_occlusion(wad.gameplay, wad.game));
			write_chunks(dest, header, wad.chunks);
			dest.write(0, header);
			break;
		}
		case Game::DL: {
			break;
		}
	}
	return dest_vec;
}

static SectorRange write_lump(OutBuffer dest, const std::vector<u8>& buffer) {
	dest.pad(SECTOR_SIZE, 0);
	s64 offset_bytes = dest.tell();
	dest.write_multiple(buffer);
	s64 size_bytes = dest.tell() - offset_bytes;
	return {
		offset_bytes / SECTOR_SIZE,
		Sector32::size_from_bytes(size_bytes)
	};
}

template <typename Header>
static void write_chunks(OutBuffer dest, Header& header, const std::map<s32, Chunk>& chunks) {
	for(const auto& [index, chunk] : chunks) {
		assert(index >= 0 && index <= 2);
		std::vector<u8> chunk_vec;
		OutBuffer chunk_buffer(chunk_vec);
		s64 header_ofs = chunk_buffer.alloc<ChunkHeader>();
		ChunkHeader chunk_header;
		chunk_buffer.pad(0x10, 0);
		chunk_header.tfrags = chunk_buffer.tell();
		compress_wad(chunk_vec, chunk.tfrags, 8);
		chunk_buffer.pad(0x10, 0);
		chunk_header.collision = chunk_buffer.tell();
		compress_wad(chunk_vec, chunk.collision, 8);
		chunk_buffer.write(header_ofs, chunk_header);
		header.chunks[index] = write_lump(dest, chunk_vec);
	}
	for(const auto& [index, chunk] : chunks) {
		header.chunk_banks[index] = write_lump(dest, chunk.sound_bank);
	}
}
