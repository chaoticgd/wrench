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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(RacAudioWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorByteRange bin_data[36];
	/* 0x128 */ Sector32 music[15];
)

packed_struct(GcLevelAudioWadHeader,
	/* 0x0000 */ s32 header_size;
	/* 0x0004 */ Sector32 sector;
	/* 0x0008 */ SectorByteRange bin_data[511];
	/* 0x1000 */ SectorByteRange upgrade_sample;
	/* 0x1008 */ SectorByteRange thermanator_freeze;
	/* 0x1010 */ SectorByteRange thermanator_thaw;
)

packed_struct(UyaLevelAudioWadHeader,
	/* 0x0000 */ s32 header_size;
	/* 0x0004 */ Sector32 sector;
	/* 0x0008 */ SectorByteRange bin_data[767];
	/* 0x1800 */ SectorByteRange upgrade_sample;
	/* 0x1808 */ SectorByteRange platinum_bolt;
	/* 0x1810 */ SectorByteRange spare;
)

packed_struct(DlLevelAudioWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorByteRange bin_data[80];
	/* 0x288 */ SectorByteRange upgrade_sample;
	/* 0x290 */ SectorByteRange platinum_bolt;
	/* 0x298 */ SectorByteRange spare;
)

static void unpack_gc_level_audio_wad(LevelAudioWadAsset& dest, const GcLevelAudioWadHeader& header, InputStream& src, Game game);
static void pack_gc_level_audio_wad(OutputStream& dest, GcLevelAudioWadHeader& header, const LevelAudioWadAsset& src, Game game);
template <typename Header>
static void unpack_uya_dl_level_audio_wad(LevelAudioWadAsset& dest, const Header& header, InputStream& src, Game game);
template <typename Header>
static void pack_uya_dl_level_audio_wad(OutputStream& dest, Header& header, const LevelAudioWadAsset& src, Game game);

on_load(LevelAudio, []() {
	LevelAudioWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<LevelAudioWadAsset, GcLevelAudioWadHeader>(unpack_gc_level_audio_wad);
	LevelAudioWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<LevelAudioWadAsset, UyaLevelAudioWadHeader>(unpack_uya_dl_level_audio_wad<UyaLevelAudioWadHeader>);
	LevelAudioWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<LevelAudioWadAsset, DlLevelAudioWadHeader>(unpack_uya_dl_level_audio_wad<DlLevelAudioWadHeader>);
	
	LevelAudioWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<LevelAudioWadAsset, GcLevelAudioWadHeader>(pack_gc_level_audio_wad);
	LevelAudioWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<LevelAudioWadAsset, UyaLevelAudioWadHeader>(pack_uya_dl_level_audio_wad<UyaLevelAudioWadHeader>);
	LevelAudioWadAsset::funcs.pack_dl = wrap_wad_packer_func<LevelAudioWadAsset, DlLevelAudioWadHeader>(pack_uya_dl_level_audio_wad<DlLevelAudioWadHeader>);
})

static void unpack_gc_level_audio_wad(LevelAudioWadAsset& dest, const GcLevelAudioWadHeader& header, InputStream& src, Game game) {
	unpack_assets<BinaryAsset>(dest.bin_data().switch_files(), src, ARRAY_PAIR(header.bin_data), game);
	unpack_asset(dest.upgrade_sample(), src, header.upgrade_sample, game);
	unpack_asset(dest.thermanator_freeze(), src, header.thermanator_freeze, game);
	unpack_asset(dest.thermanator_thaw(), src, header.thermanator_thaw, game);
}

static void pack_gc_level_audio_wad(OutputStream& dest, GcLevelAudioWadHeader& header, const LevelAudioWadAsset& src, Game game) {
	pack_assets_sa(dest, ARRAY_PAIR(header.bin_data), src.get_bin_data(), game);
	header.upgrade_sample = pack_asset_sa<SectorByteRange>(dest, src.get_upgrade_sample(), game);
	header.thermanator_freeze = pack_asset_sa<SectorByteRange>(dest, src.get_thermanator_freeze(), game);
	header.thermanator_thaw = pack_asset_sa<SectorByteRange>(dest, src.get_thermanator_thaw(), game);
}

template <typename Header>
static void unpack_uya_dl_level_audio_wad(LevelAudioWadAsset& dest, const Header& header, InputStream& src, Game game) {
	unpack_assets<BinaryAsset>(dest.bin_data().switch_files(), src, ARRAY_PAIR(header.bin_data), game);
	unpack_asset(dest.upgrade_sample(), src, header.upgrade_sample, game);
	unpack_asset(dest.platinum_bolt(), src, header.platinum_bolt, game);
	unpack_asset(dest.spare(), src, header.spare, game);
}

template <typename Header>
static void pack_uya_dl_level_audio_wad(OutputStream& dest, Header& header, const LevelAudioWadAsset& src, Game game) {
	pack_assets_sa(dest, ARRAY_PAIR(header.bin_data), src.get_bin_data(), game);
	header.upgrade_sample = pack_asset_sa<SectorByteRange>(dest, src.get_upgrade_sample(), game);
	header.platinum_bolt = pack_asset_sa<SectorByteRange>(dest, src.get_platinum_bolt(), game);
	header.spare = pack_asset_sa<SectorByteRange>(dest, src.get_spare(), game);
}