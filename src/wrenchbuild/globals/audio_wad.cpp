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

#include <set>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(GcAudioWadHeader,
	/* 0x0000 */ s32 header_size;
	/* 0x0004 */ Sector32 sector;
	/* 0x0008 */ Sector32 vendor[254];
	/* 0x0400 */ Sector32 help_english[256];
	/* 0x0800 */ Sector32 help_french[256];
	/* 0x0c00 */ Sector32 help_german[256];
	/* 0x1000 */ Sector32 help_spanish[256];
	/* 0x1400 */ Sector32 help_italian[256];
)

packed_struct(UyaAudioWadHeader,
	/* 0x0000 */ s32 header_size;
	/* 0x0004 */ Sector32 sector;
	/* 0x0008 */ Sector32 vendor[254];
	/* 0x0400 */ Sector32 help_english[400];
	/* 0x0a40 */ Sector32 help_french[400];
	/* 0x1080 */ Sector32 help_german[400];
	/* 0x16c0 */ Sector32 help_spanish[400];
	/* 0x1d00 */ Sector32 help_italian[400];
)

packed_struct(DlAudioWadHeader,
	/* 0x0000 */ s32 header_size;
	/* 0x0004 */ Sector32 sector;
	/* 0x0008 */ Sector32 vendor[254];
	/* 0x0400 */ SectorByteRange global_sfx[12];
	/* 0x0460 */ Sector32 help_english[2100];
	/* 0x2530 */ Sector32 help_french[2100];
	/* 0x4600 */ Sector32 help_german[2100];
	/* 0x66d0 */ Sector32 help_spanish[2100];
	/* 0x87a0 */ Sector32 help_italian[2100];
)

template <typename Header>
static void unpack_audio_wad(
	AudioWadAsset& dest, const Header& header, InputStream& src, BuildConfig config);
template <typename Header>
static void pack_audio_wad(
	OutputStream& dest, Header& header, const AudioWadAsset& src, BuildConfig config);
static void unpack_help_audio(
	CollectionAsset& dest,
	InputStream& src,
	const Sector32* ranges,
	s32 count,
	BuildConfig config,
	const std::set<s64>& end_sectors,
	s32 language);
static void pack_help_audio(
	OutputStream& dest,
	Sector32* sectors_dest,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	s32 language);

on_load(Audio, []() {
	AudioWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<AudioWadAsset, GcAudioWadHeader>(unpack_audio_wad<GcAudioWadHeader>);
	AudioWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<AudioWadAsset, UyaAudioWadHeader>(unpack_audio_wad<UyaAudioWadHeader>);
	AudioWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<AudioWadAsset, DlAudioWadHeader>(unpack_audio_wad<DlAudioWadHeader>);
	
	AudioWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<AudioWadAsset, GcAudioWadHeader>(pack_audio_wad<GcAudioWadHeader>);
	AudioWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<AudioWadAsset, UyaAudioWadHeader>(pack_audio_wad<UyaAudioWadHeader>);
	AudioWadAsset::funcs.pack_dl = wrap_wad_packer_func<AudioWadAsset, DlAudioWadHeader>(pack_audio_wad<DlAudioWadHeader>);
})

template <typename Header>
static void unpack_audio_wad(
	AudioWadAsset& dest, const Header& header, InputStream& src, BuildConfig config)
{
	std::set<s64> end_sectors;
	for (Sector32 sector : header.vendor) end_sectors.insert(sector.sectors);
	if constexpr(std::is_same_v<Header, DlAudioWadHeader>) {
		for (SectorByteRange range : header.global_sfx) end_sectors.insert(range.offset.sectors);
	}
	for (Sector32 sector : header.help_english) end_sectors.insert(sector.sectors);
	for (Sector32 sector : header.help_french) end_sectors.insert(sector.sectors);
	for (Sector32 sector : header.help_german) end_sectors.insert(sector.sectors);
	for (Sector32 sector : header.help_spanish) end_sectors.insert(sector.sectors);
	for (Sector32 sector : header.help_italian) end_sectors.insert(sector.sectors);
	end_sectors.insert(Sector32::size_from_bytes(src.size()).sectors);
	
	CollectionAsset& vendor = dest.vendor("vendor/vendor.asset");
	for (s32 i = 0; i < ARRAY_SIZE(header.vendor); i++) {
		s32 sector = header.vendor[i].sectors;
		if (sector > 0) {
			auto end_sector = end_sectors.upper_bound(sector);
			verify(end_sector != end_sectors.end(), "Header references audio beyond end of file. The WAD file may be truncated.");
			unpack_asset(vendor.child<BinaryAsset>(i), src, SectorRange{sector, (s32) (*end_sector - sector)}, config, FMT_BINARY_VAG);
		}
	}
	
	if constexpr(std::is_same_v<Header, DlAudioWadHeader>) {
		unpack_assets<BinaryAsset>(dest.global_sfx(), src, ARRAY_PAIR(header.global_sfx), config, FMT_BINARY_VAG);
	}
	
	CollectionAsset& help_collection = dest.help("help/help.asset");
	
	unpack_help_audio(help_collection, src, ARRAY_PAIR(header.help_english), config, end_sectors, 0);
	unpack_help_audio(help_collection, src, ARRAY_PAIR(header.help_french), config, end_sectors, 1);
	unpack_help_audio(help_collection, src, ARRAY_PAIR(header.help_german), config, end_sectors, 2);
	unpack_help_audio(help_collection, src, ARRAY_PAIR(header.help_spanish), config, end_sectors, 3);
	unpack_help_audio(help_collection, src, ARRAY_PAIR(header.help_italian), config, end_sectors, 4);
}

template <typename Header>
static void pack_audio_wad(
	OutputStream& dest, Header& header, const AudioWadAsset& src, BuildConfig config)
{
	pack_assets_sa(dest, ARRAY_PAIR(header.vendor), src.get_vendor(), config, FMT_BINARY_VAG);
	
	if constexpr(std::is_same_v<Header, DlAudioWadHeader>) {
		pack_assets_sa(dest, ARRAY_PAIR(header.global_sfx), src.get_global_sfx(), config, FMT_BINARY_VAG);
	}
	
	pack_help_audio(dest, ARRAY_PAIR(header.help_english), src.get_help(), config, 0);
	pack_help_audio(dest, ARRAY_PAIR(header.help_french), src.get_help(), config, 1);
	pack_help_audio(dest, ARRAY_PAIR(header.help_german), src.get_help(), config, 2);
	pack_help_audio(dest, ARRAY_PAIR(header.help_spanish), src.get_help(), config, 3);
	pack_help_audio(dest, ARRAY_PAIR(header.help_italian), src.get_help(), config, 4);
}

static void unpack_help_audio(
	CollectionAsset& dest,
	InputStream& src,
	const Sector32* ranges,
	s32 count,
	BuildConfig config,
	const std::set<s64>& end_sectors,
	s32 language)
{
	for (s32 i = 0; i < count; i++) {
		if (ranges[i].sectors > 0) {
			HelpAudioAsset& child = dest.foreign_child<HelpAudioAsset>(stringf("%d/audio.asset", i), false, i);
			Asset* asset;
			switch (language) {
				case 0: asset = &child.english<BinaryAsset>(); break;
				case 1: asset = &child.french<BinaryAsset>(); break;
				case 2: asset = &child.german<BinaryAsset>(); break;
				case 3: asset = &child.spanish<BinaryAsset>(); break;
				case 4: asset = &child.italian<BinaryAsset>(); break;
				default: verify_not_reached_fatal("Invalid language.");
			}
			
			auto end_sector = end_sectors.upper_bound(ranges[i].sectors);
			verify(end_sector != end_sectors.end(), "Header references audio beyond end of file (at 0x%lx). The WAD file may be truncated.", ranges[i].bytes());
			unpack_asset(*asset, src, SectorRange{ranges[i].sectors, (s32) (*end_sector - ranges[i].sectors)}, config, FMT_BINARY_VAG);
		}
	}
}

static void pack_help_audio(OutputStream& dest, Sector32* sectors_dest, s32 count, const CollectionAsset& src, BuildConfig config, s32 language) {
	for (size_t i = 0; i < count; i++) {
		if (src.has_child(i)) {
			const HelpAudioAsset& asset = src.get_child(i).as<HelpAudioAsset>();
			const Asset* child = nullptr;
			switch (language) {
				case 0: if (asset.has_english()) child = &asset.get_english(); break;
				case 1: if (asset.has_french()) child = &asset.get_french(); break;
				case 2: if (asset.has_german()) child = &asset.get_german(); break;
				case 3: if (asset.has_spanish()) child = &asset.get_spanish(); break;
				case 4: if (asset.has_italian()) child = &asset.get_italian(); break;
				default: verify_not_reached_fatal("Invalid language.");
			}
			if (child) {
				sectors_dest[i] = pack_asset_sa<Sector32>(dest, *child, config, FMT_BINARY_VAG);
			}
		}
	}
}
