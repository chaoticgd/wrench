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

#include <core/png.h>
#include <iso/wad_identifier.h>

static void pack_ps2_logo(
	OutputStream& iso, const BuildAsset& build, BuildConfig config, AssetPackerFunc pack);
static std::vector<GlobalWadInfo> enumerate_globals(const BuildAsset& build, Game game);
static std::vector<LevelInfo> enumerate_levels(
	const BuildAsset& build, Game game, const LevelAsset* single_level);
static LevelInfo enumerate_level(const LevelAsset& level, Game game);
static IsoDirectory enumerate_files(const Asset& files);
static void flatten_files(std::vector<IsoFileRecord*>& dest, IsoDirectory& root_dir);
static IsoFileRecord pack_system_cnf(OutputStream& iso, const BuildAsset& build, Game game);
static IsoFileRecord pack_boot_elf(
	OutputStream& iso, const Asset& boot_elf, BuildConfig config, AssetPackerFunc pack);
static std::string get_boot_elf_path(const Asset& boot_elf);
static void pack_files(
	OutputStream& iso, std::vector<IsoFileRecord*>& files, BuildConfig config, AssetPackerFunc pack);
static IsoDirectory pack_globals(
	OutputStream& iso,
	std::vector<GlobalWadInfo>& globals,
	BuildConfig config,
	AssetPackerFunc pack,
	bool no_mpegs);
static std::array<IsoDirectory, 3> pack_levels(
	OutputStream& iso,
	std::vector<LevelInfo>& levels,
	BuildConfig config,
	const LevelAsset* single_level,
	AssetPackerFunc pack);
static void pack_level_wad_outer(
	OutputStream& iso,
	IsoDirectory& directory,
	LevelWadInfo& wad,
	const char* name,
	BuildConfig config,
	s32 index,
	AssetPackerFunc pack);

void pack_iso(
	OutputStream& iso, const BuildAsset& src, BuildConfig, const char* hint, AssetPackerFunc pack)
{
	BuildConfig config(src.game(), src.region());
	
	std::string single_level_tag;
	bool no_mpegs = false;
	
	// Parse the hint to determine the build configuration.
	const char* type = next_hint(&hint);
	if (strcmp(type, "testlf") == 0) {
		single_level_tag = next_hint(&hint);
		const char* flags = next_hint(&hint);
		for (size_t i = 0; i < strlen(flags); i++) {
			if (i == 0 || flags[i - 1] == '|') {
				if (strcmp(flags + i, "nompegs") == 0) {
					no_mpegs = true;
				}
			}
		}
	} else if (strcmp(type, "release") != 0) {
		verify_not_reached("Invalid hint.");
	}
	
	// If only a single level is being packed, find it.
	const LevelAsset* single_level = nullptr;
	if (!single_level_tag.empty()) {
		single_level = &src.get_levels().get_child(single_level_tag.c_str()).as<LevelAsset>();
	}
	
	pack_ps2_logo(iso, src, config, pack);
	
	table_of_contents toc;
	toc.globals = enumerate_globals(src, config.game());
	toc.levels = enumerate_levels(src, config.game(), single_level);
	
	Sector32 toc_size = calculate_table_of_contents_size(toc, config.game());
	
	// Mustn't modify root_dir until after pack_files is called, or the
	// flattened file pointers will be invalidated.
	IsoDirectory root_dir = enumerate_files(src.get_files());
	std::vector<IsoFileRecord*> files;
	flatten_files(files, root_dir);
	
	u32 system_cnf_lba, table_of_contents_lba;
	if (config.game() == Game::RAC) {
		system_cnf_lba = RAC_SYSTEM_CNF_LBA;
		table_of_contents_lba = RAC_TABLE_OF_CONTENTS_LBA;
	} else {
		system_cnf_lba = GC_UYA_DL_SYSTEM_CNF_LBA;
		table_of_contents_lba = GC_UYA_DL_TABLE_OF_CONTENTS_LBA;
	}
	
	// Write out blank sectors that are to be filled in later.
	iso.pad(SECTOR_SIZE, 0);
	static const u8 null_sector[SECTOR_SIZE] = {0};
	while (iso.tell() < system_cnf_lba * SECTOR_SIZE) {
		iso.write_n(null_sector, SECTOR_SIZE);
	}
	
	// SYSTEM.CNF must be written out at a specific sector (the game hardcodes
	// this and if it's not as it expects the wrong directory will be used on
	// the memory card).
	IsoFileRecord system_cnf_record = pack_system_cnf(iso, src, config.game());
	
	iso.pad(SECTOR_SIZE, 0);
	while (iso.tell() < system_cnf_lba * SECTOR_SIZE) {
		iso.write_n(null_sector, SECTOR_SIZE);
	}
	
	// Then the table of contents (also hardcoded).
	IsoFileRecord toc_record;
	{
		iso.pad(SECTOR_SIZE, 0);
		switch (config.game()) {
			case Game::RAC:     toc_record.name = "rc1.hdr";     break;
			case Game::GC:      toc_record.name = "rc2.hdr";     break;
			case Game::UYA:     toc_record.name = "rc3.hdr";     break;
			case Game::DL:      toc_record.name = "rc4.hdr";     break;
			case Game::UNKNOWN: toc_record.name = "unknown.hdr"; break;
		}
		toc_record.lba = {(s32) table_of_contents_lba};
		toc_record.size = toc_size.bytes();
		toc_record.modified_time = fs::file_time_type::clock::now();
	}
	
	// Write out blank sectors that are to be filled in by the table of contents later.
	iso.pad(SECTOR_SIZE, 0);
	if (config.game() != Game::RAC) {
		verify_fatal(iso.tell() == table_of_contents_lba * SECTOR_SIZE);
	}
	while (iso.tell() < table_of_contents_lba * SECTOR_SIZE + toc_size.bytes()) {
		iso.write_n(null_sector, SECTOR_SIZE);
	}
	
	s64 files_begin = iso.tell();
	
	IsoFileRecord elf_record = pack_boot_elf(iso, src.get_boot_elf(), config, pack);
	pack_files(iso, files, config, pack);
	
	root_dir.files.emplace(root_dir.files.begin(), std::move(elf_record));
	root_dir.files.emplace(root_dir.files.begin(), std::move(toc_record));
	root_dir.files.emplace(root_dir.files.begin(), std::move(system_cnf_record));
	
	root_dir.subdirs.emplace_back(pack_globals(iso, toc.globals, config, pack, no_mpegs));
	auto [levels_dir, audio_dir, scenes_dir] =
		pack_levels(iso, toc.levels, config, single_level, pack);
	root_dir.subdirs.emplace_back(std::move(levels_dir));
	root_dir.subdirs.emplace_back(std::move(audio_dir));
	root_dir.subdirs.emplace_back(std::move(scenes_dir));
	
	iso.pad(SECTOR_SIZE, 0);
	s64 volume_size = iso.tell() / SECTOR_SIZE;
	
	iso.seek(0);
	write_iso_filesystem(iso, &root_dir);
	verify_fatal(iso.tell() <= table_of_contents_lba * SECTOR_SIZE);
	iso.write<IsoLsbMsb32>(0x8050, IsoLsbMsb32::from_scalar(volume_size));
	
	s64 toc_end = write_table_of_contents(iso, toc, config.game());
	verify_fatal(toc_end <= files_begin);
}

static void pack_ps2_logo(
	OutputStream& iso, const BuildAsset& build, BuildConfig config, AssetPackerFunc pack)
{
	const TextureAsset* asset;
	if (config.is_ntsc()) {
		if (!build.has_ps2_logo_ntsc()) {
			return;
		}
		asset = &build.get_ps2_logo_ntsc();
	} else {
		if (!build.has_ps2_logo_pal()) {
			return;
		}
		asset = &build.get_ps2_logo_pal();
	}
	auto png = asset->src().open_binary_file_for_reading();
	
	Opt<Texture> texture = read_png(*png);
	verify(texture.has_value(), "Build has bad ps2_logo.");
	
	texture->to_grayscale();
	verify(texture->data.size() <= 12 * SECTOR_SIZE, "PS2 logo image too big.");
	
	u8 key = build.has_ps2_logo_key() ? build.ps2_logo_key() : 0;
	for (u8& pixel : texture->data) {
		pixel = ((pixel >> 3) | (pixel << 5)) ^ key;
	}
	
	iso.write_v(texture->data);
}

static std::vector<GlobalWadInfo> enumerate_globals(const BuildAsset& build, Game game)
{
	std::vector<WadType> order;
	switch (game) {
		case Game::RAC: {
			order = {
				WadType::GLOBAL
			};
			break;
		}
		case Game::GC: {
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
		case Game::UYA: {
			order = {
				WadType::MPEG,
				WadType::MISC,
				WadType::BONUS,
				WadType::SPACE,
				WadType::ARMOR,
				WadType::AUDIO,
				WadType::GADGET,
				WadType::HUD
			};
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
		default: verify_fatal("Invalid game.");
	}
	
	std::vector<GlobalWadInfo> globals;
	for (WadType type : order) {
		GlobalWadInfo global;
		switch (type) {
			case WadType::ARMOR:  global.asset = &build.get_armor();  global.name = "armor.wad";  break;
			case WadType::AUDIO:  global.asset = &build.get_audio();  global.name = "audio.wad";  break;
			case WadType::BONUS:  global.asset = &build.get_bonus();  global.name = "bonus.wad";  break;
			case WadType::GADGET: global.asset = &build.get_gadget(); global.name = "gadget.wad"; break;
			case WadType::GLOBAL: global.asset = &build.get_global(); global.name = "global.wad"; break;
			case WadType::HUD:    global.asset = &build.get_hud();    global.name = "hud.wad";    break;
			case WadType::MISC:   global.asset = &build.get_misc();   global.name = "misc.wad";   break;
			case WadType::MPEG:   global.asset = &build.get_mpeg();   global.name = "mpeg.wad";   break;
			case WadType::ONLINE: global.asset = &build.get_online(); global.name = "online.wad"; break;
			case WadType::SCENE:  global.asset = &build.get_scene();  global.name = "scene.wad";  break;
			case WadType::SPACE:  global.asset = &build.get_space();  global.name = "space.wad";  break;
			default: verify_fatal(0);
		}
		global.header.resize(header_size_of_wad(game, type));
		verify(global.asset, "Failed to build ISO, missing global WAD asset.");
		globals.emplace_back(std::move(global));
	}
	return globals;
}

static std::vector<LevelInfo> enumerate_levels(
	const BuildAsset& build, Game game, const LevelAsset* single_level)
{
	std::vector<LevelInfo> levels;
	
	if (single_level) {
		LevelInfo info = enumerate_level(*single_level, game);
		
		// Construct a list of all the levels that exist so they can be filled in later.
		build.get_levels().for_each_logical_child_of_type<LevelAsset>([&](const LevelAsset& level) {
			s32 level_table_index = level.index();
			if (((s32) levels.size()) <= level_table_index) {
				levels.resize(level_table_index + 1);
			}
			levels[level_table_index] = info;
		});
		
		verify(levels.size() >= 1, "No levels with a valid index.");
	} else {
		build.get_levels().for_each_logical_child_of_type<LevelAsset>([&](const LevelAsset& level) {
			s32 level_table_index = level.index();
			if (((s32) levels.size()) <= level_table_index) {
				levels.resize(level_table_index + 1);
			}
			levels[level_table_index] = enumerate_level(level, game);
		});
		
		verify(levels.size() >= 1, "No levels with a valid index.");
	}
	
	return levels;
}

static LevelInfo enumerate_level(const LevelAsset& level, Game game)
{
	LevelInfo info;
	info.level_table_index = level.index();
	
	if (level.has_level()) {
		LevelWadInfo& wad = info.level.emplace();
		wad.header.resize(header_size_of_wad(game, WadType::LEVEL));
		wad.asset = &level.get_level();
	}
	if (level.has_audio()) {
		LevelWadInfo& wad = info.audio.emplace();
		wad.header.resize(header_size_of_wad(game, WadType::LEVEL_AUDIO));
		wad.asset = &level.get_audio();
	}
	if (level.has_scene()) {
		LevelWadInfo& wad = info.scene.emplace();
		wad.header.resize(header_size_of_wad(game, WadType::LEVEL_SCENE));
		wad.asset = &level.get_scene();
	}
	
	return info;
}

static IsoDirectory enumerate_files(const Asset& files)
{
	IsoDirectory root;
	
	files.for_each_logical_child_of_type<FileAsset>([&](const FileAsset& file) {
		IsoDirectory* current_dir = &root;
		std::string dvd_path = file.path();
		for (;;) {
			size_t index = dvd_path.find_first_of('/');
			if (index == std::string::npos) {
				break;
			}
			
			std::string dir_name = dvd_path.substr(0, index);
			dvd_path = dvd_path.substr(index + 1);
			
			bool found = false;
			for (IsoDirectory& subdir : current_dir->subdirs) {
				if (subdir.name == dir_name) {
					current_dir = &subdir;
					found = true;
					break;
				}
			}
			
			if (!found) {
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

static void flatten_files(std::vector<IsoFileRecord*>& dest, IsoDirectory& root_dir)
{
	for (IsoFileRecord& file : root_dir.files) {
		dest.push_back(&file);
	}
	for (IsoDirectory& dir : root_dir.subdirs) {
		flatten_files(dest, dir);
	}
}

static IsoFileRecord pack_system_cnf(OutputStream& iso, const BuildAsset& build, Game game)
{
	std::string path = get_boot_elf_path(build.get_boot_elf());
	for (char& c : path) c = toupper(c);
	
	std::string system_cnf;
	system_cnf += "BOOT2 = cdrom0:\\";
	system_cnf += path;
	system_cnf += ";1";
	if (game != Game::RAC) {
		system_cnf += " ";
	}
	system_cnf += "\r\nVER = ";
	system_cnf += build.version();
	if (game != Game::RAC) {
		system_cnf += " ";
	}
	system_cnf += "\r\nVMODE = ";
	if (build.region() != "eu") {
		system_cnf += "NTSC";
	} else {
		system_cnf += "PAL";
	}
	if (game != Game::RAC) {
		system_cnf += " ";
	}
	system_cnf += "\r\n";
	if (game == Game::RAC) {
		system_cnf += "\r\n";
	}
	
	iso.pad(SECTOR_SIZE, 0);
	
	IsoFileRecord record;
	record.name = "system.cnf";
	record.lba = {(s32) (iso.tell() / SECTOR_SIZE)};
	record.size = system_cnf.size();
	record.modified_time = fs::file_time_type::clock::now();
	
	iso.write_n((u8*) system_cnf.data(), system_cnf.size());
	
	return record;
}

static IsoFileRecord pack_boot_elf(
	OutputStream& iso, const Asset& boot_elf, BuildConfig config, AssetPackerFunc pack)
{
	IsoFileRecord record;
	record.name = get_boot_elf_path(boot_elf);
	
	iso.pad(SECTOR_SIZE, 0);
	record.lba = {(s32) (iso.tell() / SECTOR_SIZE)};
	const char* hint = (config.game() == Game::UYA || config.game() == Game::DL) ?
		FMT_ELFFILE_PACKED : FMT_NO_HINT;
	pack(iso, nullptr, &record.modified_time, boot_elf, config, hint);
	
	s64 end_of_file = iso.tell();
	record.size = (u32) (end_of_file - record.lba.bytes());
	
	return record;
}

static std::string get_boot_elf_path(const Asset& boot_elf) {
	if (boot_elf.logical_type() == ElfFileAsset::ASSET_TYPE) {
		return boot_elf.as<ElfFileAsset>().name();
	} else if (boot_elf.logical_type() == FileAsset::ASSET_TYPE) {
		return boot_elf.as<FileAsset>().path();
	} else {
		verify_not_reached("The boot_elf asset is of an invalid type.");
	}
}

static void pack_files(
	OutputStream& iso, std::vector<IsoFileRecord*>& files, BuildConfig config, AssetPackerFunc pack)
{
	for (IsoFileRecord* file : files) {
		if (file->name.find(".hdr") != std::string::npos) {
			// We're writing out a new table of contents, so if an old one
			// already exists we don't want to write it out.
			continue;
		}
		
		iso.pad(SECTOR_SIZE, 0);
		file->lba = {(s32) (iso.tell() / SECTOR_SIZE)};
		pack(iso, nullptr, &file->modified_time, *file->asset, config, FMT_NO_HINT);
		
		s64 end_of_file = iso.tell();
		file->size = (u32) (end_of_file - file->lba.bytes());
	}
}

static IsoDirectory pack_globals(
	OutputStream& iso,
	std::vector<GlobalWadInfo>& globals,
	BuildConfig config,
	AssetPackerFunc pack,
	bool no_mpegs)
{
	IsoDirectory globals_dir {"globals"};
	for (GlobalWadInfo& global : globals) {
		iso.pad(SECTOR_SIZE, 0);
		Sector32 sector = Sector32::size_from_bytes(iso.tell());
		
		const char* hint = FMT_NO_HINT;
		if (no_mpegs) {
			if (global.asset->logical_type() == GlobalWadAsset::ASSET_TYPE) {
				hint = FMT_GLOBALWAD_NOMPEGS;
			}
			if (global.asset->logical_type() == MpegWadAsset::ASSET_TYPE) {
				hint = FMT_MPEGWAD_NOMPEGS;
			}
		}
		
		verify_fatal(global.asset);
		fs::file_time_type modified_time;
		pack(iso, &global.header, &modified_time, *global.asset, config, hint);
		
		s64 end_of_file = iso.tell();
		s64 file_size = end_of_file - sector.bytes();
		
		global.index = 0; // Don't care.
		global.offset_in_toc = 0; // Don't care.
		global.sector = sector;
		
		IsoFileRecord record;
		record.name = global.name;
		record.lba = sector;
		verify_fatal(file_size < UINT32_MAX);
		record.size = file_size;
		record.modified_time = std::move(modified_time);
		globals_dir.files.push_back(record);
	}
	return globals_dir;
}

static std::array<IsoDirectory, 3> pack_levels(
	OutputStream& iso,
	std::vector<LevelInfo>& levels,
	BuildConfig config,
	const LevelAsset* single_level,
	AssetPackerFunc pack)
{
	// Create directories for the level files.
	IsoDirectory levels_dir {"levels"};
	IsoDirectory audio_dir {"audio"};
	IsoDirectory scenes_dir {"scenes"};
	if (single_level) {
		// Only write out a single level, and point every level at it.
		LevelInfo level = enumerate_level(*single_level, config.game());
		if (level.level) pack_level_wad_outer(iso, levels_dir, *level.level, "level", config, 0, pack);
		if (level.audio) pack_level_wad_outer(iso, audio_dir, *level.audio, "audio", config, 0, pack);
		if (level.scene) pack_level_wad_outer(iso, scenes_dir, *level.scene, "scene", config, 0, pack);
		
		for (size_t i = 0; i < levels.size(); i++) {
			// Preserve empty spaces in the level table.
			if (levels[i].level.has_value() || levels[i].audio.has_value() || levels[i].scene.has_value()) {
				levels[i].level = level.level;
				levels[i].audio = level.audio;
				levels[i].scene = level.scene;
			}
		}
	} else if (config.game() == Game::GC) {
		// The level files are laid out AoS.
		for (size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if (level.level) pack_level_wad_outer(iso, levels_dir, *level.level, "level", config, i, pack);
			if (level.audio) pack_level_wad_outer(iso, audio_dir, *level.audio, "audio", config, i, pack);
			if (level.scene) pack_level_wad_outer(iso, scenes_dir, *level.scene, "scene", config, i, pack);
		}
	} else {
		// The level files are laid out SoA, audio files first.
		for (size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if (level.audio) {
				pack_level_wad_outer(iso, audio_dir, *level.audio, "audio", config, i, pack);
			}
		}
		for (size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if (level.level) {
				pack_level_wad_outer(iso, levels_dir, *level.level, "level", config, i, pack);
			}
		}
		for (size_t i = 0; i < levels.size(); i++) {
			LevelInfo& level = levels[i];
			if (level.scene) {
				pack_level_wad_outer(iso, scenes_dir, *level.scene, "scene", config, i, pack);
			}
		}
	}
	return {levels_dir, audio_dir, scenes_dir};
}

static void pack_level_wad_outer(
	OutputStream& iso,
	IsoDirectory& directory,
	LevelWadInfo& wad,
	const char* name,
	BuildConfig config,
	s32 index,
	AssetPackerFunc pack)
{
	std::string file_name = stringf("%s%02d.wad", name, index);
	
	iso.pad(SECTOR_SIZE, 0);
	Sector32 sector = Sector32::size_from_bytes(iso.tell());
	
	verify_fatal(wad.asset);
	fs::file_time_type modified_time;
	pack(iso, &wad.header, &modified_time, *wad.asset, config, FMT_NO_HINT);
	
	s64 end_of_file = iso.tell();
	s64 file_size = end_of_file - sector.bytes();
	
	wad.header_lba = {0}; // Don't care.
	wad.file_size = Sector32::size_from_bytes(file_size);
	wad.file_lba = sector;
	
	IsoFileRecord record;
	record.name = file_name.c_str();
	record.lba = sector;
	verify_fatal(file_size < UINT32_MAX);
	record.size = file_size;
	record.modified_time = std::move(modified_time);
	directory.files.push_back(record);
}
