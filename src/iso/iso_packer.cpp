/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "iso_packer.h"

#include <iso/wad_identifier.h>

static std::vector<GlobalWadInfo> enumerate_globals(BuildAsset& build, Game game);
static std::vector<LevelInfo> enumerate_levels(BuildAsset& build, Game game);
static IsoDirectory enumerate_files(Asset& files);
static void flatten_files(std::vector<IsoFileRecord*>& dest, IsoDirectory& root_dir);
static IsoFileRecord write_system_cnf(OutputStream& iso, IsoDirectory& root_dir, BuildAsset& build);
static void pack_files(OutputStream& iso, std::vector<IsoFileRecord*>& files, Game game, AssetPackerFunc pack);
static IsoDirectory pack_globals(OutputStream& iso, std::vector<GlobalWadInfo>& globals, Game game, AssetPackerFunc pack);
static std::array<IsoDirectory, 3> pack_levels(OutputStream& iso, std::vector<LevelInfo>& levels, Game game, s32 single_level_index, AssetPackerFunc pack);
static void pack_level_wad_outer(OutputStream& iso, IsoDirectory& directory, LevelWadInfo& wad, const char* name, Game game, s32 index, AssetPackerFunc pack);

void pack_iso(OutputStream& iso, BuildAsset& build, Game game, AssetPackerFunc pack) {
	s32 single_level_index = -1;
	
	table_of_contents toc;
	toc.globals = enumerate_globals(build, game);
	toc.levels = enumerate_levels(build, game);
	
	Sector32 toc_size = calculate_table_of_contents_size(toc, single_level_index);
	
	// Mustn't modify root_dir until after pack_files is called, or the
	// flattened file pointers will be invalidated.
	IsoDirectory root_dir = enumerate_files(build.get_files());
	std::vector<IsoFileRecord*> files;
	flatten_files(files, root_dir);
	
	// Write out blank sectors that are to be filled in later.
	iso.pad(SECTOR_SIZE, 0);
	static const u8 null_sector[SECTOR_SIZE] = {0};
	while(iso.tell() < SYSTEM_CNF_LBA * SECTOR_SIZE) {
		iso.write(null_sector, SECTOR_SIZE);
	}
	
	// SYSTEM.CNF must be written out at sector 1000 (the game hardcodes this).
	IsoFileRecord system_cnf_record = write_system_cnf(iso, root_dir, build);
	
	// Then the table of contents at sector 1001 (also hardcoded).
	IsoFileRecord toc_record;
	{
		iso.pad(SECTOR_SIZE, 0);
		switch(game) {
			case Game::RAC1:    toc_record.name = "rc1.hdr";     break;
			case Game::RAC2:    toc_record.name = "rc2.hdr";     break;
			case Game::RAC3:    toc_record.name = "rc3.hdr";     break;
			case Game::DL:      toc_record.name = "rc4.hdr";     break;
			case Game::UNKNOWN: toc_record.name = "unknown.hdr"; break;
		}
		toc_record.lba = {RAC234_TABLE_OF_CONTENTS_LBA};
		toc_record.size = toc_size.bytes();
		toc_record.modified_time = fs::file_time_type::clock::now();
	}
	
	// Write out blank sectors that are to be filled in by the table of contents later.
	iso.pad(SECTOR_SIZE, 0);
	assert(iso.tell() == RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	while(iso.tell() < RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE + toc_size.bytes()) {
		iso.write(null_sector, SECTOR_SIZE);
	}
	
	s64 files_begin = iso.tell();
	
	pack_files(iso, files, game, pack);
	
	root_dir.files.emplace(root_dir.files.begin(), std::move(system_cnf_record));
	root_dir.files.emplace(root_dir.files.begin(), std::move(toc_record));
	
	root_dir.subdirs.emplace_back(pack_globals(iso, toc.globals, game, pack));
	auto [levels_dir, audio_dir, scenes_dir] =
		pack_levels(iso, toc.levels, game, single_level_index, pack);
	root_dir.subdirs.emplace_back(std::move(levels_dir));
	root_dir.subdirs.emplace_back(std::move(audio_dir));
	root_dir.subdirs.emplace_back(std::move(scenes_dir));
	
	iso.pad(SECTOR_SIZE, 0);
	s64 volume_size = iso.tell() / SECTOR_SIZE;
	
	iso.seek(0);
	write_iso_filesystem(iso, &root_dir);
	assert(iso.tell() <= RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	iso.write<IsoLsbMsb32>(0x8050, IsoLsbMsb32::from_scalar(volume_size));
	
	s64 toc_end = write_table_of_contents_rac234(iso, toc, game);
	assert(toc_end <= files_begin);
}

static std::vector<GlobalWadInfo> enumerate_globals(BuildAsset& build, Game game) {
	std::vector<WadType> order;
	switch(game) {
		case Game::RAC1: {
			order = {};
			break;
		}
		case Game::RAC2: {
			order = {
				WadType::MPEG,
				WadType::MISC,
				WadType::HUD,
				WadType::BONUS,
				WadType::AUDIO,
				WadType::SPACE,
				WadType::SCENE,
				WadType::GADGET,
				WadType::ARMOR
			};
			break;
		}
		case Game::RAC3: {
			break;
		}
		case Game::DL: {
			order = {
				WadType::MPEG,
				WadType::MISC,
				WadType::BONUS,
				WadType::SPACE,
				WadType::ARMOR,
				WadType::AUDIO,
				WadType::HUD,
				WadType::ONLINE
			};
			break;
		}
	}
	
	std::vector<GlobalWadInfo> globals;
	for(WadType type : order) {
		GlobalWadInfo global;
		switch(type) {
			case WadType::ARMOR:  global.asset = &build.get_armor();  global.name = "armor.wad";  break;
			case WadType::AUDIO:  global.asset = &build.get_audio();  global.name = "audio.wad";  break;
			case WadType::BONUS:  global.asset = &build.get_bonus();  global.name = "bonus.wad";  break;
			case WadType::GADGET: global.asset = &build.get_gadget(); global.name = "gadget.wad"; break;
			case WadType::HUD:    global.asset = &build.get_hud();    global.name = "hud.wad";    break;
			case WadType::MISC:   global.asset = &build.get_misc();   global.name = "misc.wad";   break;
			case WadType::MPEG:   global.asset = &build.get_mpeg();   global.name = "mpeg.wad";   break;
			case WadType::ONLINE: global.asset = &build.get_online(); global.name = "online.wad"; break;
			case WadType::SCENE:  global.asset = &build.get_scene();  global.name = "scene.wad";  break;
			case WadType::SPACE:  global.asset = &build.get_space();  global.name = "space.wad";  break;
		}
		global.header.resize(header_size_of_wad(game, type));
		verify(global.asset, "Failed to build ISO, missing global WAD asset.");
		globals.emplace_back(std::move(global));
	}
	return globals;
}

static std::vector<LevelInfo> enumerate_levels(BuildAsset& build, Game game) {
	std::vector<LevelInfo> levels;
	
	build.get_levels().for_each_logical_child_of_type<LevelAsset>([&](LevelAsset& level) {
		LevelInfo info;
		
		if(level.has_level()) {
			LevelWadInfo& wad = info.level.emplace();
			wad.header.resize(header_size_of_wad(game, WadType::LEVEL));
			wad.asset = &level.get_level();
		}
		if(level.has_audio()) {
			LevelWadInfo& wad = info.audio.emplace();
			wad.header.resize(header_size_of_wad(game, WadType::LEVEL_AUDIO));
			wad.asset = &level.get_audio();
		}
		if(level.has_scene()) {
			LevelWadInfo& wad = info.scene.emplace();
			wad.header.resize(header_size_of_wad(game, WadType::LEVEL_SCENE));
			wad.asset = &level.get_scene();
		}
		
		levels.emplace_back(std::move(info));
	});
	
	return levels;
}

static IsoDirectory enumerate_files(Asset& files) {
	IsoDirectory root;
	
	files.for_each_logical_child_of_type<FileAsset>([&](FileAsset& file) {
		IsoDirectory* current_dir = &root;
		std::string dvd_path = file.path();
		for(;;) {
			size_t index = dvd_path.find_first_of('/');
			if(index == std::string::npos) {
				break;
			}
			
			std::string dir_name = dvd_path.substr(0, index);
			dvd_path = dvd_path.substr(index + 1);
			
			bool found = false;
			for(IsoDirectory& subdir : current_dir->subdirs) {
				if(subdir.name == dir_name) {
					current_dir = &subdir;
					found = true;
					break;
				}
			}
			
			if(!found) {
				IsoDirectory& new_dir = current_dir->subdirs.emplace_back();
				new_dir.name = dir_name;
				current_dir = &new_dir;
			}
		}
		
		IsoFileRecord& record = current_dir->files.emplace_back();
		record.name = dvd_path;
		record.asset = &file;
	});
	
	return root;
}

static void flatten_files(std::vector<IsoFileRecord*>& dest, IsoDirectory& root_dir) {
	for(IsoFileRecord& file : root_dir.files) {
		dest.push_back(&file);
	}
	for(IsoDirectory& dir : root_dir.subdirs) {
		flatten_files(dest, dir);
	}
}

static IsoFileRecord write_system_cnf(OutputStream& iso, IsoDirectory& root_dir, BuildAsset& build) {
	// TODO: Add a seperate attribute for the system.cnf file or generate it.
	FileAsset& file_asset = build.get_files().get_child("system_cnf").as<FileAsset>();
	FileReference ref = file_asset.src();
	
	auto stream = ref.owner->open_binary_file_for_reading(ref);
	verify(stream.get(), "Failed to open '%s' for reading.", ref.path.string().c_str());
	s64 system_cnf_size = stream->size();
	
	iso.pad(SECTOR_SIZE, 0);
	
	IsoFileRecord record;
	record.name = "system.cnf";
	record.lba = {(s32) (iso.tell() / SECTOR_SIZE)};
	record.size = system_cnf_size;
	record.modified_time = fs::file_time_type::clock::now();
	
	Stream::copy(iso, *stream, system_cnf_size);
	
	return record;
}

static void pack_files(OutputStream& iso, std::vector<IsoFileRecord*>& files, Game game, AssetPackerFunc pack) {
	for(IsoFileRecord* file : files) {
		if(file->name.find(".hdr") != std::string::npos) {
			// We're writing out a new table of contents, so if an old one
			// already exists we don't want to write it out.
			continue;
		}
		
		if(file->name.find("system.cnf") == 0) {
			// SYSTEM.CNF must be written out at a fixed sector number so we
			// handle it seperately.
			continue;
		}
		
		iso.pad(SECTOR_SIZE, 0);
		file->lba = {(s32) (iso.tell() / SECTOR_SIZE)};
		pack(iso, nullptr, &file->modified_time, *file->asset, game, FMT_NO_HINT);
		
		s64 end_of_file = iso.tell();
		file->size = (u32) (end_of_file - file->lba.bytes());
	}
}

static IsoDirectory pack_globals(OutputStream& iso, std::vector<GlobalWadInfo>& globals, Game game, AssetPackerFunc pack) {
	IsoDirectory globals_dir {"globals"};
	for(GlobalWadInfo& global : globals) {
		iso.pad(SECTOR_SIZE, 0);
		Sector32 sector = Sector32::size_from_bytes(iso.tell());
		
		assert(global.asset);
		fs::file_time_type modified_time;
		pack(iso, &global.header, &modified_time, *global.asset, game, FMT_NO_HINT);
		
		s64 end_of_file = iso.tell();
		s64 file_size = end_of_file - sector.bytes();
		
		global.index = 0; // Don't care.
		global.offset_in_toc = 0; // Don't care.
		global.sector = sector;
		
		IsoFileRecord record;
		record.name = global.name;
		record.lba = sector;
		assert(file_size < UINT32_MAX);
		record.size = file_size;
		record.modified_time = std::move(modified_time);
		globals_dir.files.push_back(record);
	}
	return globals_dir;
}

static std::array<IsoDirectory, 3> pack_levels(OutputStream& iso, std::vector<LevelInfo>& levels, Game game, s32 single_level_index, AssetPackerFunc pack) {
	// Create directories for the level files.
	IsoDirectory levels_dir {"levels"};
	IsoDirectory audio_dir {"audio"};
	IsoDirectory scenes_dir {"scenes"};
	if(single_level_index > -1) {
		// Only write out a single level, and point every level at it.
		//auto& p = level_files[single_level_index].parts;
		//LevelWadInfo level_part, audio_part, scene_part;
		//assert(p[LEVEL_PART]);
		//level_part = write_level_part(levels_dir, *p[LEVEL_PART]);
		//if(p[AUDIO_PART]) audio_part = write_level_part(audio_dir, *p[AUDIO_PART]);
		//if(p[SCENE_PART]) scene_part = write_level_part(scenes_dir, *p[SCENE_PART]);
		//for(size_t i = 0; i < level_files.size(); i++) {
		//	toc_levels[i].parts[0] = level_part;
		//	if(p[AUDIO_PART]) toc_levels[i].parts[1] = audio_part;
		//	if(p[SCENE_PART]) toc_levels[i].parts[2] = scene_part;
		//}
	} else if(game == Game::RAC2) {
		// The level files are laid out AoS.
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.level) pack_level_wad_outer(iso, levels_dir, *level.level, "level", game, i, pack);
			if(level.audio) pack_level_wad_outer(iso, audio_dir, *level.audio, "audio", game, i, pack);
			if(level.scene) pack_level_wad_outer(iso, scenes_dir, *level.scene, "scene", game, i, pack);
		}
	} else {
		// The level files are laid out SoA, audio files first.
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.audio) pack_level_wad_outer(iso, audio_dir, *level.audio, "audio", game, i, pack);
		}
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.level) pack_level_wad_outer(iso, levels_dir, *level.level, "level", game, i, pack);
		}
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.scene) pack_level_wad_outer(iso, scenes_dir, *level.scene, "scene", game, i, pack);
		}
	}
	return {levels_dir, audio_dir, scenes_dir};
}

static void pack_level_wad_outer(OutputStream& iso, IsoDirectory& directory, LevelWadInfo& wad, const char* name, Game game, s32 index, AssetPackerFunc pack) {
	std::string file_name = stringf("%s%02d.wad", name, index);
	
	iso.pad(SECTOR_SIZE, 0);
	Sector32 sector = Sector32::size_from_bytes(iso.tell());
	
	assert(wad.asset);
	fs::file_time_type modified_time;
	pack(iso, &wad.header, &modified_time, *wad.asset, game, FMT_NO_HINT);
	
	s64 end_of_file = iso.tell();
	s64 file_size = end_of_file - sector.bytes();
	
	wad.header_lba = {0}; // Don't care.
	wad.file_size = Sector32::size_from_bytes(file_size);
	wad.file_lba = sector;
	
	IsoFileRecord record;
	record.name = file_name.c_str();
	record.lba = sector;
	assert(file_size < UINT32_MAX);
	record.size = file_size;
	record.modified_time = std::move(modified_time);
	directory.files.push_back(record);
}
