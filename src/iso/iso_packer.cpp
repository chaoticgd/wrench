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
static IsoDirectory enumerate_files(std::vector<Asset*> assets);
static void flatten_files(std::vector<IsoFileRecord*>& dest, IsoDirectory& root_dir);
static IsoFileRecord write_system_cnf(OutputStream& iso, IsoDirectory& root_dir, BuildAsset& build);
static void write_files(OutputStream& iso, std::vector<IsoFileRecord*>& files);
static IsoDirectory pack_globals(OutputStream& iso, std::vector<GlobalWadInfo>& globals, const AssetPackerFunc& pack_asset, Game game);
static std::array<IsoDirectory, 3> pack_levels(OutputStream& iso, std::vector<LevelInfo>& levels, const AssetPackerFunc& pack_asset, Game game, s32 single_level_index);
static void pack_level_wad(OutputStream& iso, IsoDirectory& directory, LevelWadInfo& wad, const AssetPackerFunc& pack_asset, const char* name, Game game, s32 index);

void pack_iso(OutputStream& iso, BuildAsset& build, Game game, const AssetPackerFunc& pack_asset) {
	s32 single_level_index = -1;
	
	table_of_contents toc;
	toc.globals = enumerate_globals(build, game);
	toc.levels = enumerate_levels(build, game);
	
	Sector32 toc_size = calculate_table_of_contents_size(toc, single_level_index);
	
	// Mustn't modify root_dir until after write_files is called, or the
	// flattened file pointers will be invalidated.
	IsoDirectory root_dir = enumerate_files(build.files());
	std::vector<IsoFileRecord*> files;
	flatten_files(files, root_dir);
	
	// Write out blank sectors that are to be filled in later.
	iso.pad(SECTOR_SIZE, 0);
	static const u8 null_sector[SECTOR_SIZE] = {0};
	while(iso.tell() < SYSTEM_CNF_LBA * SECTOR_SIZE) {
		iso.write(null_sector, SECTOR_SIZE);
	}
	
	printf("Sector          Size (bytes)    Filename\n");
	printf("------          ------------    --------\n");
	
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
		print_file_record(toc_record);
	}
	
	// Write out blank sectors that are to be filled in by the table of contents later.
	iso.pad(SECTOR_SIZE, 0);
	assert(iso.tell() == RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	while(iso.tell() < RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE + toc_size.bytes()) {
		iso.write(null_sector, SECTOR_SIZE);
	}
	
	s64 files_begin = iso.tell();
	
	write_files(iso, files);
	
	root_dir.files.emplace(root_dir.files.begin(), std::move(system_cnf_record));
	root_dir.files.emplace(root_dir.files.begin(), std::move(toc_record));
	
	root_dir.subdirs.emplace_back(pack_globals(iso, toc.globals, pack_asset, game));
	auto [levels_dir, audio_dir, scenes_dir] =
		pack_levels(iso, toc.levels, pack_asset, game, single_level_index);
	root_dir.subdirs.emplace_back(std::move(levels_dir));
	root_dir.subdirs.emplace_back(std::move(audio_dir));
	root_dir.subdirs.emplace_back(std::move(scenes_dir));
	
	s64 volume_size = iso.tell();
	
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
			case WadType::ARMOR:  global.asset = build.armor();  global.name = "armor.wad";  break;
			case WadType::AUDIO:  global.asset = build.audio();  global.name = "audio.wad";  break;
			case WadType::BONUS:  global.asset = build.bonus();  global.name = "bonus.wad";  break;
			case WadType::GADGET: global.asset = build.gadget(); global.name = "gadget.wad"; break;
			case WadType::HUD:    global.asset = build.hud();    global.name = "hud.wad";    break;
			case WadType::MISC:   global.asset = build.misc();   global.name = "misc.wad";   break;
			case WadType::MPEG:   global.asset = build.mpeg();   global.name = "mpeg.wad";   break;
			case WadType::ONLINE: global.asset = build.online(); global.name = "online.wad"; break;
			case WadType::SCENE:  global.asset = build.scene();  global.name = "scene.wad";  break;
			case WadType::SPACE:  global.asset = build.space();  global.name = "space.wad";  break;
		}
		global.header.resize(header_size_of_wad(game, type));
		verify(global.asset, "Failed to build ISO, missing global WAD asset.");
		globals.emplace_back(std::move(global));
	}
	return globals;
}

static std::vector<LevelInfo> enumerate_levels(BuildAsset& build, Game game) {
	std::vector<LevelInfo> levels;
	
	for(Asset* asset : build.levels()) {
		if(asset && asset->type() == LevelAsset::ASSET_TYPE) {
			LevelInfo level;
			
			Asset* null_asset = nullptr;
			
			LevelAsset& level_asset = static_cast<LevelAsset&>(*asset);
			if(Asset* wad_asset = level_asset.level(null_asset)) {
				LevelWadInfo& wad = level.level.emplace();
				wad.header.resize(header_size_of_wad(game, WadType::LEVEL));
				wad.asset = wad_asset;
			}
			if(Asset* wad_asset = level_asset.audio(null_asset)) {
				LevelWadInfo& wad = level.audio.emplace();
				wad.header.resize(header_size_of_wad(game, WadType::LEVEL_AUDIO));
				wad.asset = wad_asset;
			}
			if(Asset* wad_asset = level_asset.scene(null_asset)) {
				LevelWadInfo& wad = level.scene.emplace();
				wad.header.resize(header_size_of_wad(game, WadType::LEVEL_SCENE));
				wad.asset = wad_asset;
			}
			
			levels.emplace_back(std::move(level));
		}
	}
	
	return levels;
}

static IsoDirectory enumerate_files(std::vector<Asset*> assets) {
	IsoDirectory root;
	
	for(Asset* asset : assets) {
		if(FileAsset* file = dynamic_cast<FileAsset*>(asset)) {
			IsoDirectory* current_dir = &root;
			std::string dvd_path = file->path();
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
			record.asset = file;
		}
	}
	
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
	FileReference ref;
	// TODO: Add a seperate attribute for the system.cnf file or generate it.
	for(Asset* asset : build.files()) {
		if(asset && asset->type() == FileAsset::ASSET_TYPE && asset->tag() == "system_cnf") {
			ref = static_cast<FileAsset*>(asset)->src();
		}
	}
	if(ref.path.empty()) {
		fprintf(stderr, "error: No SYSTEM.CNF file referenced!\n");
		exit(1);
	}
	
	auto stream = ref.owner->open_binary_file_for_reading(ref);
	verify(stream.get(), "Failed to open '%s' for reading.", ref.path.string().c_str());
	s64 system_cnf_size = stream->size();
	
	iso.pad(SECTOR_SIZE, 0);
	
	IsoFileRecord record;
	record.name = "system.cnf";
	record.lba = {(s32) (iso.tell() / SECTOR_SIZE)};
	record.size = system_cnf_size;
	record.modified_time = fs::file_time_type::clock::now();
	
	print_file_record(record);
	
	Stream::copy(iso, *stream, system_cnf_size);
	
	return record;
}

static void write_files(OutputStream& iso, std::vector<IsoFileRecord*>& files) {
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
		
		FileReference ref = file->asset->src();
		std::unique_ptr<InputStream> src = file->asset->file().open_binary_file_for_reading(ref, &file->modified_time);
		verify(src.get(), "Failed to open file '%s' for reading.", file->name.c_str());
		
		iso.pad(SECTOR_SIZE, 0);
		
		s64 file_size = src->size();
		file->lba = {(s32) (iso.tell() / SECTOR_SIZE)};
		file->size = file_size;
		print_file_record(*file);
		
		Stream::copy(iso, *src, file_size);
	}
}

static IsoDirectory pack_globals(OutputStream& iso, std::vector<GlobalWadInfo>& globals, const AssetPackerFunc& pack_asset, Game game) {
	IsoDirectory globals_dir {"globals"};
	for(GlobalWadInfo& global : globals) {
		iso.pad(SECTOR_SIZE, 0);
		Sector32 sector = Sector32::size_from_bytes(iso.tell());
		
		printf("%-16ld                %s\n", (size_t) sector.sectors, global.name.c_str());
		
		assert(global.asset);
		fs::file_time_type modified_time;
		pack_asset(iso, &global.header, &modified_time, *global.asset, game, 0);
		
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

static std::array<IsoDirectory, 3> pack_levels(OutputStream& iso, std::vector<LevelInfo>& levels, const AssetPackerFunc& pack_asset, Game game, s32 single_level_index) {
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
			if(level.level) pack_level_wad(iso, levels_dir, *level.level, pack_asset, "level", game, i);
			if(level.audio) pack_level_wad(iso, audio_dir, *level.audio, pack_asset, "audio", game, i);
			if(level.scene) pack_level_wad(iso, scenes_dir, *level.scene, pack_asset, "scene", game, i);
		}
	} else {
		// The level files are laid out SoA, audio files first.
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.audio) pack_level_wad(iso, audio_dir, *level.audio, pack_asset, "audio", game, i);
		}
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.level) pack_level_wad(iso, levels_dir, *level.level, pack_asset, "level", game, i);
		}
		for(size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if(level.scene) pack_level_wad(iso, scenes_dir, *level.scene, pack_asset, "scene", game, i);
		}
	}
	return {levels_dir, audio_dir, scenes_dir};
}

static void pack_level_wad(OutputStream& iso, IsoDirectory& directory, LevelWadInfo& wad, const AssetPackerFunc& pack_asset, const char* name, Game game, s32 index) {
	std::string file_name = stringf("%s%02d.wad", name, index);
	
	iso.pad(SECTOR_SIZE, 0);
	Sector32 sector = Sector32::size_from_bytes(iso.tell());
	
	printf("%-16d                %s\n", (size_t) sector.sectors, file_name.c_str());
	
	assert(wad.asset);
	fs::file_time_type modified_time;
	pack_asset(iso, &wad.header, &modified_time, *wad.asset, game, 0);
	
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
