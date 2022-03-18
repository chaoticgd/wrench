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

#include "table_of_contents.h"

#include <core/buffer.h>
#include <core/filesystem.h>
#include <editor/util.h>

static LevelWadInfo adapt_rac1_level_wad_header(FILE* iso, Rac1AmalgamatedWadHeader& header);
static Opt<LevelWadInfo> adapt_rac1_audio_wad_header(FILE* iso, Rac1AmalgamatedWadHeader& header);
static Opt<LevelWadInfo> adapt_rac1_scene_wad_header(FILE* iso, Rac1AmalgamatedWadHeader& header);
static Sector32 get_vag_size(FILE* iso, Sector32 sector);
static Sector32 get_lz_size(FILE* iso, Sector32 sector);

static std::size_t get_rac234_level_table_offset(Buffer src);

table_of_contents read_table_of_contents(FILE* iso, Game game) {
	table_of_contents toc;
	if(game == Game::RAC1) {
		toc = read_table_of_contents_rac1(iso);
	} else {
		toc = read_table_of_contents_rac234(iso);
		if(toc.levels.size() == 0) {
			fprintf(stderr, "error: Unable to locate level table!\n");
			exit(1);
		}
	}
	return toc;
}

table_of_contents read_table_of_contents_rac1(FILE* iso) {
	s32 magic, toc_size;
	fseek(iso, RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE, SEEK_SET);
	verify(fread(&magic, 4, 1, iso) == 1, "Failed to read R&C1 table of contents.");
	verify(fread(&toc_size, 4, 1, iso) == 1, "Failed to read R&C1 table of contents.");
	verify(toc_size > 0 && toc_size < 1024 * 1024 * 1024, "Invalid R&C1 table of contents.");
	verify(magic == 1, "Invalid R&C1 table of contents.");
	std::vector<u8> bytes = read_file(iso, RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE, toc_size);
	Buffer buffer(bytes);
	
	table_of_contents toc;
	for(s64 ofs = 8; ofs < buffer.size(); ofs += 8) {
		Sector32 lsn = buffer.read<Sector32>(ofs, "sector");
		Sector32 size = buffer.read<Sector32>(ofs + 4, "size");
		if(lsn.sectors != 0) {
			std::vector<u8> header_bytes = read_file(iso, lsn.bytes(), 0x2434);
			Buffer header_buffer(header_bytes);
			if(header_buffer.read<s32>(4, "file header") == 0x2434) {
				Rac1AmalgamatedWadHeader header = header_buffer.read<Rac1AmalgamatedWadHeader>(0, "level header");
				
				LevelInfo level;
				level.level_table_index = header.level_number;
				level.parts[0] = adapt_rac1_level_wad_header(iso, header);
				level.parts[1] = adapt_rac1_audio_wad_header(iso, header);
				level.parts[2] = adapt_rac1_scene_wad_header(iso, header);
				toc.levels.emplace_back(std::move(level));
			}
		}
	}
	return toc;
}

static SectorRange sub_sector_range(SectorRange range, Sector32 lsn) {
	return {{range.offset.sectors - lsn.sectors}, range.size};
}

static SectorByteRange sub_sector_byte_range(SectorByteRange range, Sector32 lsn) {
	return {{range.offset.sectors - lsn.sectors}, range.size_bytes};
}

static LevelWadInfo adapt_rac1_level_wad_header(FILE* iso, Rac1AmalgamatedWadHeader& header) {
	// Determine where the file begins and ends on disc.
	Sector32 low = {INT32_MAX};
	low = {std::min(low.sectors, header.primary.offset.sectors)};
	low = {std::min(low.sectors, header.gameplay_ntsc.offset.sectors)};
	low = {std::min(low.sectors, header.gameplay_pal.offset.sectors)};
	low = {std::min(low.sectors, header.occlusion.offset.sectors)};
	
	Sector32 high = {0};
	high = {std::max(high.sectors, header.primary.end().sectors)};
	high = {std::max(high.sectors, header.gameplay_ntsc.end().sectors)};
	high = {std::max(high.sectors, header.gameplay_pal.end().sectors)};
	high = {std::max(high.sectors, header.occlusion.end().sectors)};
	
	// Rewrite header, convert absolute sector numbers to relative sector offsets.
	s32 header_size_sectors = Sector32::size_from_bytes(sizeof(Rac1LevelWadHeader)).sectors;
	Sector32 adjust = {low.sectors - header_size_sectors};
	Rac1LevelWadHeader level_header = {0};
	level_header.header_size = sizeof(Rac1LevelWadHeader);
	level_header.level_number = header.level_number;
	level_header.primary = sub_sector_range(header.primary, adjust);
	level_header.gameplay_ntsc = sub_sector_range(header.gameplay_ntsc, adjust);
	level_header.gameplay_pal = sub_sector_range(header.gameplay_pal, adjust);
	level_header.occlusion = sub_sector_range(header.occlusion, adjust);
	
	LevelWadInfo level_part;
	level_part.header_lba = {0};
	level_part.magic = sizeof(Rac1LevelWadHeader);
	level_part.file_lba = {low.sectors};
	level_part.file_size = {high.sectors - low.sectors};
	level_part.info = {level_file_type::LEVEL, "level",  GAME_RAC1};
	level_part.header.resize(sizeof(Rac1LevelWadHeader));
	memcpy(level_part.header.data(), &level_header, sizeof(Rac1LevelWadHeader));
	level_part.prepend_header = true;
	
	verify(level_part.file_size.bytes() < 1024 * 1024 * 1024, "Level WAD too big!");
	
	return level_part;
}

static Opt<LevelWadInfo> adapt_rac1_audio_wad_header(FILE* iso, Rac1AmalgamatedWadHeader& header) {
	// Determine where the file begins and ends on disc.
	Sector32 low = {INT32_MAX};
	Sector32 high = {0};
	for(s32 i = 0; i < sizeof(header.bindata) / sizeof(header.bindata[0]); i++) {
		if(header.bindata[i].offset.sectors > 0) {
			low = {std::min(low.sectors, header.bindata[i].offset.sectors)};
			high = {std::max(high.sectors, header.bindata[i].end().sectors)};
		}
	}
	for(s32 i = 0; i < sizeof(header.music) / sizeof(header.music[0]); i++) {
		if(header.music[i].sectors > 0) {
			low = {std::min(low.sectors, header.music[i].sectors)};
			high = {std::max(high.sectors, header.music[i].sectors + get_vag_size(iso, header.music[i].sectors).sectors)};
		}
	}
	if(low.sectors == INT32_MAX || high.sectors == 0) {
		return {};
	}
	
	s32 header_size_sectors = Sector32::size_from_bytes(sizeof(Rac1AudioWadHeader)).sectors;
	Sector32 adjust = {low.sectors - header_size_sectors};
	
	// Rewrite header, convert absolute sector numbers to relative sector offsets.
	Rac1AudioWadHeader audio_header = {0};
	audio_header.header_size = sizeof(Rac1AudioWadHeader);
	for(s32 i = 0; i < sizeof(header.bindata) / sizeof(header.bindata[0]); i++) {
		if(header.bindata[i].offset.sectors > 0) {
			audio_header.bindata[i] = sub_sector_byte_range(header.bindata[i], adjust);
		}
	}
	for(s32 i = 0; i < sizeof(header.music) / sizeof(header.music[0]); i++) {
		if(header.music[i].sectors > 0) {
			audio_header.music[i] = {header.music[i].sectors - adjust.sectors};
		}
	}
	
	LevelWadInfo audio_part;
	audio_part.header_lba = {0};
	audio_part.magic = sizeof(Rac1AudioWadHeader);
	audio_part.file_lba = low;
	audio_part.file_size = {high.sectors - low.sectors};
	audio_part.info = {level_file_type::AUDIO, "audio",  GAME_RAC1};
	audio_part.header.resize(sizeof(Rac1AudioWadHeader));
	memcpy(audio_part.header.data(), &audio_header, sizeof(Rac1AudioWadHeader));
	audio_part.prepend_header = true;
	
	verify(audio_part.file_size.bytes() < (1024 * 1024 * 1024), "Level audio WAD too big!");
	
	return audio_part;
}

static Opt<LevelWadInfo> adapt_rac1_scene_wad_header(FILE* iso, Rac1AmalgamatedWadHeader& header) {
	// Determine where the file begins and ends on disc.
	Sector32 low = {INT32_MAX};
	Sector32 high = {0};
	for(s32 i = 0; i < sizeof(header.scenes) / sizeof(header.scenes[0]); i++) {
		const Rac1SceneHeader& scene = header.scenes[i];
		for(s32 j = 0; j < sizeof(scene.sounds) / sizeof(scene.sounds[0]); j++) {
			if(scene.sounds[j].sectors > 0) {
				low = {std::min(low.sectors, scene.sounds[j].sectors)};
				high = {std::max(high.sectors, scene.sounds[j].sectors + get_vag_size(iso, scene.sounds[j].sectors).sectors)};
			}
		}
		for(s32 j = 0; j < sizeof(scene.wads) / sizeof(scene.wads[0]); j++) {
			if(scene.wads[j].sectors > 0) {
				low = {std::min(low.sectors, scene.wads[j].sectors)};
				high = {std::max(high.sectors, scene.wads[j].sectors + get_lz_size(iso, scene.wads[j].sectors).sectors)};
			}
		}
	}
	if(low.sectors == INT32_MAX || high.sectors == 0) {
		return {};
	}
	
	s32 header_size_sectors = Sector32::size_from_bytes(sizeof(Rac1SceneWadHeader)).sectors;
	Sector32 adjust = {low.sectors - header_size_sectors};
	
	// Rewrite header, convert absolute sector numbers to relative sector offsets.
	Rac1SceneWadHeader scene_header = {0};
	scene_header.header_size = sizeof(Rac1SceneWadHeader);
	for(s32 i = 0; i < sizeof(header.scenes) / sizeof(header.scenes[0]); i++) {
		Rac1SceneHeader& dest = scene_header.scenes[i];
		Rac1SceneHeader& src = header.scenes[i];
		for(s32 j = 0; j < sizeof(dest.sounds)/ sizeof(dest.sounds[0]); j++) {
			if(src.sounds[j].sectors > 0) {
				dest.sounds[j] = {src.sounds[j].sectors - adjust.sectors};
			}
		}
		for(s32 j = 0; j < sizeof(dest.wads) / sizeof(dest.wads[0]); j++) {
			if(src.wads[j].sectors > 0) {
				dest.wads[j] = {src.wads[j].sectors - adjust.sectors};
			}
		}
	}
	
	LevelWadInfo scene_part;
	scene_part.header_lba = {0};
	scene_part.magic = sizeof(Rac1SceneWadHeader);
	scene_part.file_lba = low;
	scene_part.file_size = {high.sectors - low.sectors};
	scene_part.info = {level_file_type::SCENE, "scene",  GAME_RAC1};
	scene_part.header.resize(sizeof(Rac1SceneWadHeader));
	memcpy(scene_part.header.data(), &scene_header, sizeof(Rac1SceneWadHeader));
	scene_part.prepend_header = true;
	
	verify(scene_part.file_size.bytes() < 1024 * 1024 * 1024, "Level scene WAD too big!");
	
	return scene_part;
}

static Sector32 get_vag_size(FILE* iso, Sector32 sector) {
	std::vector<u8> header_bytes = read_file(iso, sector.bytes(), sizeof(VagHeader));
	VagHeader header = Buffer(header_bytes).read<VagHeader>(0, "vag header");
	if(memcmp(header.magic, "VAGp", 4) != 0) {
		return {1};
	}
	return Sector32::size_from_bytes(sizeof(VagHeader) + byte_swap_32(header.data_size));
}

static Sector32 get_lz_size(FILE* iso, Sector32 sector) {
	std::vector<u8> header_bytes = read_file(iso, sector.bytes(), sizeof(LzHeader));
	LzHeader header = Buffer(header_bytes).read<LzHeader>(0, "WAD LZ header");
	if(memcmp(header.magic, "WAD", 3) != 0) {
		return {1};
	}
	return Sector32::size_from_bytes(header.compressed_size);
}

table_of_contents read_table_of_contents_rac234(FILE* iso) {
	std::vector<u8> bytes = read_file(iso, RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE, TOC_MAX_SIZE);
	Buffer buffer(bytes);
	
	table_of_contents toc;
	
	std::size_t level_table_offset = get_rac234_level_table_offset(buffer);
	if(level_table_offset == 0x0) {
		// We've failed to find the level table, at least try to find some of the other tables.
		level_table_offset = 0xffff;
	}
	
	std::size_t global_index = 0;
	s64 ofs = 0;
	while(ofs + 4 * 6 < level_table_offset) {
		GlobalWadInfo global;
		global.index = global_index++;
		global.offset_in_toc = ofs;
		s32 header_size = buffer.read<s32>(ofs, "global header");
		if(header_size < 8 || header_size > 0xffff) {
			break;
		}
		global.sector = buffer.read<Sector32>(ofs + 4, "global header");
		global.header = buffer.read_multiple<u8>(ofs, header_size, "global header").copy();
		toc.globals.emplace_back(std::move(global));
		ofs += header_size;
	}
	
	// This fixes an off-by-one error with R&C3 where since the first entry of
	// the level table is supposed to be zeroed out, this code would otherwise
	// think that the level table starts 0x18 bytes later than it actually does.
	if(ofs + 0x18 == level_table_offset) {
		level_table_offset -= 0x18;
	}
	
	auto level_table = buffer.read_multiple<toc_level_table_entry>(level_table_offset, TOC_MAX_LEVELS, "level table");
	for(size_t i = 0; i < TOC_MAX_LEVELS; i++) {
		toc_level_table_entry entry = level_table[i];
		
		LevelInfo level;
		level.level_table_index = i;
		bool has_level_part = false;
		
		// The games have the fields in different orders, so we check the type
		// of what each field points to so we can support them all.
		for(size_t j = 0; j < 3; j++) {
			LevelWadInfo part;
			part.header_lba = entry.parts[j].offset;
			part.file_size = entry.parts[j].size;
			
			Sector32 sector = {part.header_lba.sectors - (s32) RAC234_TABLE_OF_CONTENTS_LBA};
			if(sector.sectors <= 0) {
				continue;
			}
			if(sector.bytes() > buffer.size()) {
				break;
			}
			part.magic = buffer.read<uint32_t>(sector.bytes(), "level header size");
			part.file_lba = buffer.read<Sector32>(sector.bytes() + 4, "level sector number");
			
			auto info = LEVEL_FILE_TYPES.find(part.magic);
			if(info == LEVEL_FILE_TYPES.end()) {
				continue;
			}
			part.info = info->second;
			
			part.header = buffer.read_multiple<u8>(sector.bytes(), part.magic, "level header").copy();
			
			has_level_part |= part.info.type == level_file_type::LEVEL;
			level.parts[j] = part;
		}
		
		if(has_level_part) {
			toc.levels.push_back(level);
		}
	}
	
	return toc;
}

static std::size_t get_rac234_level_table_offset(Buffer src) {
	// Check that the two next entries are valid. This is necessary to
	// get past a false positive in Deadlocked.
	for(size_t i = 0; i < src.size() / 4 - 12; i++) {
		int parts = 0;
		for(int j = 0; j < 6; j++) {
			Sector32 lsn = src.read<Sector32>((i + j * 2) * 4, "table of contents");
			size_t header_offset = lsn.bytes() - RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE;
			if(lsn.sectors == 0 || header_offset > TOC_MAX_SIZE - 4) {
				break;
			}
			uint32_t header_size = src.read<uint32_t>(header_offset, "level header size");
			if(LEVEL_FILE_TYPES.find(header_size) != LEVEL_FILE_TYPES.end()) {
				parts++;
			}
		}
		if(parts == 6) {
			return i * 4;
		}
	}
	return 0;
}
