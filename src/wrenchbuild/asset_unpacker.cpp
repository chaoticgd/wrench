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

#include "asset_unpacker.h"

#include <iso/iso_unpacker.h>

AssetUnpackerGlobals g_asset_unpacker = {};

static bool handle_special_debugging_cases(Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint);

on_load(Unpacker, []() {
	BuildAsset::funcs.unpack_rac1 = wrap_iso_unpacker_func<BuildAsset>(unpack_iso, unpack_asset_impl);
})

void unpack_asset_impl(Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint) {
	if (handle_special_debugging_cases(dest, src, header_src, config, hint)) {
		return;
	}
	
	// Hacks to skip unpacking certain wads. These should be removed over time.
	if (config.game() == Game::RAC
		&& (dest.logical_type() == LevelAudioWadAsset::ASSET_TYPE)) {
		std::string tag = dest.tag();
		unpack_asset_impl(dest.parent()->transmute_child<BinaryAsset>(tag.c_str()), src, header_src, config, FMT_BINARY_WAD);
		return;
	}
	
	if (config.game() == Game::GC
		&& (dest.physical_type() == HudWadAsset::ASSET_TYPE
		|| dest.physical_type() == SpaceWadAsset::ASSET_TYPE
		|| dest.physical_type() == SceneWadAsset::ASSET_TYPE
		|| dest.physical_type() == GadgetWadAsset::ASSET_TYPE)) {
		std::string tag = dest.tag();
		unpack_asset_impl(dest.parent()->transmute_child<BinaryAsset>(tag.c_str()), src, header_src, config, FMT_BINARY_WAD);
		return;
	}
	
	if (config.game() == Game::UYA
		&& (dest.physical_type() == HudWadAsset::ASSET_TYPE
		|| dest.physical_type() == SpaceWadAsset::ASSET_TYPE
		|| dest.physical_type() == GadgetWadAsset::ASSET_TYPE)) {
		std::string tag = dest.tag();
		unpack_asset_impl(dest.parent()->transmute_child<BinaryAsset>(tag.c_str()), src, header_src, config, FMT_BINARY_WAD);
		return;
	}
	
	if (dest.physical_type() == LevelSceneWadAsset::ASSET_TYPE) {
		std::string tag = dest.tag();
		unpack_asset_impl(dest.parent()->transmute_child<BinaryAsset>(tag.c_str()), src, header_src, config, FMT_BINARY_WAD);
		return;
	}
	
	std::string reference = dest.absolute_link().to_string();
	std::string type = asset_type_to_string(dest.physical_type());
	if (!g_asset_unpacker.quiet) {
		for (char& c : type) c = tolower(c);
		s32 percentage = (s32) ((g_asset_unpacker.current_file_offset * 100.f) / g_asset_unpacker.total_file_size);
		if (strlen(hint) > 0) {
			printf("[%3d%%] \033[32mUnpacking %s asset %s (%s)\033[0m\n", percentage, type.c_str(), reference.c_str(), hint);
		} else {
			printf("[%3d%%] \033[32mUnpacking %s asset %s\033[0m\n", percentage, type.c_str(), reference.c_str());
		}
	}
	
	AssetUnpackerFunc* unpack_func = nullptr;
	if (dest.physical_type() == BuildAsset::ASSET_TYPE) {
		unpack_func = dest.funcs.unpack_rac1;
	} else {
		switch (config.game()) {
			case Game::RAC: unpack_func = dest.funcs.unpack_rac1; break;
			case Game::GC: unpack_func = dest.funcs.unpack_rac2; break;
			case Game::UYA: unpack_func = dest.funcs.unpack_rac3; break;
			case Game::DL: unpack_func = dest.funcs.unpack_dl; break;
			default: verify_not_reached("Invalid game.");
		}
	}
	
	verify(unpack_func, "Tried to unpack nonunpackable asset \"%s\" of type \"%s\" for game \"%s\".",
		reference.c_str(), type.c_str(), game_to_string(config.game()).c_str());
	(*unpack_func)(dest, src, header_src, config, hint);
	
	// Update the completion percentage based on how far through the input file
	// we are, ignoring streams that aren't the input file.
	SubInputStream* sub_stream = dynamic_cast<SubInputStream*>(&src);
	if (sub_stream) {
		if (s64 offset = sub_stream->offset_relative_to(g_asset_unpacker.input_file)) {
			s64 new_file_offset = offset + sub_stream->size();
			if (new_file_offset >= g_asset_unpacker.current_file_offset) {
				g_asset_unpacker.current_file_offset = new_file_offset;
			}
		}
	}
}

static bool handle_special_debugging_cases(Asset& dest, InputStream& src, const std::vector<u8>* header_src, BuildConfig config, const char* hint) {
	s32 is_wad = dest.flags & ASSET_IS_WAD;
	s32 is_level_wad = dest.flags & ASSET_IS_LEVEL_WAD; 
	s32 is_bin_leaf = dest.flags & ASSET_IS_BIN_LEAF;
	s32 is_flattenable = dest.flags & ASSET_IS_FLATTENABLE;
	if (is_wad && ((!is_level_wad && g_asset_unpacker.skip_globals) || (is_level_wad && g_asset_unpacker.skip_levels))) {
		return true;
	}
	
	if (g_asset_unpacker.dump_wads && is_wad) {
		std::string tag = dest.tag();
		BinaryAsset& bin = dest.parent()->transmute_child<BinaryAsset>(tag.c_str());
		unpack_asset_impl(bin, src, header_src, config, FMT_BINARY_WAD);
		return true;
	}
	
	if (g_asset_unpacker.dump_binaries && is_bin_leaf) {
		const char* type = asset_type_to_string(dest.physical_type());
		std::string tag = dest.tag();
		BinaryAsset& bin = dest.parent()->transmute_child<BinaryAsset>(tag.c_str());
		bin.set_asset_type(type);
		bin.set_format_hint(hint);
		bin.set_game(game_to_string(config.game()));
		bin.set_region(region_to_string(config.region()));
		unpack_asset_impl(bin, src, nullptr, config, FMT_NO_HINT);
		return true;
	}
	
	if (g_asset_unpacker.dump_flat && is_wad && dest.physical_type() != FlatWadAsset::ASSET_TYPE) {
		if (is_flattenable) {
			std::string tag = dest.tag();
			FlatWadAsset& flat_wad = dest.parent()->transmute_child<FlatWadAsset>(tag.c_str());
			unpack_asset_impl(flat_wad, src, nullptr, config);
		}
		return true;
	}
	
	return false;
}
