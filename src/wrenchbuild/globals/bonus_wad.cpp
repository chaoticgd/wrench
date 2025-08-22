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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(RacBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange goodies_images[10];
	/* 0x060 */ SectorRange character_sketches[19];
	/* 0x0f0 */ SectorRange character_renders[19];
	/* 0x188 */ SectorRange skill_images[31];
	/* 0x280 */ SectorRange epilogue_english[12];
	/* 0x2e0 */ SectorRange epilogue_french[12];
	/* 0x340 */ SectorRange epilogue_italian[12];
	/* 0x3a0 */ SectorRange epilogue_german[12];
	/* 0x400 */ SectorRange epilogue_spanish[12];
	/* 0x460 */ SectorRange sketchbook[30];
	/* 0x550 */ SectorRange commercials[4];
	/* 0x578 */ SectorRange item_images[9];
	/* 0x5c0 */ u64 dont_care[245];
	/* 0xd68 */ SectorRange credits_images_ntsc[20];
	/* 0xe08 */ SectorRange credits_images_pal[20];
)

packed_struct(GcBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange goodies_images[10];
	/* 0x060 */ SectorRange character_sketches[19];
	/* 0x0f0 */ SectorRange character_renders[19];
	/* 0x188 */ SectorRange old_skill_images[31];
	/* 0x280 */ SectorRange epilogue_english[12];
	/* 0x2e0 */ SectorRange epilogue_french[12];
	/* 0x340 */ SectorRange epilogue_italian[12];
	/* 0x3a0 */ SectorRange epilogue_german[12];
	/* 0x400 */ SectorRange epilogue_spanish[12];
	/* 0x460 */ SectorRange sketchbook[30];
	/* 0x550 */ SectorRange commercials[5];
	/* 0x578 */ SectorRange item_images[9];
	/* 0x5c0 */ SectorRange credits_text;
	/* 0x5c8 */ SectorRange credits_images[29];
	/* 0x6b0 */ SectorRange random_stuff[5];
	/* 0x6d8 */ SectorRange movie_images[5];
	/* 0x700 */ SectorRange cinematic_images[33];
	/* 0x808 */ SectorRange skill_images[30];
	/* 0x8f8 */ SectorRange clanks_day[18];
	/* 0x988 */ SectorRange endorsement_deals[10];
	/* 0x9d8 */ SectorRange short_cuts[8];
	/* 0xa18 */ SectorRange paintings[6];
)

packed_struct(UyaBonusWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ u64 unused_8[183];
	/* 0x5c0 */ SectorRange credits_text[6];
	/* 0x5f0 */ SectorRange credits_images[13];
	/* 0x658 */ u64 unused_658[115];
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
static void unpack_rac_gc_bonus_wad(
	BonusWadAsset& dest, const Header& header, InputStream& src, BuildConfig config);
template <typename Header>
static void pack_rac_gc_bonus_wad(
	OutputStream& dest, Header& header, const BonusWadAsset& src, BuildConfig config);
static void unpack_rac_gc_credits_text(
	CollectionAsset& dest, InputStream& src, SectorRange range, BuildConfig config);
static SectorRange pack_rac_gc_credits_text(
	OutputStream& dest, const CollectionAsset& src, BuildConfig config);
template <typename Header>
static void unpack_uya_dl_bonus_wad(
	BonusWadAsset& dest, const Header& header, InputStream& src, BuildConfig config);
template <typename Header>
static void pack_uya_dl_bonus_wad(
	OutputStream& dest, Header& header, const BonusWadAsset& src, BuildConfig config);
static void unpack_demo_images(
	CollectionAsset& dest,
	InputStream& src,
	const SectorRange* ranges,
	s32 outer_count,
	s32 inner_count,
	BuildConfig config);
static void pack_demo_images(
	OutputStream& dest,
	SectorRange* ranges,
	s32 outer_count,
	s32 inner_count,
	const CollectionAsset& src,
	BuildConfig config,
	const char* muffin);

on_load(Bonus, []() {
	BonusWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<BonusWadAsset, RacBonusWadHeader>(unpack_rac_gc_bonus_wad<RacBonusWadHeader>);
	BonusWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<BonusWadAsset, GcBonusWadHeader>(unpack_rac_gc_bonus_wad<GcBonusWadHeader>);
	BonusWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<BonusWadAsset, UyaBonusWadHeader>(unpack_uya_dl_bonus_wad<UyaBonusWadHeader>);
	BonusWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<BonusWadAsset, DlBonusWadHeader>(unpack_uya_dl_bonus_wad<DlBonusWadHeader>);
	
	BonusWadAsset::funcs.pack_rac1 = wrap_wad_packer_func<BonusWadAsset, RacBonusWadHeader>(pack_rac_gc_bonus_wad<RacBonusWadHeader>);
	BonusWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<BonusWadAsset, GcBonusWadHeader>(pack_rac_gc_bonus_wad<GcBonusWadHeader>);
	BonusWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<BonusWadAsset, UyaBonusWadHeader>(pack_uya_dl_bonus_wad<UyaBonusWadHeader>);
	BonusWadAsset::funcs.pack_dl = wrap_wad_packer_func<BonusWadAsset, DlBonusWadHeader>(pack_uya_dl_bonus_wad<DlBonusWadHeader>);
})

template <typename Header>
static void unpack_rac_gc_bonus_wad(
	BonusWadAsset& dest, const Header& header, InputStream& src, BuildConfig config)
{
	unpack_compressed_assets<TextureAsset>(dest.goodies_images(SWITCH_FILES), src, ARRAY_PAIR(header.goodies_images), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.character_sketches(SWITCH_FILES), src, ARRAY_PAIR(header.character_sketches), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.character_renders(SWITCH_FILES), src, ARRAY_PAIR(header.character_renders), config, FMT_TEXTURE_PIF8);
	if constexpr(std::is_same_v<Header, GcBonusWadHeader>) {
		unpack_compressed_assets<TextureAsset>(dest.old_skill_images(SWITCH_FILES), src, ARRAY_PAIR(header.old_skill_images), config, FMT_TEXTURE_PIF8);
	} else {
		unpack_compressed_assets<TextureAsset>(dest.skill_images(SWITCH_FILES), src, ARRAY_PAIR(header.skill_images), config, FMT_TEXTURE_PIF8);
	}
	unpack_compressed_assets<TextureAsset>(dest.epilogue_english(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_english), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_french(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_french), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_italian(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_italian), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_german(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_german), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_spanish(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_spanish), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.sketchbook(SWITCH_FILES), src, ARRAY_PAIR(header.sketchbook), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.commercials(SWITCH_FILES), src, ARRAY_PAIR(header.commercials), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.item_images(SWITCH_FILES), src, ARRAY_PAIR(header.item_images), config, FMT_TEXTURE_PIF8);
	if constexpr(std::is_same_v<Header, GcBonusWadHeader>) {
		unpack_rac_gc_credits_text(dest.credits_text(SWITCH_FILES), src, header.credits_text, config);
		unpack_assets<TextureAsset>(dest.credits_images(SWITCH_FILES), src, ARRAY_PAIR(header.credits_images), config, FMT_TEXTURE_RGBA);
		unpack_compressed_assets<TextureAsset>(dest.random_stuff(SWITCH_FILES), src, ARRAY_PAIR(header.random_stuff), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.movie_images(SWITCH_FILES), src, ARRAY_PAIR(header.movie_images), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.cinematic_images(SWITCH_FILES), src, ARRAY_PAIR(header.cinematic_images), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.skill_images(SWITCH_FILES), src, ARRAY_PAIR(header.skill_images), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.clanks_day_at_insomniac(SWITCH_FILES), src, ARRAY_PAIR(header.clanks_day), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.endorsement_deals(SWITCH_FILES), src, ARRAY_PAIR(header.endorsement_deals), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.short_cuts(SWITCH_FILES), src, ARRAY_PAIR(header.short_cuts), config, FMT_TEXTURE_PIF8);
		unpack_compressed_assets<TextureAsset>(dest.paintings(SWITCH_FILES), src, ARRAY_PAIR(header.paintings), config, FMT_TEXTURE_PIF8);
	} else {
		unpack_assets<TextureAsset>(dest.credits_images(SWITCH_FILES), src, ARRAY_PAIR(header.credits_images_ntsc), config, FMT_TEXTURE_RGBA_512_416);
		unpack_assets<TextureAsset>(dest.short_cuts(SWITCH_FILES), src, ARRAY_PAIR(header.credits_images_pal), config, FMT_TEXTURE_RGBA_512_448);
	}
}

template <typename Header>
static void pack_rac_gc_bonus_wad(
	OutputStream& dest, Header& header, const BonusWadAsset& src, BuildConfig config)
{
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.goodies_images), src.get_goodies_images(), config, "goodies_images", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.character_sketches), src.get_character_sketches(), config, "character_sketches", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.character_renders), src.get_character_renders(), config, "character_renders", FMT_TEXTURE_PIF8);
	if constexpr(std::is_same_v<Header, GcBonusWadHeader>) {
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.old_skill_images), src.get_old_skill_images(), config, "old_skill_images", FMT_TEXTURE_PIF8);
	} else {
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.skill_images), src.get_skill_images(), config, "skill_images", FMT_TEXTURE_PIF8);
	}
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.epilogue_english), src.get_epilogue_english(), config, "epilogue_english", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.epilogue_french), src.get_epilogue_french(), config, "epilogue_french", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.epilogue_italian), src.get_epilogue_italian(), config, "epilogue_italian", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.epilogue_german), src.get_epilogue_german(), config, "epilogue_german", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.epilogue_spanish), src.get_epilogue_spanish(), config, "epilogue_spanish", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.sketchbook), src.get_sketchbook(), config, "sketchbook", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.commercials), src.get_commercials(), config, "commercials", FMT_TEXTURE_PIF8);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.item_images), src.get_item_images(), config, "item_images", FMT_TEXTURE_PIF8);
	if constexpr(std::is_same_v<Header, GcBonusWadHeader>) {
		header.credits_text = pack_rac_gc_credits_text(dest, src.get_credits_text(), config);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.credits_images), src.get_credits_images(), config, "credits_images", FMT_TEXTURE_RGBA);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.random_stuff), src.get_random_stuff(), config, "random_stuff", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.movie_images), src.get_movie_images(), config, "movie_images", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.cinematic_images), src.get_cinematic_images(), config, "cinematic_images", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.skill_images), src.get_skill_images(), config, "skill_images", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.clanks_day), src.get_clanks_day_at_insomniac(), config, "clanks_day", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.endorsement_deals), src.get_endorsement_deals(), config, "endorsement_deals", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.short_cuts), src.get_short_cuts(), config, "short_cuts", FMT_TEXTURE_PIF8);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.paintings), src.get_paintings(), config, "paintings", FMT_TEXTURE_PIF8);
	} else {
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.credits_images_ntsc), src.get_credits_images(), config, "credits_images", FMT_TEXTURE_RGBA_512_416);
		pack_compressed_assets_sa(dest, ARRAY_PAIR(header.credits_images_pal), src.get_credits_images_pal(), config, "credits_images", FMT_TEXTURE_RGBA_512_448);
	}
}

static void unpack_rac_gc_credits_text(
	CollectionAsset& dest, InputStream& src, SectorRange range, BuildConfig config)
{
	std::vector<s32> offsets = src.read_multiple<s32>(range.bytes().offset, 8);
	for (s32 i = 0; i < 8; i++) {
		ByteRange inner;
		inner.offset = range.bytes().offset + offsets[i];
		if (i < 7) {
			inner.size = offsets[i + 1] - offsets[i];
		} else {
			inner.size = range.bytes().size - offsets[i];
		}
		unpack_asset(dest.child<BinaryAsset>(i), src, inner, config);
	}
}

static SectorRange pack_rac_gc_credits_text(
	OutputStream& dest, const CollectionAsset& src, BuildConfig config)
{
	dest.pad(SECTOR_SIZE, 0);
	s64 begin_ofs = dest.tell();
	dest.alloc_multiple<s32>(8);
	std::vector<s32> offsets(8, -1);
	for (s32 i = 0; i < 8; i++) {
		if (src.has_child(i)) {
			ByteRange range = {-1, -1};
			offsets[i] = pack_asset(dest, src.get_child(i), config, 0x10, FMT_NO_HINT, &range).offset - (s32) begin_ofs;
		}
	}
	s64 end_ofs = dest.tell();
	dest.seek(begin_ofs);
	dest.write_v(offsets);
	return {Sector32::size_from_bytes(begin_ofs), Sector32::size_from_bytes(end_ofs - begin_ofs)};
}

template <typename Header>
static void unpack_uya_dl_bonus_wad(
	BonusWadAsset& dest, const Header& header, InputStream& src, BuildConfig config)
{
	unpack_assets<BinaryAsset>(dest.credits_text(SWITCH_FILES), src, ARRAY_PAIR(header.credits_text), config);
	unpack_assets<TextureAsset>(dest.credits_images(SWITCH_FILES), src, ARRAY_PAIR(header.credits_images), config, FMT_TEXTURE_RGBA);
	unpack_demo_images(dest.demo_menu(SWITCH_FILES), src, ARRAY_PAIR(header.demo_menu), 30, config);
	unpack_demo_images(dest.demo_exit(SWITCH_FILES), src, ARRAY_PAIR(header.demo_exit), 10, config);
	unpack_assets<TextureAsset>(dest.cheat_images(SWITCH_FILES), src, ARRAY_PAIR(header.cheat_images), config, FMT_TEXTURE_PIF8);
	unpack_assets<TextureAsset>(dest.skill_images(SWITCH_FILES), src, ARRAY_PAIR(header.skill_images), config, FMT_TEXTURE_PIF8);
	unpack_asset(dest.trophy_image<BinaryAsset>(), src, header.trophy_image, config);
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		unpack_asset(dest.dige(), src, header.dige, config);
	}
}

template <typename Header>
void pack_uya_dl_bonus_wad(
	OutputStream& dest, Header& header, const BonusWadAsset& src, BuildConfig config)
{
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_text), src.get_credits_text(), config);
	pack_assets_sa(dest, ARRAY_PAIR(header.credits_images), src.get_credits_images(), config, FMT_TEXTURE_RGBA);
	pack_demo_images(dest, ARRAY_PAIR(header.demo_menu), 30, src.get_demo_menu(), config, "demo_menu");
	pack_demo_images(dest, ARRAY_PAIR(header.demo_exit), 10, src.get_demo_exit(), config, "demo_exit");
	pack_assets_sa(dest, ARRAY_PAIR(header.cheat_images), src.get_cheat_images(), config, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.skill_images), src.get_skill_images(), config, FMT_TEXTURE_PIF8);
	header.trophy_image = pack_asset_sa<SectorRange>(dest, src.get_trophy_image(), config);
	if constexpr(std::is_same_v<Header, DlBonusWadHeader>) {
		header.dige = pack_asset_sa<SectorRange>(dest, src.get_dige(), config);
	}
}

static void unpack_demo_images(
	CollectionAsset& dest,
	InputStream& src,
	const SectorRange* ranges,
	s32 outer_count,
	s32 inner_count,
	BuildConfig config)
{
	for (s32 i = 0; i < outer_count; i++) {
		CollectionAsset& inner = dest.foreign_child<CollectionAsset>(i);
		SubInputStream stream(src, ranges[i].bytes());
		std::vector<s32> offsets = stream.read_multiple<s32>(0, inner_count);
		for (s32 j = 0; j < inner_count; j++) {
			if (offsets[j] > -1) {
				ByteRange range;
				range.offset = offsets[j];
				if (j + 1 < inner_count && offsets[j + 1] > -1) {
					range.size = offsets[j + 1] - offsets[j];
				} else {
					range.size = (s32) stream.size() - offsets[j];
				}
				unpack_compressed_asset(inner.child<TextureAsset>(j), stream, range, config, FMT_TEXTURE_RGBA);
			}
		}
	}
}

static void pack_demo_images(
	OutputStream& dest,
	SectorRange* ranges,
	s32 outer_count,
	s32 inner_count,
	const CollectionAsset& src,
	BuildConfig config,
	const char* muffin)
{
	for (s32 i = 0; i < outer_count; i++) {
		if (src.has_child(i)) {
			const CollectionAsset& inner = src.get_child(i).as<CollectionAsset>();
			dest.pad(SECTOR_SIZE, 0);
			s64 begin_ofs = dest.tell();
			ranges[i].offset = Sector32::size_from_bytes(begin_ofs);
			SubOutputStream stream(dest, begin_ofs);
			stream.alloc_multiple<s32>(inner_count);
			std::vector<s32> offsets(inner_count);
			for (s32 j = 0; j < inner_count; j++) {
				if (inner.has_child(j)) {
					offsets[j] = pack_compressed_asset<ByteRange>(stream, inner.get_child(j).as<TextureAsset>(), config, 0x10, muffin, FMT_TEXTURE_RGBA).offset;
				}
			}
			stream.seek(0);
			stream.write_v(offsets);
			ranges[i].size = Sector32::size_from_bytes(dest.tell() - begin_ofs);
		}
	}
}
