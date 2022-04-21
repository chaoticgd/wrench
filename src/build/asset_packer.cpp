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

#include "asset_packer.h"

#include <iso/iso_packer.h>
#include <build/global_wads.h>
#include <build/level/level_wad.h>
#include <build/level/level_audio_wad.h>
#include <build/level/level_scene_wad.h>

s32 g_asset_packer_max_assets_processed = 0;
s32 g_asset_packer_num_assets_processed = 0;
bool g_asset_packer_dry_run = false;

static void pack_binary_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, BinaryAsset& asset);
static void pack_file_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, FileAsset& asset);

on_load([]() {
	BinaryAsset::pack_func = wrap_bin_packer_func<BinaryAsset>(pack_binary_asset);
	BuildAsset::pack_func = wrap_iso_packer_func<BuildAsset>(pack_iso, pack_asset_impl);
	FileAsset::pack_func = wrap_bin_packer_func<FileAsset>(pack_file_asset);
})

void pack_asset_impl(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& asset, Game game, u32 hint) {
	if(!g_asset_packer_dry_run) {
		std::string type = asset_type_to_string(asset.type());
		for(char& c : type) c = tolower(c);
		std::string reference = asset_reference_to_string(asset.absolute_reference());
		s32 completion_percentage = (s32) ((g_asset_packer_num_assets_processed * 100.f) / g_asset_packer_max_assets_processed);
		printf("[%3d%%] \033[32mPacking %s asset %s\033[0m\n", completion_percentage, type.c_str(), reference.c_str());
	}
	
	asset.pack(dest, header_dest, time_dest, game, hint);
	
	g_asset_packer_num_assets_processed++;
}

static void pack_binary_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, BinaryAsset& asset) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	auto src = asset.file().open_binary_file_for_reading(asset.src(), time_dest);
	if(header_dest) {
		s32 header_size = src->read<s32>();
		assert(header_size == header_dest->size());
		s64 padded_header_size = Sector32::size_from_bytes(header_size).bytes();
		
		// Extract the header.
		assert(padded_header_size != 0);
		header_dest->resize(padded_header_size);
		*(s32*) header_dest->data() = header_size;
		src->read(header_dest->data() + 4, padded_header_size - 4);
		
		// Write the header.
		dest.write(header_dest->data(), padded_header_size);
		
		// The calling code needs the unpadded header.
		header_dest->resize(header_size);
		
		assert(dest.tell() % SECTOR_SIZE == 0);
		
		// Copy the rest of the file.
		Stream::copy(dest, *src, src->size() - padded_header_size);
	} else {
		Stream::copy(dest, *src, src->size());
	}
}

static void pack_file_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, FileAsset& asset) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	FileReference ref = asset.src();
	auto src = asset.file().open_binary_file_for_reading(asset.src(), time_dest);
	verify(src.get(), "Failed to open file '%s' for reading.", ref.path.string().c_str());
	Stream::copy(dest, *src, src->size());
}
