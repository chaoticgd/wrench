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

static void unpack_binary_asset(BinaryAsset& dest, InputStream& src, Game game);
static void unpack_file_asset(FileAsset& dest, InputStream& src, Game game);

on_load(Unpacker, []() {
	BinaryAsset::funcs.unpack_rac1 = wrap_unpacker_func<BinaryAsset>(unpack_binary_asset);
	BinaryAsset::funcs.unpack_rac2 = wrap_unpacker_func<BinaryAsset>(unpack_binary_asset);
	BinaryAsset::funcs.unpack_rac3 = wrap_unpacker_func<BinaryAsset>(unpack_binary_asset);
	BinaryAsset::funcs.unpack_dl = wrap_unpacker_func<BinaryAsset>(unpack_binary_asset);
	
	FileAsset::funcs.unpack_rac1 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_rac2 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_rac3 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_dl = wrap_unpacker_func<FileAsset>(unpack_file_asset);
})

void unpack_asset_impl(Asset& dest, InputStream& src, Game game, AssetFormatHint hint) {
	std::string reference = asset_reference_to_string(dest.reference());
	std::string type = asset_type_to_string(dest.type());
	for(char& c : type) c = tolower(c);
	printf("[%3d%%] \033[32mUnpacking %s asset %s\033[0m\n", -1, type.c_str(), reference.c_str());
	
	AssetUnpackerFunc* unpack_func = nullptr;
	switch(game) {
		case Game::RAC1: unpack_func = dest.funcs.unpack_rac1; break;
		case Game::RAC2: unpack_func = dest.funcs.unpack_rac2; break;
		case Game::RAC3: unpack_func = dest.funcs.unpack_rac3; break;
		case Game::DL: unpack_func = dest.funcs.unpack_dl; break;
		default: verify_not_reached("Invalid game.");
	}
	verify(unpack_func, "Tried to unpack nonunpackable asset '%s'.", reference.c_str());
	(*unpack_func)(dest, src, game, hint);
}

static void unpack_binary_asset(BinaryAsset& dest, InputStream& src, Game game) {
	std::string file_name = dest.tag() + ".bin";
	auto [stream, ref] = dest.file().open_binary_file_for_writing(file_name);
	verify(stream.get(), "Failed to open file '%s' for writing binary asset '%s'.",
		file_name.c_str(),
		asset_reference_to_string(dest.reference()).c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	dest.set_src(ref);
}

static void unpack_file_asset(FileAsset& dest, InputStream& src, Game game) {
	std::string file_name = dest.tag() + ".bin";
	auto [stream, ref] = dest.file().open_binary_file_for_writing(file_name);
	verify(stream.get(), "Failed to open file '%s' for writing file asset '%s'.",
		file_name.c_str(),
		asset_reference_to_string(dest.reference()).c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	dest.set_src(ref);
}
