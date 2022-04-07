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

#include "armor_wad.h"

#include <spanner/asset_packer.h>

packed_struct(ArmorHeader,
	/* 0x0 */ SectorRange mesh;
	/* 0x8 */ SectorRange textures;
)

packed_struct(ArmorWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[20];
	/* 0x148 */ SectorRange bot_textures[12];
	/* 0x1a8 */ SectorRange landstalker_textures[8];
	/* 0x1e8 */ SectorRange dropship_textures[8];
)

ArmorWadAsset& unpack_armor_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<ArmorWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("armors/armors.asset");
	
	ArmorWadAsset& wad = asset_file.root().child<ArmorWadAsset>("bonus");
	std::vector<Asset*> armors;
	for(s32 i = 0; i < ARRAY_SIZE(header.armors); i++) {
		if(header.armors[i].mesh.size.sectors > 0) {
			Asset& armor_wad = wad.asset_file(stringf("%02d/armor.asset", i));
			ArmorAsset& armor = armor_wad.child<ArmorAsset>(std::to_string(i));
			armor.set_mesh(unpack_binary(armor, *file, header.armors[i].mesh, "mesh", "mesh.bin"));
			armor.set_mesh(unpack_binary(armor, *file, header.armors[i].textures, "textures", "textures.bin"));
			armors.push_back(&armor);
		}
	}
	wad.set_armors(armors);
	wad.set_bot_textures(unpack_binaries(wad, *file, ARRAY_PAIR(header.bot_textures), "bot_textures"));
	wad.set_landstalker_textures(unpack_binaries(wad, *file, ARRAY_PAIR(header.landstalker_textures), "landstalker_textures"));
	wad.set_dropship_textures(unpack_binaries(wad, *file, ARRAY_PAIR(header.dropship_textures), "dropship_textures"));
	
	return wad;
}

void pack_armor_wad(OutputStream& dest, ArmorWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	ArmorWadHeaderDL header = {0};
	header.header_size = sizeof(ArmorWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	std::vector<Asset*> armors = wad.armors();
	verify(armors.size() < ARRAY_SIZE(header.armors), "Too many armors.");
	for(size_t i = 0; i < armors.size(); i++) {
		ArmorAsset* armor = static_cast<ArmorAsset*>(armors[i]);
		verify(armor, "Armor assets must be of type ArmorAsset.");
		header.armors[i].mesh = pack_asset<SectorRange>(dest, *armor->mesh(), game, base);
		// textures
	}
	//pack_binaries(dest, ARRAY_PAIR(header.bot_textures), wad.bot_textures(), base);
	//pack_binaries(dest, ARRAY_PAIR(header.landstalker_textures), wad.landstalker_textures(), base);
	//pack_binaries(dest, ARRAY_PAIR(header.dropship_textures), wad.dropship_textures(), base);
	
	dest.write(base, header);
}
