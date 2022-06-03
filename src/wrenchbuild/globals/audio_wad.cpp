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
static void unpack_audio_wad(AudioWadAsset& dest, const Header& header, InputStream& src, Game game);
template <typename Header>
static void pack_audio_wad(OutputStream& dest, Header& header, const AudioWadAsset& src, Game game);
template <typename Getter>
static void unpack_help_audio(CollectionAsset& dest, InputStream& src, const Sector32* ranges, s32 count, Game game, const std::set<s64>& end_sectors, Getter getter);
static void pack_help_audio(OutputStream& dest, Sector32* sectors_dest, s32 count, const CollectionAsset& src, Game game, s32 language);

on_load(Audio, []() {
	AudioWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<AudioWadAsset, GcAudioWadHeader>(unpack_audio_wad<GcAudioWadHeader>);
	AudioWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<AudioWadAsset, UyaAudioWadHeader>(unpack_audio_wad<UyaAudioWadHeader>);
	AudioWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<AudioWadAsset, DlAudioWadHeader>(unpack_audio_wad<DlAudioWadHeader>);
	
	AudioWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<AudioWadAsset, GcAudioWadHeader>(pack_audio_wad<GcAudioWadHeader>);
	AudioWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<AudioWadAsset, UyaAudioWadHeader>(pack_audio_wad<UyaAudioWadHeader>);
	AudioWadAsset::funcs.pack_dl = wrap_wad_packer_func<AudioWadAsset, DlAudioWadHeader>(pack_audio_wad<DlAudioWadHeader>);
})

template <typename Header>
static void unpack_audio_wad(AudioWadAsset& dest, const Header& header, InputStream& src, Game game) {
	std::set<s64> end_sectors;
	for(Sector32 sector : header.vendor) end_sectors.insert(sector.sectors);
	if constexpr(std::is_same_v<Header, DlAudioWadHeader>) {
		for(SectorByteRange range : header.global_sfx) end_sectors.insert(range.offset.sectors);
	}
	for(Sector32 sector : header.help_english) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_french) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_german) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_spanish) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_italian) end_sectors.insert(sector.sectors);
	end_sectors.insert(Sector32::size_from_bytes(src.size()).sectors);
	
	AudioWadAsset& vendor_file = dest.switch_files("vendor/vendor.asset");
	CollectionAsset& vendor = vendor_file.vendor();
	for(s32 i = 0; i < ARRAY_SIZE(header.vendor); i++) {
		s32 sector = header.vendor[i].sectors;
		if(sector > 0) {
			auto end_sector = end_sectors.upper_bound(sector);
			verify(end_sector != end_sectors.end(), "Header references audio beyond end of file. The WAD file may be truncated.");
			unpack_asset(vendor.child<BinaryAsset>(i), src, SectorRange{sector, *end_sector - sector}, game);
		}
	}
	
	if constexpr(std::is_same_v<Header, DlAudioWadHeader>) {
		unpack_assets<BinaryAsset>(dest.global_sfx(), src, ARRAY_PAIR(header.global_sfx), game);
	}
	
	CollectionAsset& help_collection = dest.help();
	CollectionAsset& help_file = help_collection.switch_files("help/help.asset");
	
	unpack_help_audio(help_file, src, ARRAY_PAIR(header.help_english), game, end_sectors, &HelpAudioAsset::english<BinaryAsset>);
	unpack_help_audio(help_file, src, ARRAY_PAIR(header.help_french), game, end_sectors, &HelpAudioAsset::french<BinaryAsset>);
	unpack_help_audio(help_file, src, ARRAY_PAIR(header.help_german), game, end_sectors, &HelpAudioAsset::german<BinaryAsset>);
	unpack_help_audio(help_file, src, ARRAY_PAIR(header.help_spanish), game, end_sectors, &HelpAudioAsset::spanish<BinaryAsset>);
	unpack_help_audio(help_file, src, ARRAY_PAIR(header.help_italian), game, end_sectors, &HelpAudioAsset::italian<BinaryAsset>);
}

template <typename Header>
static void pack_audio_wad(OutputStream& dest, Header& header, const AudioWadAsset& src, Game game) {
	pack_assets_sa(dest, ARRAY_PAIR(header.vendor), src.get_vendor(), game);
	
	if constexpr(std::is_same_v<Header, DlAudioWadHeader>) {
		pack_assets_sa(dest, ARRAY_PAIR(header.global_sfx), src.get_global_sfx(), game);
	}
	
	pack_help_audio(dest, ARRAY_PAIR(header.help_english), src.get_help(), game, 0);
	pack_help_audio(dest, ARRAY_PAIR(header.help_french), src.get_help(), game, 1);
	pack_help_audio(dest, ARRAY_PAIR(header.help_german), src.get_help(), game, 2);
	pack_help_audio(dest, ARRAY_PAIR(header.help_spanish), src.get_help(), game, 3);
	pack_help_audio(dest, ARRAY_PAIR(header.help_italian), src.get_help(), game, 4);
}

template <typename Getter>
static void unpack_help_audio(CollectionAsset& dest, InputStream& src, const Sector32* ranges, s32 count, Game game, const std::set<s64>& end_sectors, Getter getter) {
	for(s32 i = 0; i < count; i++) {
		if(ranges[i].sectors > 0) {
			CollectionAsset& help_audio_file = dest.switch_files(stringf("%d/audio.asset", i));
			Asset& asset = (help_audio_file.child<HelpAudioAsset>(i).*getter)();
			
			auto end_sector = end_sectors.upper_bound(ranges[i].sectors);
			verify(end_sector != end_sectors.end(), "Header references audio beyond end of file (at 0x%lx). The WAD file may be truncated.", ranges[i].bytes());
			unpack_asset(asset, src, SectorRange{ranges[i].sectors, *end_sector - ranges[i].sectors}, game);
		}
	}
}

static void pack_help_audio(OutputStream& dest, Sector32* sectors_dest, s32 count, const CollectionAsset& src, Game game, s32 language) {
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			const HelpAudioAsset& asset = src.get_child(i).as<HelpAudioAsset>();
			const Asset* child = nullptr;
			switch(language) {
				case 0: if(asset.has_english()) child = &asset.get_english(); break;
				case 1: if(asset.has_french()) child = &asset.get_french(); break;
				case 2: if(asset.has_german()) child = &asset.get_german(); break;
				case 3: if(asset.has_spanish()) child = &asset.get_spanish(); break;
				case 4: if(asset.has_italian()) child = &asset.get_italian(); break;
				default: assert(0);
			}
			if(child) {
				sectors_dest[i] = pack_asset_sa<Sector32>(dest, *child, game);
			}
		}
	}
}
