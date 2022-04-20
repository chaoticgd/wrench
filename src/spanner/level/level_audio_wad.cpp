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

#include "level_audio_wad.h"

#include <spanner/asset_packer.h>

static void pack_level_audio_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelAudioWadAsset& src, Game game);

on_load([]() {
	LevelAudioWadAsset::pack_func = wrap_wad_packer_func<LevelAudioWadAsset>(pack_level_audio_wad);
})

packed_struct(LevelAudioWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorByteRange bin_data[80];
	/* 0x288 */ SectorByteRange upgrade_sample;
	/* 0x290 */ SectorByteRange platinum_bolt;
	/* 0x298 */ SectorByteRange spare;
)

static void pack_level_audio_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelAudioWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	LevelAudioWadHeaderDL header = {0};
	header.header_size = sizeof(LevelAudioWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.bin_data), src.get_bin_data(), game, base);
	header.upgrade_sample = pack_asset_sa<SectorByteRange>(dest, src.get_upgrade_sample(), game, base);
	header.platinum_bolt = pack_asset_sa<SectorByteRange>(dest, src.get_platinum_bolt(), game, base);
	header.spare = pack_asset_sa<SectorByteRange>(dest, src.get_spare(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_level_audio_wad(LevelAudioWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<LevelAudioWadHeaderDL>(src);
	
	unpack_binaries(dest.bin_data().switch_files(), *file, ARRAY_PAIR(header.bin_data), ".vag");
	unpack_binary(dest.upgrade_sample(), *file, header.upgrade_sample, "upgrade_sample.vag");
	unpack_binary(dest.platinum_bolt(), *file, header.platinum_bolt, "platinum_bolt.vag");
	unpack_binary(dest.spare(), *file, header.spare, "spare.vag");
}
