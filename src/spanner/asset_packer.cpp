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
#include <spanner/global_wads.h>
#include <spanner/level/level_wad.h>
#include <spanner/level/level_audio_wad.h>
#include <spanner/level/level_scene_wad.h>

static void pack_binary_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, BinaryAsset& asset);

void pack_asset_impl(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& asset, Game game, u32 hint) {
	std::string type = asset_type_to_string(asset.type());
	for(char& c : type) c = tolower(c);
	std::string reference = asset_reference_to_string(asset.absolute_reference());
	printf("[  ?%] \033[32mPacking %s asset %s\033[0m\n", type.c_str(), reference.c_str());
	
	switch(asset.type().id) {
		case BinaryAsset::ASSET_TYPE.id: {
			pack_binary_asset(dest, header_dest, time_dest, static_cast<BinaryAsset&>(asset));
			return;
		}
		case BuildAsset::ASSET_TYPE.id: {
			pack_iso(dest, static_cast<BuildAsset&>(asset), Game::DL, pack_asset_impl);
			return;
		}
		case ArmorWadAsset::ASSET_TYPE.id:
		case AudioWadAsset::ASSET_TYPE.id:
		case BonusWadAsset::ASSET_TYPE.id:
		case HudWadAsset::ASSET_TYPE.id:
		case MiscWadAsset::ASSET_TYPE.id:
		case MpegWadAsset::ASSET_TYPE.id:
		case OnlineWadAsset::ASSET_TYPE.id:
		case SpaceWadAsset::ASSET_TYPE.id: {
			pack_global_wad(dest, header_dest, asset, Game::DL);
			break;
		}
		case LevelWadAsset::ASSET_TYPE.id: {
			pack_level_wad(dest, header_dest, static_cast<LevelWadAsset&>(asset), game);
			break;
		}
		case LevelAudioWadAsset::ASSET_TYPE.id: {
			pack_level_audio_wad(dest, header_dest, static_cast<LevelAudioWadAsset&>(asset), game);
			break;
		}
		case LevelSceneWadAsset::ASSET_TYPE.id: {
			pack_level_scene_wad(dest, header_dest, static_cast<LevelSceneWadAsset&>(asset), game);
			break;
		}
		default: {
			verify_not_reached("Tried to pack unpackable asset '%s'!", reference.c_str());
		}
	}
	if(time_dest) {
		*time_dest = fs::file_time_type::clock::now();
	}
}

static void pack_binary_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, BinaryAsset& asset) {
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
