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

static void unpack_binary_asset(Asset& dest, InputStream& src, Game game, AssetFormatHint hint);
static void unpack_file_asset(FileAsset& dest, InputStream& src, Game game);

on_load(Unpacker, []() {
	BinaryAsset::funcs.unpack_rac1 = new AssetUnpackerFunc(unpack_binary_asset);
	BinaryAsset::funcs.unpack_rac2 = new AssetUnpackerFunc(unpack_binary_asset);
	BinaryAsset::funcs.unpack_rac3 = new AssetUnpackerFunc(unpack_binary_asset);
	BinaryAsset::funcs.unpack_dl = new AssetUnpackerFunc(unpack_binary_asset);
	
	BuildAsset::funcs.unpack_rac1 = wrap_iso_unpacker_func<BuildAsset>(unpack_iso, unpack_asset_impl);
	BuildAsset::funcs.unpack_rac2 = wrap_iso_unpacker_func<BuildAsset>(unpack_iso, unpack_asset_impl);
	BuildAsset::funcs.unpack_rac3 = wrap_iso_unpacker_func<BuildAsset>(unpack_iso, unpack_asset_impl);
	BuildAsset::funcs.unpack_dl = wrap_iso_unpacker_func<BuildAsset>(unpack_iso, unpack_asset_impl);
	
	FileAsset::funcs.unpack_rac1 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_rac2 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_rac3 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_dl = wrap_unpacker_func<FileAsset>(unpack_file_asset);
})

void unpack_asset_impl(Asset& dest, InputStream& src, Game game, AssetFormatHint hint) {
	if(g_asset_unpacker.dump_wads && dest.is_wad) {
		if((!dest.is_level_wad && g_asset_unpacker.dump_global_wads) || (dest.is_level_wad && g_asset_unpacker.dump_level_wads)) {
			unpack_asset_impl(dest.parent()->transmute_child<BinaryAsset>(dest.tag().c_str()), src, game, FMT_BINARY_WAD);
		}
		return;
	}
	
	if(g_asset_unpacker.dump_binaries && dest.is_bin_leaf) {
		unpack_asset_impl(dest.parent()->transmute_child<BinaryAsset>(dest.tag().c_str()), src, game, FMT_NO_HINT);
		return;
	}
	
	std::string reference = asset_reference_to_string(dest.reference());
	std::string type = asset_type_to_string(dest.type());
	for(char& c : type) c = tolower(c);
	s32 percentage = (s32) ((g_asset_unpacker.current_file_offset * 100.f) / g_asset_unpacker.total_file_size);
	printf("[%3d%%] \033[32mUnpacking %s asset %s\033[0m\n", percentage, type.c_str(), reference.c_str());
	
	AssetUnpackerFunc* unpack_func = nullptr;
	if(dest.type() == BuildAsset::ASSET_TYPE) {
		unpack_func = dest.funcs.unpack_rac1;
	} else {
		switch(game) {
			case Game::RAC1: unpack_func = dest.funcs.unpack_rac1; break;
			case Game::RAC2: unpack_func = dest.funcs.unpack_rac2; break;
			case Game::RAC3: unpack_func = dest.funcs.unpack_rac3; break;
			case Game::DL: unpack_func = dest.funcs.unpack_dl; break;
			default: verify_not_reached("Invalid game.");
		}
	}
	
	verify(unpack_func, "Tried to unpack nonunpackable asset '%s'.", reference.c_str());
	(*unpack_func)(dest, src, game, hint);
	
	// Update the completion percentage based on how far through the input file
	// we are, ignoring streams that aren't the input file.
	SubInputStream* sub_stream = dynamic_cast<SubInputStream*>(&src);
	if(sub_stream) {
		if(s64 offset = sub_stream->offset_relative_to(g_asset_unpacker.input_file)) {
			s64 new_file_offset = g_asset_unpacker.current_file_offset;
			if(new_file_offset >= g_asset_unpacker.current_file_offset) {
				g_asset_unpacker.current_file_offset = offset + sub_stream->size();
			}
		}
	}
}

static void unpack_binary_asset(Asset& dest, InputStream& src, Game game, AssetFormatHint hint) {
	BinaryAsset& binary = dest.as<BinaryAsset>();
	std::string file_name = binary.tag() + (hint == FMT_BINARY_WAD ? ".wad" : ".bin");
	auto [stream, ref] = binary.file().open_binary_file_for_writing(file_name);
	verify(stream.get(), "Failed to open file '%s' for writing binary asset '%s'.",
		file_name.c_str(),
		asset_reference_to_string(binary.reference()).c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	binary.set_src(ref);
}

static void unpack_file_asset(FileAsset& dest, InputStream& src, Game game) {
	auto [stream, ref] = dest.file().open_binary_file_for_writing(fs::path(dest.path()));
	verify(stream.get(), "Failed to open file '%s' for writing file asset '%s'.",
		dest.path().c_str(),
		asset_reference_to_string(dest.reference()).c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	dest.set_src(ref);
}
