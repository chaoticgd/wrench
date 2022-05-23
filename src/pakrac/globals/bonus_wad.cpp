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

#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

packed_struct(UyaBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ u32 pad_8[366];
	/* 0x5c0 */ SectorRange credits_text[6];
	/* 0x5f0 */ SectorRange credits_images[13];
	/* 0x658 */ u32 pad_658[230];
	/* 0x9f0 */ SectorRange cheat_images2[6];
	/* 0x9f0 */ SectorRange cheat_images4[6];
	/* 0xa50 */ SectorRange cheat_images[9];
	/* 0xa98 */ SectorRange cheat_images3[11];
	/* 0xaf0 */ SectorRange skill_images[32];
)

packed_struct(DlBonusWadHeader,
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

template <typename Header>
static void unpack_bonus_wad(BonusWadAsset& dest, InputStream& src, Game game);
template <typename Header>
static void pack_bonus_wad(OutputStream& dest, Header& header, BonusWadAsset& src, Game game);

on_load(Bonus, []() {
	BonusWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<BonusWadAsset>(unpack_bonus_wad<UyaBonusWadHeader>);
	BonusWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<BonusWadAsset>(unpack_bonus_wad<DlBonusWadHeader>);
	
	BonusWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<BonusWadAsset, UyaBonusWadHeader>(pack_bonus_wad<UyaBonusWadHeader>);
	BonusWadAsset::funcs.pack_dl = wrap_wad_packer_func<BonusWadAsset, DlBonusWadHeader>(pack_bonus_wad<DlBonusWadHeader>);
})

template <typename Header>
static void unpack_bonus_wad(BonusWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Header>(0);
	
	unpack_assets<BinaryAsset>(dest.credits_text().switch_files(), src, ARRAY_PAIR(header.credits_text), game);
	unpack_assets<TextureAsset>(dest.credits_images().switch_files(), src, ARRAY_PAIR(header.credits_images), game, FMT_TEXTURE_RGBA);
	
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		unpack_assets<BinaryAsset>(dest.demomenu().switch_files(), src, ARRAY_PAIR(header.demomenu), game);
		unpack_assets<BinaryAsset>(dest.demoexit().switch_files(), src, ARRAY_PAIR(header.demoexit), game);
	}
	
	unpack_assets<TextureAsset>(dest.cheat_images().switch_files(), src, ARRAY_PAIR(header.cheat_images), game, FMT_TEXTURE_PIF8);
	unpack_assets<TextureAsset>(dest.skill_images().switch_files(), src, ARRAY_PAIR(header.skill_images), game, FMT_TEXTURE_PIF8);
	
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		unpack_asset(dest.trophy_image<BinaryAsset>(), src, header.trophy_image, game);
		unpack_asset(dest.dige(), src, header.dige, game);
	}
}

template <typename Header>
void pack_bonus_wad(OutputStream& dest, Header& header, BonusWadAsset& src, Game game) {
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_text), src.get_credits_text(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_images), src.get_credits_images(), game, FMT_TEXTURE_RGBA);
	
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		pack_assets_sa(dest, ARRAY_PAIR(header.demomenu), src.get_demomenu(), game);
		pack_assets_sa(dest, ARRAY_PAIR(header.demoexit), src.get_demoexit(), game);
	}
	
	pack_assets_sa(dest, ARRAY_PAIR(header.cheat_images), src.get_cheat_images(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.skill_images), src.get_skill_images(), game, FMT_TEXTURE_PIF8);
	
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		header.trophy_image = pack_asset_sa<SectorRange>(dest, src.get_trophy_image(), game);
		header.dige = pack_asset_sa<SectorRange>(dest, src.get_dige(), game);
	}
}
