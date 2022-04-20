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

static void pack_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game);

on_load([]() {
	ArmorWadAsset::pack_func = wrap_wad_packer_func<ArmorWadAsset>(pack_armor_wad);
})

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

void pack_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	ArmorWadHeaderDL header = {0};
	header.header_size = sizeof(ArmorWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	for(size_t i = 0; i < ARRAY_SIZE(header.armors); i++) {
		if(src.get_armors().has_child(i)) {
			ArmorAsset& armor = src.get_armors().get_child(i).as<ArmorAsset>();
			header.armors[i].mesh = pack_asset<SectorRange>(dest, armor.get_mesh(), game, base);
			header.armors[i].textures = pack_asset<SectorRange>(dest, armor.get_textures(), game, base);
		}
	}
	pack_assets_sa(dest, ARRAY_PAIR(header.bot_textures), src.get_bot_textures(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.landstalker_textures), src.get_landstalker_textures(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.dropship_textures), src.get_dropship_textures(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

void unpack_armor_wad(ArmorWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<ArmorWadHeaderDL>(src);
	
	CollectionAsset& armors = dest.armors();
	for(s32 i = 0; i < ARRAY_SIZE(header.armors); i++) {
		if(header.armors[i].mesh.size.sectors > 0) {
			Asset& armor_file = armors.switch_files(stringf("armors/%02d/armor%02d.asset", i, i));
			ArmorAsset& armor = armor_file.child<ArmorAsset>(std::to_string(i).c_str());
			unpack_binary(armor.mesh<BinaryAsset>(), *file, header.armors[i].mesh, "mesh.bin");
			unpack_binary(armor.textures(), *file, header.armors[i].textures, "textures.bin");
		}
	}
	unpack_binaries(dest.bot_textures().switch_files(), *file, ARRAY_PAIR(header.bot_textures));
	unpack_binaries(dest.landstalker_textures().switch_files(), *file, ARRAY_PAIR(header.landstalker_textures));
	unpack_binaries(dest.dropship_textures().switch_files(), *file, ARRAY_PAIR(header.dropship_textures));
}
