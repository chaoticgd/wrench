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

struct UnpackInfo {
	Asset* asset;
	s64 header_offset;
	ByteRange64 data_range;
};

static void unpack_ps2_logo(BuildAsset& build, InputStream& src, BuildConfig config);
static void unpack_primary_volume_descriptor(BuildAsset& build, const IsoPrimaryVolumeDescriptor& pvd);
static void enumerate_global_wads(std::vector<UnpackInfo>& dest, BuildAsset& build, const table_of_contents& toc, InputStream& src, Game game);
static void enumerate_level_wads(std::vector<UnpackInfo>& dest, CollectionAsset& levels, const table_of_contents& toc, InputStream& src);
static void enumerate_non_wads(std::vector<UnpackInfo>& dest, CollectionAsset& files, fs::path out, const IsoDirectory& dir, InputStream& src);
static size_t get_global_wad_file_size(const GlobalWadInfo& global, const table_of_contents& toc);

void unpack_iso(BuildAsset& dest, InputStream& src, BuildConfig config, AssetUnpackerFunc unpack) {
	dest.set_game(game_to_string(config.game()));
	dest.set_region(region_to_string(config.region()));
	
	IsoFilesystem filesystem = read_iso_filesystem(src);
	table_of_contents toc = read_table_of_contents(src, config.game());
	
	std::vector<UnpackInfo> files;
	
	unpack_ps2_logo(dest, src, config);
	unpack_primary_volume_descriptor(dest, filesystem.pvd);
	
	enumerate_global_wads(files, dest, toc, src, config.game());
	enumerate_level_wads(files, dest.levels(SWITCH_FILES), toc, src);
	enumerate_non_wads(files, dest.files(SWITCH_FILES), "", filesystem.root, src);
	
	// The reported completion percentage is based on how far through the file
	// we are, so it's important to unpack them in order.
	std::sort(BEGIN_END(files), [](auto& lhs, auto& rhs)
		{ return lhs.data_range.offset < rhs.data_range.offset; });
	
	for(UnpackInfo& info : files) {
		SubInputStream stream(src, info.data_range);
		unpack(*info.asset, stream, config, FMT_NO_HINT, info.header_offset);
	}
}

static void unpack_ps2_logo(BuildAsset& build, InputStream& src, BuildConfig config) {
	src.seek(0);
	std::vector<u8> logo = src.read_multiple<u8>(12 * SECTOR_SIZE);
	
	u8 key = logo[0];
	build.set_ps2_logo_key(key);
	
	for(u8& pixel : logo) {
		pixel ^= key;
		pixel = (pixel << 3) | (pixel >> 5);
	}
	
	s32 width, height;
	if(config.is_ntsc()) {
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
	
	if(config.is_ntsc()) {
		build.ps2_logo_ntsc().set_src(ref);
	} else {
		build.ps2_logo_pal().set_src(ref);
	}
}

static void unpack_primary_volume_descriptor(BuildAsset& build, const IsoPrimaryVolumeDescriptor& pvd) {
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

static void enumerate_global_wads(std::vector<UnpackInfo>& dest, BuildAsset& build, const table_of_contents& toc, InputStream& src, Game game) {
	if(game == Game::RAC) {
		s64 toc_ofs = RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE;
		dest.emplace_back(UnpackInfo{&build.bonus<BonusWadAsset>("globals/bonus/bonus.asset"), toc_ofs + (s64) offsetof(RacWadInfo, bonus_1), {0, src.size()}});
		dest.emplace_back(UnpackInfo{&build.mpeg<MpegWadAsset>("globals/mpeg/mpeg.asset"), toc_ofs + (s64) offsetof(RacWadInfo, mpegs), {0, src.size()}});
	} else {
		for(const GlobalWadInfo& global : toc.globals) {
			auto [wad_game, wad_type, name] = identify_wad(global.header);
			std::string file_name = std::string(name) + ".wad";
			size_t file_size = get_global_wad_file_size(global, toc);
			
			Asset* asset;
			switch(wad_type) {
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
			
			dest.emplace_back(UnpackInfo{asset, 0, {global.sector.bytes(), (s64) file_size}});
		}
	}
}

static void enumerate_level_wads(std::vector<UnpackInfo>& dest, CollectionAsset& levels, const table_of_contents& toc, InputStream& src) {
	for(s32 i = 0; i < (s32) toc.levels.size(); i++) {
		const LevelInfo& level = toc.levels[i];
		
		if(level.level.has_value()) {
			assert(level.level->header.size() >= 0xc);
			s32 id = *(s32*) &level.level->header[8];
			
			std::string asset_path = stringf("%02d/level.asset", id);
			LevelAsset& level_asset = levels.foreign_child<LevelAsset>(asset_path, id);
			level_asset.set_index(i);
			
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
					case WadType::LEVEL: {
						LevelWadAsset& level_wad = level_asset.level<LevelWadAsset>();
						level_wad.set_id(id);
						asset = &level_wad;
						break;
					}
					case WadType::LEVEL_AUDIO: {
						asset = &level_asset.audio<LevelAudioWadAsset>();
						break;
					}
					case WadType::LEVEL_SCENE: {
						asset = &level_asset.scene<LevelSceneWadAsset>();
						break;
					}
					default: fprintf(stderr, "warning: Extracted level WAD of unknown type.\n");
				}
				
				dest.emplace_back(UnpackInfo{asset, 0, {part->file_lba.bytes(), part->file_size.bytes()}});
			}
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
			
			dest.emplace_back(UnpackInfo{&asset, 0, {file.lba.bytes(), file.size}});
		}
	}
	for(const IsoDirectory& subdir : dir.subdirs) {
		enumerate_non_wads(dest, files, out/subdir.name, subdir, src);
	}
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
