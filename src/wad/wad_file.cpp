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

#include "collision.h"

static WadBuffer wad_buffer(Buffer buf) {
	return {buf.lo, buf.hi};
}

packed_struct(MobyClassEntry,
	s32 offset_in_asset_wad;
	s32 o_class;
	s32 unknown_8;
	s32 unknown_c;
	u8 textures[16];
)

packed_struct(TieClassEntry,
	s32 offset_in_asset_wad;
	s32 o_class;
	s32 unknown_8;
	s32 unknown_c;
	u8 textures[16];
)

packed_struct(ShrubClassEntry,
	uint32_t offset_in_asset_wad;
	uint32_t o_class;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint8_t textures[16];
	uint8_t unknown_20[16];
)

struct Assets {
	static void read(LevelWad& wad, Buffer asset_header, Buffer assets) {
		auto header = asset_header.read<DeadlockedAssetHeader>(0, "asset header");
		
		s32 tfrags_size;
		if(header.occlusion != 0) {
			tfrags_size = header.occlusion;
		} else if(header.sky != 0) {
			tfrags_size = header.sky;
		} else if(header.collision != 0) {
			tfrags_size = header.collision;
		} else {
			verify_not_reached("Unable to determine size of tfrag block.");
		}
		wad.tfrags = assets.read_bytes(header.tfrags, tfrags_size, "tfrags");
		s32 occlusion_size = next_block(header.occlusion, header) - header.occlusion;
		wad.occlusion = assets.read_bytes(header.occlusion, occlusion_size, "occlusion");
		s32 sky_size = next_block(header.sky, header) - header.sky;
		wad.sky = assets.read_bytes(header.sky, sky_size, "sky");
		s32 collision_size = next_block(header.collision, header) - header.collision;
		std::vector<u8> collision = assets.read_bytes(header.collision, collision_size, "collision");
		wad.collision = read_collision(Buffer(collision));
		
		verify(header.moby_classes.count >= 1, "Level has no moby classes.");
		verify(header.tie_classes.count >= 1, "Level has no tie classes.");
		verify(header.shrub_classes.count >= 1, "Level has no shrub classes.");
		
		auto moby_classes = asset_header.read_multiple<MobyClassEntry>(header.moby_classes.offset, header.moby_classes.count, "moby class table");
		auto tie_classes = asset_header.read_multiple<TieClassEntry>(header.tie_classes.offset, header.tie_classes.count, "tie class table");
		auto shrub_classes = asset_header.read_multiple<ShrubClassEntry>(header.shrub_classes.offset, header.shrub_classes.count, "shrub class table");
		
		s32 textures_size = moby_classes[0].offset_in_asset_wad - header.textures_base_offset;
		wad.shared_textures = assets.read_bytes(header.textures_base_offset, textures_size, "textures");
		
		s32 mobies_size = tie_classes[0].offset_in_asset_wad - moby_classes[0].offset_in_asset_wad;
		wad.mobies = assets.read_bytes(moby_classes[0].offset_in_asset_wad, mobies_size, "moby classes");
		
		s32 ties_size = shrub_classes[0].offset_in_asset_wad - tie_classes[0].offset_in_asset_wad;
		wad.ties = assets.read_bytes(tie_classes[0].offset_in_asset_wad, ties_size, "tie classes");
		
		s32 shrubs_size = header.assets_decompressed_size - shrub_classes[0].offset_in_asset_wad;
		wad.shrubs = assets.read_bytes(shrub_classes[0].offset_in_asset_wad, shrubs_size, "shrub classes");
	}
	
	static SectorRange write(OutBuffer& dest, const LevelWad& wad) {
		
	}
	
	static s32 next_block(s32 ofs, const DeadlockedAssetHeader& header) {
		if(ofs == 0) {
			return 0;
		}
		
		s32 offsets[] = {
			header.tfrags,
			header.occlusion,
			header.sky,
			header.collision,
			header.textures_base_offset
		};
		s32 value = -1;
		for(s32 compare : offsets) {
			if((value == -1 || compare < value) && compare > ofs) {
				value = compare;
			}
		}
		return value;
	}
};

struct PrimaryLump {
	static void read(LevelWad& wad, Buffer src) {
		std::vector<u8> header_bytes = src.read_bytes(0, max_header_size(), "primary header");
		PrimaryHeader header = {0};
		swap_header(header, header_bytes, wad.game);
		
		wad.code = src.read_bytes(header.code.offset, header.code.size, "code");
		wad.asset_header = src.read_bytes(header.asset_header.offset, header.asset_header.size, "asset_header");
		wad.small_textures = src.read_bytes(header.small_textures.offset, header.small_textures.size, "small_textures");
		wad.hud_header = src.read_bytes(header.hud_header.offset, header.hud_header.size, "hud_header");
		for(s32 i = 0; i < 5; i++) {
			if(header.hud_banks[i].offset > 0) {
				wad.hud_banks[i] = src.read_bytes(header.hud_banks[i].offset, header.hud_banks[i].size, "hud_banks");
			}
		}
		std::vector<u8> assets_vec;
		verify(decompress_wad(assets_vec, wad_buffer(src.subbuf(header.assets.offset))), "Failed to decompress assets.");
		Buffer assets(assets_vec);
		Assets::read(wad, wad.asset_header, assets);
		if(header.moby8355_pvars.has_value()) {
			wad.moby8355_pvars = src.read_bytes(header.moby8355_pvars->offset, header.moby8355_pvars->size, "moby8355_pvars");
		}
		if(header.global_nav_data.has_value()) {
			wad.global_nav_data = src.read_bytes(header.global_nav_data->offset, header.global_nav_data->size, "global_nav_data");
		}
	}
	
	static SectorRange write(OutBuffer& dest, const LevelWad& wad) {
		return {0, 0};
	}
	
	static void swap_header(PrimaryHeader& l, std::vector<u8>& r, Game game) {
		switch(game) {
			case Game::RAC1:
			case Game::RAC2:
			case Game::RAC3: {
				Rac123PrimaryHeader packed_header = {0};
				if(r.size() >= sizeof(Rac123PrimaryHeader)) {
					packed_header = Buffer(r).read<Rac123PrimaryHeader>(0, "primary header");
				}
				l.moby8355_pvars = {};
				SWAP_PACKED(l.code, packed_header.code);
				SWAP_PACKED(l.asset_header, packed_header.asset_header);
				SWAP_PACKED(l.small_textures, packed_header.small_textures);
				SWAP_PACKED(l.hud_header, packed_header.hud_header);
				for(s32 i = 0; i < 5; i++) {
					SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
				}
				SWAP_PACKED(l.assets, packed_header.assets);
				l.art_instances = {};
				l.gameplay_core = {};
				l.global_nav_data = {};
				if(r.size() == 0) {
					OutBuffer(r).write(packed_header);
				}
				break;
			}
			case Game::DL: {
				DeadlockedPrimaryHeader packed_header = {0};
				if(r.size() >= sizeof(DeadlockedPrimaryHeader)) {
					packed_header = Buffer(r).read<DeadlockedPrimaryHeader>(0, "primary header");
					l.moby8355_pvars = ByteRange {0, 0};
					l.art_instances = ByteRange {0, 0};
					l.gameplay_core = ByteRange {0, 0};
					l.global_nav_data = ByteRange {0, 0};
				}
				SWAP_PACKED(*l.moby8355_pvars, packed_header.moby8355_pvars);
				SWAP_PACKED(l.code, packed_header.code);
				SWAP_PACKED(l.asset_header, packed_header.asset_header);
				SWAP_PACKED(l.small_textures, packed_header.small_textures);
				SWAP_PACKED(l.hud_header, packed_header.hud_header);
				for(s32 i = 0; i < 5; i++) {
					SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
				}
				SWAP_PACKED(l.assets, packed_header.assets);
				SWAP_PACKED(*l.moby8355_pvars, packed_header.moby8355_pvars);
				SWAP_PACKED(*l.art_instances, packed_header.art_instances);
				SWAP_PACKED(*l.gameplay_core, packed_header.gameplay_core);
				SWAP_PACKED(*l.global_nav_data, packed_header.global_nav_data);
				if(r.size() == 0) {
					OutBuffer(r).write(packed_header);
				}
				break;
			}
		}
	}
	
	static s64 max_header_size() {
		return std::max(sizeof(Rac123PrimaryHeader), sizeof(DeadlockedPrimaryHeader));
	}
};

static std::vector<u8> read_compressed_lump(FILE* file, SectorRange range, const char* name);
static std::map<s32, Chunk> read_chunks(FILE* file, SectorRange chunk_ranges[3], SectorRange chunk_bank_ranges[3]);
static std::map<s32, Mission> read_missions(FILE* file, SectorRange mission_ranges[128], SectorRange mission_bank_ranges[128]);

std::unique_ptr<Wad> read_wad(FILE* file) {
	s32 header_size;
	verify(fread(&header_size, 4, 1, file) == 1, "Failed to read WAD header.");
	switch(header_size) {
		case sizeof(Rac1LevelWadHeader): {
			auto header = read_header<Rac1LevelWadHeader>(file);
			LevelWad wad;
			wad.game = Game::RAC1;
			wad.type = WadType::LEVEL;
			wad.level_number = header.level_number;
			std::vector<u8> primary_lump = read_lump(file, header.primary, "primary");
			PrimaryLump::read(wad, Buffer(primary_lump));
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay_ntsc, "gameplay NTSC");
			read_gameplay(wad, wad.gameplay, gameplay_lump, wad.game, RAC1_GAMEPLAY_BLOCKS);
			return std::make_unique<LevelWad>(std::move(wad));
		}
		case sizeof(Rac23LevelWadHeader): {
			auto header = read_header<Rac23LevelWadHeader>(file);
			LevelWad wad;
			wad.type = WadType::LEVEL;
			wad.level_number = header.level_number;
			s32 reverb = header.reverb;
			wad.reverb = reverb;
			std::vector<u8> primary_lump = read_lump(file, header.primary, "primary");
			wad.game = detect_game_rac23(primary_lump);
			PrimaryLump::read(wad, Buffer(primary_lump));
			wad.core_bank = read_lump(file, header.core_bank, "core bank");
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay, "gameplay");
			read_gameplay(wad, wad.gameplay, gameplay_lump, wad.game, RAC23_GAMEPLAY_BLOCKS);
			wad.help_messages.swap(wad.gameplay); // gameplay -> help_messages
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
			std::vector<u8> primary_lump = read_lump(file, header.primary, "primary");
			PrimaryLump::read(wad, Buffer(primary_lump));
			wad.core_bank = read_lump(file, header.core_bank, "core bank");
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay_1, "gameplay");
			read_gameplay(wad, wad.gameplay, gameplay_lump, wad.game, RAC23_GAMEPLAY_BLOCKS);
			wad.help_messages.swap(wad.gameplay); // gameplay -> help_messages
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
			std::vector<u8> primary_lump = read_lump(file, header.primary, "primary");
			PrimaryLump::read(wad, Buffer(primary_lump));
			wad.core_bank = read_lump(file, header.core_bank, "core bank");
			wad.chunks = read_chunks(file, header.chunks, header.chunk_banks);
			std::vector<u8> gameplay_lump = read_compressed_lump(file, header.gameplay_core, "gameplay core");
			read_gameplay(wad, wad.gameplay, gameplay_lump, wad.game, DL_GAMEPLAY_CORE_BLOCKS);
			wad.help_messages.swap(wad.gameplay); // gameplay -> help_messages
			wad.missions = read_missions(file, header.gameplay_mission_data, header.mission_banks);
			std::vector<u8> art_instances_lump = read_compressed_lump(file, header.art_instances, "art instances");
			read_gameplay(wad, wad.gameplay, art_instances_lump, wad.game, DL_ART_INSTANCE_BLOCKS);
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

static std::map<s32, Chunk> read_chunks(FILE* file, SectorRange chunk_ranges[3], SectorRange chunk_bank_ranges[3]) {
	std::map<s32, Chunk> chunks;
	for(s32 i = 0; i < 3; i++) {
		Chunk chunk;
		bool is_chunky = false;
		if(chunk_ranges[i].size.sectors > 0) {
			std::vector<u8> chunk_lump_vec = read_lump(file, chunk_ranges[i], "chunk");
			Buffer chunk_lump(chunk_lump_vec);
			auto& header = chunk_lump.read<ChunkHeader>(0, "chunk header");
			Buffer tfrag_buffer = chunk_lump.subbuf(header.tfrags);
			Buffer collision_buffer = chunk_lump.subbuf(header.collision);
			chunk.tfrags = std::vector<u8>();
			chunk.collision = std::vector<u8>();
			verify(decompress_wad(*chunk.tfrags, wad_buffer(tfrag_buffer)), "Failed to decompress chunk tfrags.");
			verify(decompress_wad(*chunk.collision, wad_buffer(collision_buffer)), "Failed to decompress chunk collision.");
			is_chunky = true;
		}
		if(chunk_bank_ranges[i].size.sectors > 0) {
			chunk.sound_bank = read_lump(file, chunk_bank_ranges[i], "chunk bank");
			is_chunky = true;
		}
		if(is_chunky) {
			chunks.emplace(i, std::move(chunk));
		}
	}
	return chunks;
}

static std::map<s32, Mission> read_missions(FILE* file, SectorRange mission_ranges[128], SectorRange mission_bank_ranges[128]) {
	std::map<s32, Mission> missions;
	for(s32 i = 0; i < 128; i++) {
		Mission mission;
		bool is_mission = false;
		if(mission_ranges[i].size.sectors > 0) {
			std::vector<u8> mission_lump_vec = read_lump(file, mission_ranges[i], "mission lump");
			Buffer mission_lump(mission_lump_vec);
			auto& header = mission_lump.read<MissionHeader>(0, "mission header");
			if(header.instances.offset > 0) {
				Buffer instances_buffer = mission_lump.subbuf(header.instances.offset - mission_ranges[i].offset.bytes());
				mission.instances = std::vector<u8>();
				verify(decompress_wad(*mission.instances, wad_buffer(instances_buffer)), "Failed to decompress mission instances.");
			}
			if(header.classes.offset > 0) {
				Buffer classes_buffer = mission_lump.subbuf(header.classes.offset - mission_ranges[i].offset.bytes());
				mission.classes = std::vector<u8>();
				verify(decompress_wad(*mission.classes, wad_buffer(classes_buffer)), "Failed to decompress mission classes.");
			}
			is_mission = true;
		}
		if(mission_bank_ranges[i].size.sectors > 0) {
			mission.sound_bank = read_lump(file, mission_bank_ranges[i], "mission bank lump");
			is_mission = true;
		}
		if(is_mission) {
			missions.emplace(i, std::move(mission));
		}
	}
	return missions;
}

static std::vector<u8> build_level_wad(LevelWad& wad);
static SectorRange write_lump(OutBuffer dest, const std::vector<u8>& buffer);
static SectorRange write_compressed_lump(OutBuffer dest, const std::vector<u8>& buffer);
template <typename Header>
static void write_chunks(OutBuffer dest, Header& header, const std::map<s32, Chunk>& chunks);
static void write_missions(OutBuffer dest, DeadlockedLevelWadHeader& header, const std::map<s32, Mission>& missions);

void write_wad(FILE* file, Wad* wad) {
	if(wad->type == WadType::LEVEL) {
		std::vector<u8> level = build_level_wad(*dynamic_cast<LevelWad*>(wad));
		fwrite(level.data(), level.size(), 1, file);
	}
}

static std::vector<u8> build_level_wad(LevelWad& wad) {
	std::vector<u8> dest_vec;
	OutBuffer dest(dest_vec);
	switch(wad.game) {
		case Game::RAC1: {
			Rac1LevelWadHeader header = {0};
			header.header_size = sizeof(Rac1LevelWadHeader);
			header.level_number = wad.level_number;
			dest.alloc<Rac23LevelWadHeader>();
			header.primary = PrimaryLump::write(dest, wad);
			wad.help_messages.swap(wad.gameplay); // help_messages -> gameplay
			std::vector<u8> gameplay = write_gameplay(wad, wad.gameplay, wad.game, RAC23_GAMEPLAY_BLOCKS);
			wad.help_messages.swap(wad.gameplay); // gameplay -> help_messages
			header.gameplay_ntsc = write_compressed_lump(dest, gameplay);
			header.occlusion = write_lump(dest, write_occlusion(wad.gameplay, wad.game));
			dest.write(0, header);
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
			header.primary = PrimaryLump::write(dest, wad);
			wad.help_messages.swap(wad.gameplay); // help_messages -> gameplay
			std::vector<u8> gameplay = write_gameplay(wad, wad.gameplay, wad.game, RAC23_GAMEPLAY_BLOCKS);
			wad.help_messages.swap(wad.gameplay); // gameplay -> help_messages
			header.gameplay = write_compressed_lump(dest, gameplay);
			header.occlusion = write_lump(dest, write_occlusion(wad.gameplay, wad.game));
			write_chunks(dest, header, wad.chunks);
			dest.write(0, header);
			break;
		}
		case Game::DL: {
			DeadlockedLevelWadHeader header = {0};
			header.header_size = sizeof(DeadlockedLevelWadHeader);
			header.level_number = wad.level_number;
			header.reverb = *wad.reverb;
			for(const auto& [index, mission] : wad.missions) {
				if(mission.instances.has_value() && (s32) mission.instances->size() > header.max_mission_instances_size) {
					header.max_mission_instances_size = mission.instances->size();
				}
				if(mission.classes.has_value() && (s32) mission.classes->size() > header.max_mission_classes_size) {
					header.max_mission_classes_size = mission.classes->size();
				}
			}
			dest.alloc<DeadlockedLevelWadHeader>();
			header.core_bank = write_lump(dest, wad.core_bank);
			header.primary = PrimaryLump::write(dest, wad);
			write_chunks(dest, header, wad.chunks);
			wad.help_messages.swap(wad.gameplay); // help_messages -> gameplay
			std::vector<u8> gameplay = write_gameplay(wad, wad.gameplay, wad.game, DL_GAMEPLAY_CORE_BLOCKS);
			wad.help_messages.swap(wad.gameplay); // gameplay -> help_messages
			header.gameplay_core = write_compressed_lump(dest, gameplay);
			write_missions(dest, header, wad.missions);
			std::vector<u8> art_instances = write_gameplay(wad, wad.gameplay, wad.game, DL_ART_INSTANCE_BLOCKS);
			header.art_instances = write_compressed_lump(dest, art_instances);
			dest.write(0, header);
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
		(s32) (offset_bytes / SECTOR_SIZE),
		Sector32::size_from_bytes(size_bytes)
	};
}

static SectorRange write_compressed_lump(OutBuffer dest, const std::vector<u8>& buffer) {
	std::vector<u8> compressed;
	compress_wad(compressed, buffer, 8);
	return write_lump(dest, compressed);
}

template <typename Header>
static void write_chunks(OutBuffer dest, Header& header, const std::map<s32, Chunk>& chunks) {
	for(const auto& [index, chunk] : chunks) {
		if(chunk.tfrags.has_value() && chunk.collision.has_value()) {
			assert(index >= 0 && index <= 2);
			std::vector<u8> chunk_vec;
			OutBuffer chunk_buffer(chunk_vec);
			s64 header_ofs = chunk_buffer.alloc<ChunkHeader>();
			ChunkHeader chunk_header;
			chunk_buffer.pad(0x10, 0);
			chunk_header.tfrags = chunk_buffer.tell();
			compress_wad(chunk_vec, *chunk.tfrags, 8);
			chunk_buffer.pad(0x10, 0);
			chunk_header.collision = chunk_buffer.tell();
			compress_wad(chunk_vec, *chunk.collision, 8);
			chunk_buffer.write(header_ofs, chunk_header);
			header.chunks[index] = write_lump(dest, chunk_vec);
		}
	}
	for(const auto& [index, chunk] : chunks) {
		if(chunk.sound_bank.has_value()) {
			header.chunk_banks[index] = write_lump(dest, *chunk.sound_bank);
		}
	}
}

static void write_missions(OutBuffer dest, DeadlockedLevelWadHeader& header, const std::map<s32, Mission>& missions) {
	for(const auto& [index, mission] : missions) {
		assert(index >= 0 && index <= 127);
		if(mission.instances.has_value()) {
			header.gameplay_mission_instances[index] = write_lump(dest, *mission.instances);
		}
	}
	for(const auto& [index, mission] : missions) {
		dest.pad(SECTOR_SIZE, 0);
		std::vector<u8> mission_vec;
		OutBuffer mission_buffer(mission_vec);
		s64 header_ofs = mission_buffer.alloc<MissionHeader>();
		MissionHeader mission_header = {0};
		if(mission.instances.has_value()) {
			mission_buffer.pad(0x40, 0);
			mission_header.instances.offset = mission_buffer.tell();
			compress_wad(mission_vec, *mission.instances, 8);
			mission_header.instances.size = mission_buffer.tell() - mission_header.instances.offset;
			mission_header.instances.offset += dest.tell();
		} else {
			mission_header.instances.offset = -1;
		}
		if(mission.classes.has_value()) {
			mission_buffer.pad(0x40, 0);
			mission_header.classes.offset = mission_buffer.tell();
			compress_wad(mission_vec, *mission.classes, 8);
			mission_header.classes.size = mission_buffer.tell() - mission_header.classes.offset;
			mission_header.classes.offset += dest.tell();
		} else {
			mission_header.instances.offset = -1;
		}
		mission_buffer.write(header_ofs, mission_header);
		header.gameplay_mission_data[index] = write_lump(dest, mission_vec);
	}
	for(const auto& [index, mission] : missions) {
		if(mission.sound_bank.has_value()) {
			header.mission_banks[index] = write_lump(dest, *mission.sound_bank);
		}
	}
}
