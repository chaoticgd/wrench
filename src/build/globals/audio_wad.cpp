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

#include "audio_wad.h"

#include <set>
#include <build/asset_packer.h>

static void pack_audio_wad(OutputStream& dest, std::vector<u8>* header_dest, AudioWadAsset& src, Game game);
template <typename Getter>
static void pack_help_audio(OutputStream& dest, Sector32* sectors_dest, s32 count, CollectionAsset& src, Game game, s64 base, Getter getter);
static void unpack_help_audio(BinaryAsset& dest, InputStream& file, Sector32 sector, std::string name, const std::set<s64>& end_sectors);

on_load([]() {
	AudioWadAsset::pack_func = wrap_wad_packer_func<AudioWadAsset>(pack_audio_wad);
})

packed_struct(AudioWadHeaderDL,
	/* 0x0000 */ s32 header_size;
	/* 0x0004 */ Sector32 sector;
	/* 0x0008 */ struct Sector32 vendor[254];
	/* 0x0400 */ struct SectorByteRange global_sfx[12];
	/* 0x0460 */ struct Sector32 help_english[2100];
	/* 0x2530 */ struct Sector32 help_french[2100];
	/* 0x4600 */ struct Sector32 help_german[2100];
	/* 0x66d0 */ struct Sector32 help_spanish[2100];
	/* 0x87a0 */ struct Sector32 help_italian[2100];
)

void pack_audio_wad(OutputStream& dest, std::vector<u8>* header_dest, AudioWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	AudioWadHeaderDL header = {0};
	header.header_size = sizeof(AudioWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.vendor), src.get_vendor(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.global_sfx), src.get_global_sfx(), game, base);
	
	pack_help_audio(dest, ARRAY_PAIR(header.help_english), src.get_help(), game, base, &HelpAudioAsset::get_english);
	pack_help_audio(dest, ARRAY_PAIR(header.help_french), src.get_help(), game, base, &HelpAudioAsset::get_french);
	pack_help_audio(dest, ARRAY_PAIR(header.help_german), src.get_help(), game, base, &HelpAudioAsset::get_german);
	pack_help_audio(dest, ARRAY_PAIR(header.help_spanish), src.get_help(), game, base, &HelpAudioAsset::get_spanish);
	pack_help_audio(dest, ARRAY_PAIR(header.help_italian), src.get_help(), game, base, &HelpAudioAsset::get_italian);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_audio_wad(AudioWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<AudioWadHeaderDL>(src);
	
	std::set<s64> end_sectors;
	for(Sector32 sector : header.vendor) end_sectors.insert(sector.sectors);
	for(SectorByteRange range : header.global_sfx) end_sectors.insert(range.offset.sectors);
	for(Sector32 sector : header.help_english) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_french) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_german) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_spanish) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_italian) end_sectors.insert(sector.sectors);
	end_sectors.insert(Sector32::size_from_bytes(file->size()).sectors);
	
	AudioWadAsset& vendor_file = dest.switch_files("vendor/vendor.asset");
	CollectionAsset& vendor = vendor_file.vendor();
	for(s32 i = 0; i < ARRAY_SIZE(header.vendor); i++) {
		s32 sector = header.vendor[i].sectors;
		if(sector > 0) {
			auto end_sector = end_sectors.upper_bound(sector);
			verify(end_sector != end_sectors.end(), "Header references audio beyond end of file. The WAD file may be truncated.");
			file->seek(sector * SECTOR_SIZE);
			std::vector<u8> bytes = file->read_multiple<u8>((*end_sector - sector) * SECTOR_SIZE);
			FileReference ref = vendor_file.file().write_binary_file(std::to_string(i) + ".vag", bytes);
			BinaryAsset& binary = vendor.child<BinaryAsset>(i);
			binary.set_src(ref);
		}
	}
	unpack_binaries(dest.global_sfx(), *file, ARRAY_PAIR(header.global_sfx), ".vag");
	CollectionAsset& help_collection = dest.help();
	CollectionAsset& help_file = help_collection.switch_files("help/help.asset");
	for(s32 i = 0; i < ARRAY_SIZE(header.help_english); i++) {
		bool exists = false;
		
		exists |= header.help_english[i].sectors > 0;
		exists |= header.help_french[i].sectors > 0;
		exists |= header.help_german[i].sectors > 0;
		exists |= header.help_spanish[i].sectors > 0;
		exists |= header.help_italian[i].sectors > 0;
		
		if(exists) {
			CollectionAsset& help_audio_file = help_file.switch_files(stringf("%d/audio.asset", i));
			HelpAudioAsset& help = help_audio_file.child<HelpAudioAsset>(i);
			
			unpack_help_audio(help.english<BinaryAsset>(), *file, header.help_english[i], "english", end_sectors);
			unpack_help_audio(help.french<BinaryAsset>(), *file, header.help_french[i], "french", end_sectors);
			unpack_help_audio(help.german<BinaryAsset>(), *file, header.help_german[i], "german", end_sectors);
			unpack_help_audio(help.spanish<BinaryAsset>(), *file, header.help_spanish[i], "spanish", end_sectors);
			unpack_help_audio(help.italian<BinaryAsset>(), *file, header.help_italian[i], "italian", end_sectors);
		}
	}
}

template <typename Getter>
static void pack_help_audio(OutputStream& dest, Sector32* sectors_dest, s32 count, CollectionAsset& src, Game game, s64 base, Getter getter) {
	for(size_t i = 0; i < ARRAY_SIZE(AudioWadHeaderDL::help_english); i++) {
		if(src.has_child(i)) {
			Asset& asset = (src.get_child(i).as<HelpAudioAsset>().*getter)();
			sectors_dest[i] = pack_asset_sa<Sector32>(dest, asset, game, base);
		}
	}
}

static void unpack_help_audio(BinaryAsset& dest, InputStream& file, Sector32 sector, std::string name, const std::set<s64>& end_sectors) {
	if(sector.sectors > 0) {
		auto end_sector = end_sectors.upper_bound(sector.sectors);
		verify(end_sector != end_sectors.end(), "Header references audio beyond end of file (%x). The WAD file may be truncated.", sector.sectors);
		file.seek(sector.sectors * SECTOR_SIZE);
		std::vector<u8> bytes = file.read_multiple<u8>((*end_sector - sector.sectors) * SECTOR_SIZE);
		FileReference ref = dest.file().write_binary_file(name + ".vag", bytes);
		dest.set_src(ref);
	}
}
