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

void unpack_bonus_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<BonusWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("bonus/bonus.asset");
	
	BonusWadAsset& wad = asset_file.root().child<BonusWadAsset>("bonus");
	wad.set_credits_text(unpack_binaries(wad, *file, ARRAY_PAIR(header.credits_text), "credits_text"));
	wad.set_credits_images(unpack_binaries(wad, *file, ARRAY_PAIR(header.credits_images), "credits_images"));
	wad.set_demomenu(unpack_binaries(wad, *file, ARRAY_PAIR(header.demomenu), "demomenu"));
	wad.set_demoexit(unpack_binaries(wad, *file, ARRAY_PAIR(header.demoexit), "demoexit"));
	wad.set_cheat_images(unpack_binaries(wad, *file, ARRAY_PAIR(header.cheat_images), "cheat_images"));
	wad.set_skill_images(unpack_binaries(wad, *file, ARRAY_PAIR(header.skill_images), "skill_images"));
	wad.set_trophy_image(unpack_binary(wad, *file, header.trophy_image, "trophy_image", "trophy_image.bin"));
	wad.set_dige(unpack_binary(wad, *file, header.dige, "dige", "dige.bin"));
}
