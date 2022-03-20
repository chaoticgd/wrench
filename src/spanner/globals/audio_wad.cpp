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

static Asset* unpack_help_audio(HelpAudioAsset& help, FileHandle& file, Sector32 sector, std::string name, const std::set<s64>& end_sectors);

void unpack_audio_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<AudioWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("audio/audio.asset");
	
	std::set<s64> end_sectors;
	for(Sector32 sector : header.vendor) end_sectors.insert(sector.sectors);
	for(SectorByteRange range : header.global_sfx) end_sectors.insert(range.offset.sectors);
	for(Sector32 sector : header.help_english) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_french) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_german) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_spanish) end_sectors.insert(sector.sectors);
	for(Sector32 sector : header.help_italian) end_sectors.insert(sector.sectors);
	end_sectors.insert(Sector32::size_from_bytes(file.size()).sectors);
	
	AudioWadAsset& wad = asset_file.root().child<AudioWadAsset>("audio");
	std::vector<Asset*> vendor;
	Asset& vendor_file = wad.asset_file("vendor/vendor.asset");
	CollectionAsset& vendor_collection = vendor_file.child<CollectionAsset>("vendor");
	for(s32 i = 0; i < ARRAY_SIZE(header.vendor); i++) {
		s32 sector = header.vendor[i].sectors;
		if(sector > 0) {
			auto end_sector = end_sectors.upper_bound(sector);
			verify(end_sector != end_sectors.end(), "Header references audio beyond end of file. The WAD file may be truncated.");
			std::vector<u8> bytes = file.read_binary(ByteRange64{sector * SECTOR_SIZE, (*end_sector - sector) * SECTOR_SIZE});
			FileReference ref = vendor_file.file().write_binary_file(std::to_string(i) + ".vag", bytes);
			BinaryAsset& binary = vendor_collection.child<BinaryAsset>(std::to_string(i));
			binary.set_src(ref);
			vendor.push_back(&binary);
		}
	}
	wad.set_vendor(vendor);
	wad.set_global_sfx(unpack_binaries(wad, file, ARRAY_PAIR(header.global_sfx), "global_sfx", ".vag"));
	std::vector<Asset*> help_assets;
	Asset& help_file = wad.asset_file("help/help.asset");
	for(s32 i = 0; i < ARRAY_SIZE(header.help_english); i++) {
		bool exists = false;
		
		exists |= header.help_english[i].sectors > 0;
		exists |= header.help_french[i].sectors > 0;
		exists |= header.help_german[i].sectors > 0;
		exists |= header.help_spanish[i].sectors > 0;
		exists |= header.help_italian[i].sectors > 0;
		
		if(exists) {
			Asset& help_audio_file = help_file.asset_file(stringf("%d/audio.asset", i));
			HelpAudioAsset& help = help_audio_file.child<HelpAudioAsset>(std::to_string(i));
			
			if(Asset* asset = unpack_help_audio(help, file, header.help_english[i], "english", end_sectors)) {
				help.set_english(asset);
			}
			
			if(Asset* asset = unpack_help_audio(help, file, header.help_french[i], "french", end_sectors)) {
				help.set_french(asset);
			}
			
			if(Asset* asset = unpack_help_audio(help, file, header.help_german[i], "german", end_sectors)) {
				help.set_german(asset);
			}
			
			if(Asset* asset = unpack_help_audio(help, file, header.help_spanish[i], "spanish", end_sectors)) {
				help.set_spanish(asset);
			}
			
			if(Asset* asset = unpack_help_audio(help, file, header.help_italian[i], "italian", end_sectors)) {
				help.set_italian(asset);
			}
			
			help_assets.push_back(&help);
		}
	}
	wad.set_help(help_assets);
}

static Asset* unpack_help_audio(HelpAudioAsset& help, FileHandle& file, Sector32 sector, std::string name, const std::set<s64>& end_sectors) {
	if(sector.sectors > 0) {
		auto end_sector = end_sectors.upper_bound(sector.sectors);
		verify(end_sector != end_sectors.end(), "Header references audio beyond end of file (%x). The WAD file may be truncated.", sector.sectors);
		std::vector<u8> bytes = file.read_binary(ByteRange64{sector.sectors * SECTOR_SIZE, (*end_sector - sector.sectors) * SECTOR_SIZE});
		FileReference ref = help.file().write_binary_file(name + ".vag", bytes);
		BinaryAsset& binary = help.child<BinaryAsset>(name);
		binary.set_src(ref);
		return &binary;
	} else {
		return nullptr;
	}
}
