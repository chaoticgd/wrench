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

#include <engine/vag.h>
#include <iso/table_of_contents.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_rac_global_wad(GlobalWadAsset& dest, const RacWadInfo& header, InputStream& src, BuildConfig config);
static void pack_rac_global_wad(OutputStream& dest, RacWadInfo& header, const GlobalWadAsset& src, BuildConfig config);

static void unpack_vags(CollectionAsset& dest, InputStream& src, const Sector32* sectors, s32 count, BuildConfig config);

on_load(Bonus, []() {
	GlobalWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<GlobalWadAsset, RacWadInfo>(unpack_rac_global_wad);
	
	GlobalWadAsset::funcs.pack_rac1 = wrap_wad_packer_func<GlobalWadAsset, RacWadInfo>(pack_rac_global_wad);
})

static void unpack_rac_global_wad(GlobalWadAsset& dest, const RacWadInfo& header, InputStream& src, BuildConfig config) {
	unpack_asset(dest.debug_font<TextureAsset>(), src, header.debug_font, config, FMT_TEXTURE_PIF8);
	unpack_asset(dest.save_game(), src, header.save_game, config);
	unpack_compressed_assets<BinaryAsset>(dest.ratchet_seqs(SWITCH_FILES), src, ARRAY_PAIR(header.ratchet_seqs), config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_seqs(SWITCH_FILES), src, ARRAY_PAIR(header.hud_seqs), config);
	unpack_asset(dest.vendor(), src, header.vendor, config);
	unpack_assets<BinaryAsset>(dest.vendor_audio(SWITCH_FILES), src, ARRAY_PAIR(header.vendor_audio), config, FMT_BINARY_VAG);
	unpack_assets<TextureAsset>(dest.help_controls(SWITCH_FILES), src, ARRAY_PAIR(header.help_controls), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.help_moves(SWITCH_FILES), src, ARRAY_PAIR(header.help_moves), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.help_weapons(SWITCH_FILES), src, ARRAY_PAIR(header.help_weapons), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.help_gadgets(SWITCH_FILES), src, ARRAY_PAIR(header.help_gadgets), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.help_items(SWITCH_FILES), src, ARRAY_PAIR(header.help_items), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.unknown_images(SWITCH_FILES), src, ARRAY_PAIR(header.unknown_images), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.options_menu(SWITCH_FILES), src, ARRAY_PAIR(header.options_menu), config, FMT_TEXTURE_PIF8);
	unpack_asset<BinaryAsset>(dest.frontbin(SWITCH_FILES), src, header.frontbin, config);
	unpack_compressed_assets<TextureAsset>(dest.stuff(SWITCH_FILES), src, ARRAY_PAIR(header.stuff), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.planets(SWITCH_FILES), src, ARRAY_PAIR(header.planets), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<BinaryAsset>(dest.stuff2(SWITCH_FILES), src, ARRAY_PAIR(header.stuff2), config);
	unpack_compressed_assets<TextureAsset>(dest.goodies_images(SWITCH_FILES), src, ARRAY_PAIR(header.goodies_images), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.character_sketches(SWITCH_FILES), src, ARRAY_PAIR(header.character_sketches), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.character_renders(SWITCH_FILES), src, ARRAY_PAIR(header.character_renders), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.skill_images(SWITCH_FILES), src, ARRAY_PAIR(header.skill_images), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_english(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_english), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_french(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_french), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_italian(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_italian), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_german(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_german), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.epilogue_spanish(SWITCH_FILES), src, ARRAY_PAIR(header.epilogue_spanish), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.sketchbook(SWITCH_FILES), src, ARRAY_PAIR(header.sketchbook), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.commercials(SWITCH_FILES), src, ARRAY_PAIR(header.commercials), config, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.item_images(SWITCH_FILES), src, ARRAY_PAIR(header.item_images), config, FMT_TEXTURE_PIF8);
	unpack_asset(dest.irx(), src, header.irx, config);
	unpack_asset(dest.sound_bank(), src, header.sound_bank, config);
	unpack_asset(dest.wad_14e0(), src, header.wad_14e0, config);
	unpack_asset(dest.music(), src, header.music, config, FMT_BINARY_VAG);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
	unpack_asset(dest.post_credits_helpdesk_girl_seq(), src, header.post_credits_helpdesk_girl_seq, config);
	unpack_assets<BinaryAsset>(dest.post_credits_audio(SWITCH_FILES), src, ARRAY_PAIR(header.post_credits_audio), config, FMT_BINARY_VAG);
	unpack_assets<TextureAsset>(dest.credits_images_ntsc(SWITCH_FILES), src, ARRAY_PAIR(header.credits_images_ntsc), config, FMT_TEXTURE_RGBA_512_416);
	unpack_assets<TextureAsset>(dest.credits_images_pal(SWITCH_FILES), src, ARRAY_PAIR(header.credits_images_pal), config, FMT_TEXTURE_RGBA_512_448);
	unpack_compressed_assets<BinaryAsset>(dest.wad_things(SWITCH_FILES), src, ARRAY_PAIR(header.wad_things), config);
	unpack_assets<BinaryAsset>(dest.mpegs(SWITCH_FILES), src, ARRAY_PAIR(header.mpegs), config, FMT_BINARY_PSS);
	unpack_vags(dest.help_audio(SWITCH_FILES), src, ARRAY_PAIR(header.help_audio), config);
	unpack_vags(dest.qwark_boss_audio(SWITCH_FILES), src, ARRAY_PAIR(header.qwark_boss_audio), config);
	unpack_assets<BinaryAsset>(dest.mobies(SWITCH_FILES), src, ARRAY_PAIR(header.mobies), config);
	unpack_assets<BinaryAsset>(dest.anim_looking_thing_2(SWITCH_FILES), src, ARRAY_PAIR(header.anim_looking_thing_2), config);
	unpack_compressed_assets<CollectionAsset>(dest.pif_lists(SWITCH_FILES), src, ARRAY_PAIR(header.pif_lists), config, FMT_COLLECTION_PIF8);
	unpack_asset(dest.transition(), src, header.transition, config);
	unpack_assets<BinaryAsset>(dest.space_sounds(SWITCH_FILES), src, ARRAY_PAIR(header.space_sounds), config, FMT_BINARY_VAG);
	unpack_assets<BinaryAsset>(dest.things(SWITCH_FILES), src, ARRAY_PAIR(header.things), config);
}

static void pack_rac_global_wad(OutputStream& dest, RacWadInfo& header, const GlobalWadAsset& src, BuildConfig config) {
	// debug_font
	// save_game
	// ratchet_seqs
	// hud_seqs
	// vendor
	// vendor_audio
	// help_controls
	// help_moves
	// help_weapons
	// help_gadgets
	// help_items
	// unknown_images
	// options_menu
	// stuff
	// planets
	// stuff2
	// goodies_images
	// character_sketches
	// character_renders
	// skill_images
	// epilogue_english
	// epilogue_french
	// epilogue_italian
	// epilogue_german
	// epilogue_spanish
	// sketchbook
	// commercials
	// item_images
	// irx
	// sound_bank
	// wad_14e0
	// music
	// hud_header
	// hud_banks
	// all_text_english
	// post_credits_helpdesk_girl_seq
	// post_credits_audio
	// credits_images_ntsc
	// credits_images_pal
	// wad_things
	// mpegs
	// vags5
	// vags2
	// mobies
	// anim_looking_thing_2
	// pif_lists
	// transition
	// space_sounds
	// things
}

static void unpack_vags(CollectionAsset& dest, InputStream& src, const Sector32* sectors, s32 count, BuildConfig config) {
	for(s32 i = 0; i < count; i++) {
		if(!sectors[i].empty()) {
			Sector32 size = get_vag_size(src, sectors[i]);
			SectorRange range{sectors[i], size};
			unpack_asset(dest.child<BinaryAsset>(i), src, range, config, FMT_BINARY_VAG);
		}
	}
}
