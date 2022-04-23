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

#include <build/asset_unpacker.h>
#include <build/asset_packer.h>

static void unpack_bonus_wad(BonusWadAsset& dest, InputStream& src, Game game);
static void pack_bonus_wad(OutputStream& dest, std::vector<u8>* header_dest, BonusWadAsset& src, Game game);

on_load(Bonus, []() {
	BonusWadAsset::funcs.unpack_dl = wrap_unpacker_func<BonusWadAsset>(unpack_bonus_wad);
	
	BonusWadAsset::funcs.pack_dl = wrap_wad_packer_func<BonusWadAsset>(pack_bonus_wad);
})

packed_struct(DeadlockedBonusWadHeader,
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

static void unpack_bonus_wad(BonusWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedBonusWadHeader>(0);
	
	unpack_assets<BinaryAsset>(dest.credits_text().switch_files(), src, ARRAY_PAIR(header.credits_text), game);
	unpack_assets<BinaryAsset>(dest.credits_images().switch_files(), src, ARRAY_PAIR(header.credits_images), game);
	unpack_assets<BinaryAsset>(dest.demomenu().switch_files(), src, ARRAY_PAIR(header.demomenu), game);
	unpack_assets<BinaryAsset>(dest.demoexit().switch_files(), src, ARRAY_PAIR(header.demoexit), game);
	unpack_assets<BinaryAsset>(dest.cheat_images().switch_files(), src, ARRAY_PAIR(header.cheat_images), game);
	unpack_assets<BinaryAsset>(dest.skill_images().switch_files(), src, ARRAY_PAIR(header.skill_images), game);
	unpack_asset(dest.trophy_image<BinaryAsset>(), src, header.trophy_image, game);
	unpack_asset(dest.dige(), src, header.dige, game);
}

void pack_bonus_wad(OutputStream& dest, std::vector<u8>* header_dest, BonusWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	DeadlockedBonusWadHeader header = {0};
	header.header_size = sizeof(DeadlockedBonusWadHeader);
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
