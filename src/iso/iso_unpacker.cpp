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

#include "iso_unpacker.h"

#include <algorithm>

#include <core/png.h>
#include <assetmgr/asset_path_gen.h>
#include <iso/iso_filesystem.h>
#include <iso/wad_identifier.h>

struct UnpackInfo
{
	Asset* asset = nullptr;
	const std::vector<u8>* header = nullptr;
	ByteRange64 data_range = {-1, -1};
	const char* hint = FMT_NO_HINT;
};

static void add_missing_levels_from_filesystem(table_of_contents& toc, const IsoDirectory& dir, InputStream& iso);
static void unpack_ps2_logo(BuildAsset& build, InputStream& src, BuildConfig config);
static void unpack_primary_volume_descriptor(BuildAsset& build, const IsoPrimaryVolumeDescriptor& pvd);
static void enumerate_global_wads(std::vector<UnpackInfo>& dest, BuildAsset& build, const table_of_contents& toc, InputStream& src, Game game);
static std::string parse_system_cnf(BuildAsset& build, const std::string& src, const IsoDirectory& root);
static void enumerate_level_wads(std::vector<UnpackInfo>& dest, CollectionAsset& levels, const table_of_contents& toc, InputStream& src);
static void enumerate_extra_files(std::vector<UnpackInfo>& dest, CollectionAsset& files, fs::path out, const IsoDirectory& dir, InputStream& src, const std::string& boot_elf);
static size_t get_global_wad_file_size(const GlobalWadInfo& global, const table_of_contents& toc);

void unpack_iso(BuildAsset& dest, InputStream& src, BuildConfig config, AssetUnpackerFunc unpack)
{
	dest.set_game(game_to_string(config.game()));
	dest.set_region(region_to_string(config.region()));
	
	IsoFilesystem filesystem = read_iso_filesystem(src);
	table_of_contents toc = read_table_of_contents(src, config.game());
	add_missing_levels_from_filesystem(toc, filesystem.root, src);
	
	std::vector<UnpackInfo> files;
	
	unpack_ps2_logo(dest, src, config);
	unpack_primary_volume_descriptor(dest, filesystem.pvd);
	
	std::string boot_elf;
	for (IsoFileRecord& record : filesystem.root.files) {
		if (record.name == "system.cnf") {
			std::vector<u8> bytes = src.read_multiple<u8>(record.lba.bytes(), record.size);
			std::string system_cnf(bytes.data(), bytes.data() + bytes.size());
			boot_elf = parse_system_cnf(dest, system_cnf, filesystem.root);
		}
	}
	verify(!boot_elf.empty(), "Failed to find SYSTEM.CNF file.");
	
	bool boot_elf_found = false;
	for (const IsoFileRecord& record : filesystem.root.files) {
		if (record.name == boot_elf) {
			ElfFileAsset& asset = dest.boot_elf<ElfFileAsset>();
			asset.set_name(boot_elf);
			UnpackInfo& info = files.emplace_back();
			info.asset = &asset;
			info.data_range = {record.lba.bytes(), record.size};
			if (config.game() == Game::UYA || config.game() == Game::DL) {
				info.hint = FMT_ELFFILE_PACKED;
			} else {
				info.hint = FMT_NO_HINT;
			}
			boot_elf_found = true;
			break;
		}
	}
	verify(boot_elf_found, "Failed to find boot ELF '%s'.", boot_elf.c_str());
	
	enumerate_global_wads(files, dest, toc, src, config.game());
	enumerate_level_wads(files, dest.levels(SWITCH_FILES), toc, src);
	enumerate_extra_files(files, dest.files(SWITCH_FILES), "", filesystem.root, src, boot_elf);
	
	// The reported completion percentage is based on how far through the file
	// we are, so it's important to unpack them in order.
	std::sort(BEGIN_END(files), [](auto& lhs, auto& rhs)
		{ return lhs.data_range.offset < rhs.data_range.offset; });
	
	for (UnpackInfo& info : files) {
		if (info.asset != nullptr) {
			if (info.data_range.size == -1) {
				unpack(*info.asset, src, info.header, config, info.hint);
			} else {
				SubInputStream stream(src, info.data_range);
				unpack(*info.asset, stream, info.header, config, info.hint);
			}
		}
	}
}

static void add_missing_levels_from_filesystem(
	table_of_contents& toc, const IsoDirectory& dir, InputStream& iso)
{
	// Some builds have levels not referenced by the toc. Try to find these.
	
	for (const IsoFileRecord& record : dir.files) {
		if (record.name.starts_with("level") && record.name.ends_with(".wad")) {
			std::string index_str = record.name.substr(5, record.name.size() - 9);
			s32 index = atoi(index_str.c_str());
			
			if (toc.levels.size() > index) {
				LevelInfo& info = toc.levels[index];
				if (!info.level.has_value()) {
					info.level.emplace();
					info.level->header_lba = record.lba;
					info.level->file_lba = record.lba;
					info.level->file_size = Sector32::size_from_bytes(record.size);
					s32 header_size = iso.read<s32>(record.lba.bytes());
					info.level->header = iso.read_multiple<u8>(record.lba.bytes(), header_size);
					info.level->prepend_header = false;
				}
			}
		}
	}
	
	for (const IsoDirectory& subdir : dir.subdirs) {
		add_missing_levels_from_filesystem(toc, subdir, iso);
	}
}

static void unpack_ps2_logo(BuildAsset& build, InputStream& src, BuildConfig config)
{
	src.seek(0);
	std::vector<u8> logo = src.read_multiple<u8>(12 * SECTOR_SIZE);
	
	u8 key = logo[0];
	build.set_ps2_logo_key(key);
	
	for (u8& pixel : logo) {
		pixel ^= key;
		pixel = (pixel << 3) | (pixel >> 5);
	}
	
	s32 width, height;
	if (config.is_ntsc()) {
		width = 384;
		height = 64;
	} else {
		width = 344;
		height = 71;
	}
	
	logo.resize(width * height);
	
	Texture texture = Texture::create_grayscale(width, height, logo);
	auto [file, ref] = build.file().open_binary_file_for_writing("ps2_logo.png");
	write_png(*file, texture);
	
	if (config.is_ntsc()) {
		build.ps2_logo_ntsc().set_src(ref);
	} else {
		build.ps2_logo_pal().set_src(ref);
	}
}

static void unpack_primary_volume_descriptor(BuildAsset& build, const IsoPrimaryVolumeDescriptor& pvd)
{
	PrimaryVolumeDescriptorAsset& asset = build.primary_volume_descriptor();
	asset.set_system_identifier(std::string(pvd.system_identifier, sizeof(pvd.system_identifier)));
	asset.set_volume_identifier(std::string(pvd.volume_identifier, sizeof(pvd.volume_identifier)));
	asset.set_volume_set_identifier(std::string(pvd.volume_set_identifier, sizeof(pvd.volume_set_identifier)));
	asset.set_publisher_identifier(std::string(pvd.publisher_identifier, sizeof(pvd.publisher_identifier)));
	asset.set_data_preparer_identifier(std::string(pvd.data_preparer_identifier, sizeof(pvd.data_preparer_identifier)));
	asset.set_application_identifier(std::string(pvd.application_identifier, sizeof(pvd.application_identifier)));
	asset.set_copyright_file_identifier(std::string(pvd.copyright_file_identifier, sizeof(pvd.copyright_file_identifier)));
	asset.set_abstract_file_identifier(std::string(pvd.abstract_file_identifier, sizeof(pvd.abstract_file_identifier)));
	asset.set_bibliographic_file_identifier(std::string(pvd.bibliographic_file_identifier, sizeof(pvd.bibliographic_file_identifier)));
}

static std::string parse_system_cnf(BuildAsset& build, const std::string& src, const IsoDirectory& root) {
	std::string boot_elf;
	
	size_t boot2_pos = src.find("BOOT2 = cdrom0:\\");
	verify(boot2_pos != std::string::npos, "Failed to parse SYSTEM.CNF: Missing BOOT2 parameter.");
	boot2_pos += 16;
	
	size_t boot2_end = src.size();
	for (size_t i = boot2_pos; i < src.size(); i++) {
		if (src[i] == ';' || src[i] == '\r') {
			boot2_end = i;
			break;
		}
	}
	
	boot_elf = src.substr(boot2_pos, boot2_end - boot2_pos);
	for (char& c : boot_elf) c = tolower(c);
	verify(!boot_elf.empty(), "Failed to parse SYSTEM.CFN: Invalid boot path.");
	
	size_t ver_pos = src.find("VER = ");
	verify(ver_pos != std::string::npos, "Failed to parse SYSTEM.CNF: Missing VER parameter.");
	ver_pos += 6;
	
	size_t ver_end = src.size();
	for (size_t i = ver_pos; i < src.size(); i++) {
		if (src[i] == ' ' || src[i] == '\r') {
			ver_end = i;
			break;
		}
	}
	
	std::string version = src.substr(ver_pos, ver_end - ver_pos);
	verify(!version.empty(), "Failed to parse SYSTEM.CNF: Invalid version.");
	
	build.set_version(version);
	
	return boot_elf;
}

static void enumerate_global_wads(
	std::vector<UnpackInfo>& dest,
	BuildAsset& build,
	const table_of_contents& toc,
	InputStream& src,
	Game game)
{
	for (const GlobalWadInfo& global : toc.globals) {
		auto [wad_game, wad_type, name] = identify_wad(global.header);
		std::string file_name = std::string(name) + ".wad";
		size_t file_size = get_global_wad_file_size(global, toc);
		
		Asset* asset = nullptr;
		switch (wad_type) {
			case WadType::GLOBAL: asset = &build.global<GlobalWadAsset>("globals/global");        break;
			case WadType::MPEG:   asset = &build.mpeg<MpegWadAsset>("globals/mpeg/mpeg");         break;
			case WadType::MISC:   asset = &build.misc<MiscWadAsset>("globals/misc/misc");         break;
			case WadType::HUD:    asset = &build.hud<HudWadAsset>("globals/hud/hud");             break;
			case WadType::BONUS:  asset = &build.bonus<BonusWadAsset>("globals/bonus/bonus");     break;
			case WadType::AUDIO:  asset = &build.audio<AudioWadAsset>("globals/audio/audio");     break;
			case WadType::SPACE:  asset = &build.space<SpaceWadAsset>("globals/space/space");     break;
			case WadType::SCENE:  asset = &build.scene<SceneWadAsset>("globals/scene/scene");     break;
			case WadType::GADGET: asset = &build.gadget<GadgetWadAsset>("globals/gadget/gadget"); break;
			case WadType::ARMOR:  asset = &build.armor<ArmorWadAsset>("globals/armor/armor");     break;
			case WadType::ONLINE: asset = &build.online<OnlineWadAsset>("globals/online/online"); break;
			default: fprintf(stderr, "warning: Extracted global WAD of unknown type to globals/%s.wad.\n", name);
		}
		
		if (game == Game::RAC) {
			dest.emplace_back(UnpackInfo{asset, &global.header, {0, -1}});
		} else {
			dest.emplace_back(UnpackInfo{asset, &global.header, {global.sector.bytes(), (s64) file_size}});
		}
	}
}

static void enumerate_level_wads(
	std::vector<UnpackInfo>& dest,
	CollectionAsset& levels,
	const table_of_contents& toc,
	InputStream& src)
{
	for (s32 i = 0; i < (s32) toc.levels.size(); i++) {
		const LevelInfo& level = toc.levels[i];
		
		if (level.level.has_value()) {
			verify_fatal(level.level->header.size() >= 0xc);
			s32 id = *(s32*) &level.level->header[8];
			
			std::string path = generate_level_asset_path(id, levels);
			LevelAsset& level_asset = levels.foreign_child<LevelAsset>(path, true, id);
			level_asset.set_index(i);
			
			if (level.level.has_value()) {
				const LevelWadInfo& part = *level.level;
				LevelWadAsset& level_wad = level_asset.level<LevelWadAsset>();
				level_wad.set_id(id);
				ByteRange64 range{part.file_lba.bytes(), part.file_size.bytes()};
				dest.emplace_back(UnpackInfo{&level_wad, &part.header, range});
			}
			
			if (level.audio.has_value()) {
				const LevelWadInfo& part = *level.audio;
				LevelAudioWadAsset& audio_wad = level_asset.audio<LevelAudioWadAsset>();
				ByteRange64 range{part.file_lba.bytes(), part.file_size.bytes()};
				dest.emplace_back(UnpackInfo{&audio_wad, &part.header, range});
			}
			
			if (level.scene.has_value()) {
				const LevelWadInfo& part = *level.scene;
				LevelSceneWadAsset& scene_wad = level_asset.scene<LevelSceneWadAsset>();
				ByteRange64 range{part.file_lba.bytes(), part.file_size.bytes()};
				dest.emplace_back(UnpackInfo{&scene_wad, &part.header, range});
			}
		}
	}
}

static void enumerate_extra_files(
	std::vector<UnpackInfo>& dest,
	CollectionAsset& files,
	fs::path out,
	const IsoDirectory& dir,
	InputStream& src,
	const std::string& boot_elf)
{
	for (const IsoFileRecord& file : dir.files) {
		bool include = true;
		include &= boot_elf.empty() || file.name != "system.cnf";
		include &= boot_elf.empty() || file.name.find(boot_elf) == std::string::npos;
		include &= boot_elf.empty() || file.name != "rc2.hdr"; // GC
		include &= file.name.find(".wad") == std::string::npos; // GC
		include &= file.name != "dummy."; // GC
		if (include) {
			fs::path file_path = out/file.name;
			
			std::string tag = file_path.string();
			for (char& c : tag) {
				if (!isalnum(c)) {
					c = '_';
				}
			}
			
			FileAsset& asset = files.child<FileAsset>(tag.c_str());
			asset.set_path(file_path.string());
			
			dest.emplace_back(UnpackInfo{&asset, 0, {file.lba.bytes(), file.size}});
		}
	}
	for (const IsoDirectory& subdir : dir.subdirs) {
		std::string empty;
		enumerate_extra_files(dest, files, out/subdir.name, subdir, src, empty);
	}
}

static size_t get_global_wad_file_size(const GlobalWadInfo& global, const table_of_contents& toc)
{
	// Assume the beginning of the next file after this one is also the end
	// of this file.
	size_t start_of_file = global.sector.bytes();
	size_t end_of_file = SIZE_MAX;
	for (const GlobalWadInfo& other_global : toc.globals) {
		if (other_global.sector.bytes() > start_of_file) {
			end_of_file = std::min(end_of_file, (size_t) other_global.sector.bytes());
		}
	}
	for (const LevelInfo& other_level : toc.levels) {
		for (const Opt<LevelWadInfo>& wad : {other_level.level, other_level.audio, other_level.scene}) {
			if (wad && wad->file_lba.bytes() > start_of_file) {
				end_of_file = std::min(end_of_file, (size_t) wad->file_lba.bytes());
			}
		}
	}
	verify_fatal(end_of_file != SIZE_MAX);
	verify_fatal(end_of_file >= start_of_file);
	return end_of_file - start_of_file;
}
