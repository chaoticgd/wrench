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

packed_struct(GcBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange goodies_menu[8];
	/* 0x048 */ SectorRange thing_menu[2];
	/* 0x060 */ SectorRange design_sketches[19];
	/* 0x0f0 */ SectorRange design_renders[19];
	/* 0x188 */ SectorRange skill_images[31];
	/* 0x280 */ SectorRange epilogue_english[12];
	/* 0x2e0 */ SectorRange epilogue_french[12];
	/* 0x340 */ SectorRange epilogue_italian[12];
	/* 0x3a0 */ SectorRange thing[25];
	/* 0x460 */ SectorRange sketchbook[30];
	/* 0x550 */ SectorRange commercials[3];
	/* 0x578 */ SectorRange item_images[9];
	/* 0x5c0 */ SectorRange all_text;
	
)

packed_struct(UyaBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ u64 pad_8[183];
	/* 0x5c0 */ SectorRange credits_text[6];
	/* 0x5f0 */ SectorRange credits_images[13];
	/* 0x658 */ u64 pad_658[115];
	/* 0x9f0 */ SectorRange demo_menu[6];
	/* 0xa20 */ SectorRange demo_exit[6];
	/* 0xa50 */ SectorRange cheat_images[20];
	/* 0xaf0 */ SectorRange skill_images[31];
	/* 0xbe8 */ SectorRange trophy_image;
)

packed_struct(DlBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange credits_text[6];
	/* 0x038 */ SectorRange credits_images[13];
	/* 0x0a0 */ SectorRange demo_menu[6];
	/* 0x0d0 */ SectorRange demo_exit[6];
	/* 0x100 */ SectorRange cheat_images[20];
	/* 0x1a0 */ SectorRange skill_images[31];
	/* 0x298 */ SectorRange trophy_image;
	/* 0x2a0 */ SectorRange dige;
)

template <typename Header>
static void unpack_bonus_wad(BonusWadAsset& dest, InputStream& src, Game game);
template <typename Header>
static void pack_bonus_wad(OutputStream& dest, Header& header, BonusWadAsset& src, Game game);
static void unpack_demo_images(CollectionAsset& dest, InputStream& src, SectorRange* ranges, s32 outer_count, s32 inner_count, Game game);
static void pack_demo_images(OutputStream& dest, SectorRange* ranges, s32 outer_count, s32 inner_count, CollectionAsset& src, Game game, const char* muffin);

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
	unpack_demo_images(dest.demo_menu().switch_files(), src, ARRAY_PAIR(header.demo_menu), 30, game);
	unpack_demo_images(dest.demo_exit().switch_files(), src, ARRAY_PAIR(header.demo_exit), 10, game);
	unpack_assets<TextureAsset>(dest.cheat_images().switch_files(), src, ARRAY_PAIR(header.cheat_images), game, FMT_TEXTURE_PIF8);
	unpack_assets<TextureAsset>(dest.skill_images().switch_files(), src, ARRAY_PAIR(header.skill_images), game, FMT_TEXTURE_PIF8);
	unpack_asset(dest.trophy_image<BinaryAsset>(), src, header.trophy_image, game);
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		unpack_asset(dest.dige(), src, header.dige, game);
	}
}

template <typename Header>
void pack_bonus_wad(OutputStream& dest, Header& header, BonusWadAsset& src, Game game) {
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_text), src.get_credits_text(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_images), src.get_credits_images(), game, FMT_TEXTURE_RGBA);
	pack_demo_images(dest, ARRAY_PAIR(header.demo_menu), 30, src.get_demo_menu(), game, "demo_menu");
	pack_demo_images(dest, ARRAY_PAIR(header.demo_exit), 10, src.get_demo_exit(), game, "demo_exit");
	pack_assets_sa(dest, ARRAY_PAIR(header.cheat_images), src.get_cheat_images(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.skill_images), src.get_skill_images(), game, FMT_TEXTURE_PIF8);
	header.trophy_image = pack_asset_sa<SectorRange>(dest, src.get_trophy_image(), game);
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		header.dige = pack_asset_sa<SectorRange>(dest, src.get_dige(), game);
	}
}

static void unpack_demo_images(CollectionAsset& dest, InputStream& src, SectorRange* ranges, s32 outer_count, s32 inner_count, Game game) {
	for(s32 i = 0; i < outer_count; i++) {
		CollectionAsset& inner = dest.child<CollectionAsset>(i).switch_files();
		SubInputStream stream(src, ranges[i].bytes());
		std::vector<s32> offsets = stream.read_multiple<s32>(0, inner_count);
		for(s32 j = 0; j < inner_count; j++) {
			if(offsets[j] > -1) {
				ByteRange range;
				range.offset = offsets[j];
				if(j + 1 < inner_count && offsets[j + 1] > -1) {
					range.size = offsets[j + 1] - offsets[j];
				} else {
					range.size = (s32) stream.size() - offsets[j];
				}
				unpack_compressed_asset(inner.child<TextureAsset>(j), stream, range, game, FMT_TEXTURE_RGBA);
			}
		}
	}
}

static void pack_demo_images(OutputStream& dest, SectorRange* ranges, s32 outer_count, s32 inner_count, CollectionAsset& src, Game game, const char* muffin) {
	for(s32 i = 0; i < outer_count; i++) {
		if(src.has_child(i)) {
			CollectionAsset& inner = src.get_child(i).as<CollectionAsset>();
			dest.pad(SECTOR_SIZE, 0);
			s64 begin_ofs = dest.tell();
			ranges[i].offset = Sector32::size_from_bytes(begin_ofs);
			SubOutputStream stream(dest, begin_ofs);
			stream.alloc_multiple<s32>(inner_count);
			std::vector<s32> offsets(inner_count);
			for(s32 j = 0; j < inner_count; j++) {
				if(inner.has_child(j)) {
					offsets[j] = pack_compressed_asset<ByteRange>(stream, inner.get_child(j).as<TextureAsset>(), game, 0x10, muffin, FMT_TEXTURE_RGBA).offset;
				}
			}
			stream.seek(0);
			stream.write_v(offsets);
			ranges[i].size = Sector32::size_from_bytes(dest.tell() - begin_ofs);
		}
	}
}
