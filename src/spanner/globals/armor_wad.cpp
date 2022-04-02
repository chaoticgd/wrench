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

packed_struct(ArmorHeader,
	SectorRange mesh;
	SectorRange textures;
)

packed_struct(ArmorWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[20];
	/* 0x148 */ SectorRange bot_textures[12];
	/* 0x1a8 */ SectorRange landstalker_textures[8];
	/* 0x1e8 */ SectorRange dropship_textures[8];
)

void unpack_armor_wad(AssetPack& dest, BinaryAsset& src) {
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
}
