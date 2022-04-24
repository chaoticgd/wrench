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

#include <core/png.h>
#include <iso/iso_filesystem.h>
#include <iso/wad_identifier.h>

enum class Region {
	NTSC, PAL
};

struct UnpackInfo {
	Asset* asset;
	ByteRange64 range;
};

static void unpack_ps2_logo(BuildAsset& build, InputStream& src, Region region);
static void unpack_primary_volume_descriptor(BuildAsset& build, const IsoPrimaryVolumeDescriptor& pvd);
static void enumerate_global_wads(std::vector<UnpackInfo>& dest, BuildAsset& build, const table_of_contents& toc, InputStream& src);
static void enumerate_level_wads(std::vector<UnpackInfo>& dest, CollectionAsset& levels, const table_of_contents& toc, InputStream& src);
static void enumerate_non_wads(std::vector<UnpackInfo>& dest, CollectionAsset& files, fs::path out, const IsoDirectory& dir, InputStream& src);
static std::tuple<Game, Region, const char*> identify_game(const IsoDirectory& root);
static size_t get_global_wad_file_size(const GlobalWadInfo& global, const table_of_contents& toc);

void unpack_iso(BuildAsset& dest, InputStream& src, AssetUnpackerFunc unpack) {
	IsoFilesystem filesystem = read_iso_filesystem(src);
	auto [game, region, game_tag] = identify_game(filesystem.root);
	table_of_contents toc = read_table_of_contents(src, game);
	
	dest.set_game(static_cast<s32>(game));
	
	std::vector<UnpackInfo> files;
	
	unpack_ps2_logo(dest, src, region);
	unpack_primary_volume_descriptor(dest, filesystem.pvd);
	
	enumerate_global_wads(files, dest.switch_files("globals/globals.asset"), toc, src);
	enumerate_level_wads(files, dest.switch_files("levels/levels.asset").levels(), toc, src);
	enumerate_non_wads(files, dest.switch_files("files/files.asset").files(), "", filesystem.root, src);
	
	std::sort(BEGIN_END(files), [](auto& lhs, auto& rhs)
		{ return lhs.range.offset < rhs.range.offset; });
	
	for(UnpackInfo& info : files) {
		SubInputStream stream(src, info.range);
		unpack(*info.asset, stream, game, FMT_NO_HINT);
	}
}

static void unpack_ps2_logo(BuildAsset& build, InputStream& src, Region region) {
	//std::vector<u8> logo = read_file(iso, 0, 12 * SECTOR_SIZE);
	//
	//u8 key = logo[0];
	//for(u8& pixel : logo) {
	//	pixel ^= key;
	//	pixel = (pixel << 3) | (pixel >> 5);
	//}
	//
	//s32 width, height;
	//if(region == Region::PAL) {
	//	width = 344;
	//	height = 71;
	//} else {
	//	width = 384;
	//	height = 64;
	//}
	//
	//logo.resize(width * height);
	//
	//FileReference ref = build.file().write_binary_file("ps2_logo.png", [&](FILE* file) {
	//	write_grayscale_png(file, "PS2 logo", logo, width, height);
	//});
	//
	//TextureAsset& texture = build.physical_child<TextureAsset>("ps2_logo");
	//texture.set_src(ref);
	//
	//build.set_ps2_logo(texture);
	//build.set_ps2_logo_key(key);
}

static void unpack_primary_volume_descriptor(BuildAsset& build, const IsoPrimaryVolumeDescriptor& pvd) {
	PrimaryVolumeDescriptorAsset& asset = build.switch_files("primary_volume_descriptor").primary_volume_descriptor();
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

static void enumerate_global_wads(std::vector<UnpackInfo>& dest, BuildAsset& build, const table_of_contents& toc, InputStream& src) {
	for(const GlobalWadInfo& global : toc.globals) {
		auto [wad_game, wad_type, name] = identify_wad(global.header);
		std::string file_name = std::string(name) + ".wad";
		size_t file_size = get_global_wad_file_size(global, toc);
		
		Asset* asset;
		switch(wad_type) {
			case WadType::MPEG:   asset = &build.mpeg<MpegWadAsset>();     break;
			case WadType::MISC:   asset = &build.misc<MiscWadAsset>();     break;
			case WadType::HUD:    asset = &build.hud<HudWadAsset>();       break;
			case WadType::BONUS:  asset = &build.bonus<BonusWadAsset>();   break;
			case WadType::AUDIO:  asset = &build.audio<AudioWadAsset>();   break;
			case WadType::SPACE:  asset = &build.space<SpaceWadAsset>();   break;
			case WadType::SCENE:  asset = &build.scene<SceneWadAsset>();   break;
			case WadType::GADGET: asset = &build.gadget<GadgetWadAsset>(); break;
			case WadType::ARMOR:  asset = &build.armor<ArmorWadAsset>();   break;
			case WadType::ONLINE: asset = &build.online<OnlineWadAsset>(); break;
			default: fprintf(stderr, "warning: Extracted global WAD of unknown type to globals/%s.wad.\n", name);
		}
		
		dest.emplace_back(UnpackInfo{asset, {global.sector.bytes(), (s64) file_size}});
	}
}

static void enumerate_level_wads(std::vector<UnpackInfo>& dest, CollectionAsset& levels, const table_of_contents& toc, InputStream& src) {
	for(const LevelInfo& level : toc.levels) {
		std::string asset_path = stringf("%02d/level.asset", level.level_table_index);
		LevelAsset& level_asset = levels.switch_files(asset_path).child<LevelAsset>(std::to_string(level.level_table_index).c_str());
		
		for(const Opt<LevelWadInfo>& part : {level.level, level.audio, level.scene}) {
			if(!part) {
				continue;
			}
			
			auto [wad_game, wad_type, name] = identify_wad(part->header);
			
			std::vector<u8> header;
			if(part->prepend_header) {
				// For R&C1.
				header = part->header;
				OutBuffer(header).pad(SECTOR_SIZE, 0);
			}
			
			Asset* asset;
			switch(wad_type) {
				case WadType::LEVEL:       asset = &level_asset.level<LevelWadAsset>(); break;
				case WadType::LEVEL_AUDIO: asset = &level_asset.audio<LevelAudioWadAsset>(); break;
				case WadType::LEVEL_SCENE: asset = &level_asset.scene<LevelSceneWadAsset>(); break;
				default: fprintf(stderr, "warning: Extracted level WAD of unknown type.\n");
			}
			
			dest.emplace_back(UnpackInfo{asset, {part->file_lba.bytes(), part->file_size.bytes()}});
		}
	}
}

static void enumerate_non_wads(std::vector<UnpackInfo>& dest, CollectionAsset& files, fs::path out, const IsoDirectory& dir, InputStream& src) {
	for(const IsoFileRecord& file : dir.files) {
		if(file.name.find(".wad") == std::string::npos) {
			fs::path file_path = out/file.name;
			
			std::string tag = file_path.string();
			for(char& c : tag) {
				if(!isalnum(c)) {
					c = '_';
				}
			}
			
			FileAsset& asset = files.child<FileAsset>(tag.c_str());
			asset.set_path(file_path.string());
			
			dest.emplace_back(UnpackInfo{&asset, {file.lba.bytes(), file.size}});
		}
	}
	for(const IsoDirectory& subdir : dir.subdirs) {
		enumerate_non_wads(dest, files, out/subdir.name, subdir, src);
	}
}

static const std::map<std::string, std::pair<Game, Region>> ELF_FILE_NAMES = {
	{"pbpx_955.16", {Game::RAC1, Region::NTSC}}, // japan original
	{"sced_510.75", {Game::RAC1, Region::PAL}}, // eu demo
	{"sces_509.16", {Game::RAC1, Region::PAL}}, // eu black label/plantinum
	{"scus_971.99", {Game::RAC1, Region::NTSC}}, // us original/greatest hits
	{"scus_972.09", {Game::RAC1, Region::NTSC}}, // us demo 1
	{"scus_972.40", {Game::RAC1, Region::NTSC}}, // us demo 2
	{"scaj_200.52", {Game::RAC2, Region::NTSC}}, // japan original
	{"sces_516.07", {Game::RAC2, Region::PAL}}, // eu original/platinum
	{"scus_972.68", {Game::RAC2, Region::NTSC}}, // us greatest hits
	{"scus_972.68", {Game::RAC2, Region::NTSC}}, // us original
	{"scus_973.22", {Game::RAC2, Region::NTSC}}, // us demo
	{"scus_973.23", {Game::RAC2, Region::NTSC}}, // us retail employees demo
	{"scus_973.74", {Game::RAC2, Region::NTSC}}, // us rac2 + jak demo
	{"pcpx_966.53", {Game::RAC3, Region::NTSC}}, // japan promotional
	{"sced_528.47", {Game::RAC3, Region::PAL}}, // eu demo
	{"sced_528.48", {Game::RAC3, Region::PAL}}, // r&c3 + sly 2 demo
	{"sces_524.56", {Game::RAC3, Region::PAL}}, // eu original/plantinum
	{"scps_150.84", {Game::RAC3, Region::NTSC}}, // japan original
	{"scus_973.53", {Game::RAC3, Region::NTSC}}, // us original
	{"scus_974.11", {Game::RAC3, Region::NTSC}}, // us demo
	{"scus_974.13", {Game::RAC3, Region::NTSC}}, // us beta
	{"tces_524.56", {Game::RAC3, Region::PAL}}, // eu beta trial code
	{"pcpx_980.17", {Game::DL, Region::NTSC}}, // japan demo
	{"sced_536.60", {Game::DL, Region::PAL}}, // jak x glaiator demo
	{"sces_532.85", {Game::DL, Region::PAL}}, // eu original/platinum
	{"scps_150.99", {Game::DL, Region::NTSC}}, // japan special gift package
	{"scps_193.28", {Game::DL, Region::NTSC}}, // japan reprint
	{"scus_974.65", {Game::DL, Region::NTSC}}, // us original
	{"scus_974.85", {Game::DL, Region::NTSC}}, // us demo
	{"scus_974.87", {Game::DL, Region::NTSC}} // us public beta
};

static std::tuple<Game, Region, const char*> identify_game(const IsoDirectory& root) {
	Game game = Game::UNKNOWN;
	Region region = Region::NTSC;
	for(const IsoFileRecord& file : root.files) {
		auto iter = ELF_FILE_NAMES.find(file.name);
		if(iter != ELF_FILE_NAMES.end()) {
			game = iter->second.first;
			region = iter->second.second;
			break;
		}
	}
	const char* tag;
	switch(game) {
		case Game::RAC1: tag = "1"; break;
		case Game::RAC2: tag = "2"; break;
		case Game::RAC3: tag = "3"; break;
		case Game::DL:   tag = "4"; break;
		case Game::UNKNOWN: {
			tag = "error_unknown";
			fprintf(stderr,
				"warning: Failed to identify game. For R&C1 this is a fatal error. "
				"For the other games this will mean you need to manually change "
				"the game tag from Game error_unknown in all .asset files "
				"(to Game 1 for R&C1, Game 4 for Deadlocked/Gladiator, etc).\n");
		}
	}
	return {game, region, tag};
}

static size_t get_global_wad_file_size(const GlobalWadInfo& global, const table_of_contents& toc) {
	// Assume the beginning of the next file after this one is also the end
	// of this file.
	size_t start_of_file = global.sector.bytes();
	size_t end_of_file = SIZE_MAX;
	for(const GlobalWadInfo& other_global : toc.globals) {
		if(other_global.sector.bytes() > start_of_file) {
			end_of_file = std::min(end_of_file, (size_t) other_global.sector.bytes());
		}
	}
	for(const LevelInfo& other_level : toc.levels) {
		for(const Opt<LevelWadInfo>& wad : {other_level.level, other_level.audio, other_level.scene}) {
			if(wad && wad->file_lba.bytes() > start_of_file) {
				end_of_file = std::min(end_of_file, (size_t) wad->file_lba.bytes());
			}
		}
	}
	assert(end_of_file != SIZE_MAX);
	assert(end_of_file >= start_of_file);
	return end_of_file - start_of_file;
}
