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

#include "bonus_wad.h"

#include <spanner/asset_packer.h>

packed_struct(BonusWadHeaderDL,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange credits_text[6];
	/* 0x038 */ SectorRange credits_images[13];
	/* 0x0a0 */ SectorRange demomenu[6];
	/* 0x0d0 */ SectorRange demoexit[6];
	/* 0x100 */ SectorRange cheat_images[20];
	/* 0x1a0 */ SectorRange skill_images[31];
	/* 0x298 */ SectorRange trophy_image;
	/* 0x2a0 */ SectorRange dige;
)

void unpack_bonus_wad(BonusWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<BonusWadHeaderDL>(src);
	
	unpack_binaries(dest.credits_text().switch_files(), *file, ARRAY_PAIR(header.credits_text));
	unpack_binaries(dest.credits_images().switch_files(), *file, ARRAY_PAIR(header.credits_images));
	unpack_binaries(dest.demomenu().switch_files(), *file, ARRAY_PAIR(header.demomenu));
	unpack_binaries(dest.demoexit().switch_files(), *file, ARRAY_PAIR(header.demoexit));
	unpack_binaries(dest.cheat_images().switch_files(), *file, ARRAY_PAIR(header.cheat_images));
	unpack_binaries(dest.skill_images().switch_files(), *file, ARRAY_PAIR(header.skill_images));
	unpack_binary(dest.trophy_image<BinaryAsset>(), *file, header.trophy_image, "trophy_image");
	unpack_binary(dest.dige(), *file, header.dige, "dige");
}

void pack_bonus_wad(OutputStream& dest, std::vector<u8>* header_dest, BonusWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	BonusWadHeaderDL header = {0};
	header.header_size = sizeof(BonusWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_text), src.get_credits_text(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_images), src.get_credits_images(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.demomenu), src.get_demomenu(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.demoexit), src.get_demoexit(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.cheat_images), src.get_cheat_images(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.skill_images), src.get_skill_images(), game, base);
	header.trophy_image = pack_asset_sa<SectorRange>(dest, src.get_trophy_image(), game, base);
	header.dige = pack_asset_sa<SectorRange>(dest, src.get_dige(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
