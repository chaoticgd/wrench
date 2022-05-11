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

void unpack_hud_wad(HudWadAsset& dest, InputStream& src, Game game);
static void pack_hud_wad(OutputStream& dest, std::vector<u8>* header_dest, HudWadAsset& src, Game game);

on_load(Hud, []() {
	HudWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<HudWadAsset>(unpack_hud_wad);
	
	HudWadAsset::funcs.pack_dl = wrap_wad_packer_func<HudWadAsset>(pack_hud_wad);
})

packed_struct(DeadlockedHudWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange online_images[74];
	/* 0x258 */ SectorRange ratchet_seqs[28];
	/* 0x338 */ SectorRange hud_seqs[20];
	/* 0x3d8 */ SectorRange vendor;
	/* 0x3e0 */ SectorRange all_text[8];
	/* 0x420 */ SectorRange hudw3d;
	/* 0x428 */ SectorRange e3_level_ss[10];
	/* 0x478 */ SectorRange nw_dnas_image;
	/* 0x480 */ SectorRange split_screen_texture;
	/* 0x488 */ SectorRange radar_maps[15];
	/* 0x500 */ SectorRange weapon_plates_large[20];
	/* 0x5a0 */ SectorRange mission_plates_large[15];
	/* 0x618 */ SectorRange gui_plates[23];
	/* 0x6d0 */ SectorRange vendor_plates[46];
	/* 0x840 */ SectorRange loading_screen;
	/* 0x848 */ SectorRange planets[16];
	/* 0x8c8 */ SectorRange cinematics[21];
	/* 0x970 */ SectorRange equip_large[24];
	/* 0xa30 */ SectorRange equip_small[5];
	/* 0xa58 */ SectorRange moves[15];
	/* 0xad0 */ SectorRange save_level[16];
	/* 0xb50 */ SectorRange save_empty[4];
	/* 0xb70 */ SectorRange skills[26];
	/* 0xc40 */ SectorRange reward_back;
	/* 0xc48 */ SectorRange complete_back;
	/* 0xc50 */ SectorRange complete_back_coop;
	/* 0xc58 */ SectorRange rewards[26];
	/* 0xd28 */ SectorRange leaderboard;
	/* 0xd30 */ SectorRange cutaways[7];
	/* 0xd68 */ SectorRange sketchbook[34];
	/* 0xe78 */ SectorRange character_epilogues[6];
	/* 0xea8 */ SectorRange character_cards[7];
	/* 0xee0 */ SectorRange equip_plate;
	/* 0xee8 */ SectorRange hud_flythru;
	/* 0xef0 */ SectorRange mp_maps[15];
	/* 0xf68 */ SectorRange tourney_plates_large[4];
)

void unpack_hud_wad(HudWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedHudWadHeader>(0);
	
	unpack_assets<TextureAsset>(dest.online_images().switch_files(), src, ARRAY_PAIR(header.online_images), game);
	unpack_assets<BinaryAsset>(dest.ratchet_seqs().switch_files(), src, ARRAY_PAIR(header.ratchet_seqs), game);
	unpack_assets<BinaryAsset>(dest.hud_seqs().switch_files(), src, ARRAY_PAIR(header.hud_seqs), game);
	unpack_asset(dest.vendor(), src, header.vendor, game);
	unpack_assets<BinaryAsset>(dest.all_text().switch_files(), src, ARRAY_PAIR(header.all_text), game);
	unpack_asset(dest.hudw3d(), src, header.hudw3d, game);
	unpack_compressed_assets<TextureAsset>(dest.e3_level_ss().switch_files(), src, ARRAY_PAIR(header.e3_level_ss), game);
	unpack_compressed_asset(dest.nw_dnas_image<TextureAsset>(), src, header.nw_dnas_image, game);
	unpack_asset(dest.split_screen_texture<TextureAsset>(), src, header.split_screen_texture, game);
	unpack_assets<TextureAsset>(dest.radar_maps().switch_files(), src, ARRAY_PAIR(header.radar_maps), game);
	unpack_assets<TextureAsset>(dest.weapon_plates_large().switch_files(), src, ARRAY_PAIR(header.weapon_plates_large), game);
	unpack_assets<TextureAsset>(dest.mission_plates_large().switch_files(), src, ARRAY_PAIR(header.mission_plates_large), game);
	unpack_assets<TextureAsset>(dest.gui_plates().switch_files(), src, ARRAY_PAIR(header.gui_plates), game);
	unpack_assets<TextureAsset>(dest.vendor_plates().switch_files(), src, ARRAY_PAIR(header.vendor_plates), game);
	unpack_asset(dest.loading_screen<TextureAsset>(), src, header.loading_screen, game);
	unpack_assets<TextureAsset>(dest.planets().switch_files(), src, ARRAY_PAIR(header.planets), game);
	unpack_assets<TextureAsset>(dest.cinematics().switch_files(), src, ARRAY_PAIR(header.cinematics), game);
	unpack_assets<TextureAsset>(dest.equip_large().switch_files(), src, ARRAY_PAIR(header.equip_large), game);
	unpack_assets<TextureAsset>(dest.equip_small().switch_files(), src, ARRAY_PAIR(header.equip_small), game);
	unpack_assets<TextureAsset>(dest.moves().switch_files(), src, ARRAY_PAIR(header.moves), game);
	unpack_assets<TextureAsset>(dest.save_level().switch_files(), src, ARRAY_PAIR(header.save_level), game);
	unpack_assets<TextureAsset>(dest.save_empty().switch_files(), src, ARRAY_PAIR(header.save_empty), game);
	unpack_assets<TextureAsset>(dest.skills().switch_files(), src, ARRAY_PAIR(header.skills), game);
	unpack_asset(dest.reward_back<TextureAsset>(), src, header.reward_back, game);
	unpack_asset(dest.complete_back<TextureAsset>(), src, header.complete_back, game);
	unpack_asset(dest.complete_back_coop<TextureAsset>(), src, header.complete_back_coop, game);
	unpack_assets<TextureAsset>(dest.rewards().switch_files(), src, ARRAY_PAIR(header.rewards), game);
	unpack_asset(dest.leaderboard<TextureAsset>(), src, header.leaderboard, game);
	unpack_assets<TextureAsset>(dest.cutaways().switch_files(), src, ARRAY_PAIR(header.cutaways), game);
	unpack_assets<TextureAsset>(dest.sketchbook().switch_files(), src, ARRAY_PAIR(header.sketchbook), game);
	unpack_assets<TextureAsset>(dest.character_epilogues().switch_files(), src, ARRAY_PAIR(header.character_epilogues), game);
	unpack_assets<TextureAsset>(dest.character_cards().switch_files(), src, ARRAY_PAIR(header.character_cards), game);
	unpack_asset(dest.equip_plate<TextureAsset>(), src, header.equip_plate, game);
	unpack_asset(dest.hud_flythru<TextureAsset>(), src, header.hud_flythru, game);
	unpack_assets<TextureAsset>(dest.mp_maps().switch_files(), src, ARRAY_PAIR(header.mp_maps), game);
	unpack_assets<TextureAsset>(dest.tourney_plates_large().switch_files(), src, ARRAY_PAIR(header.tourney_plates_large), game);
}

static void pack_hud_wad(OutputStream& dest, std::vector<u8>* header_dest, HudWadAsset& src, Game game) {
	DeadlockedHudWadHeader header = {0};
	header.header_size = sizeof(DeadlockedHudWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.online_images), src.get_online_images(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.ratchet_seqs), src.get_ratchet_seqs(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.hud_seqs), src.get_hud_seqs(), game);
	header.vendor = pack_asset_sa<SectorRange>(dest, src.get_vendor(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.all_text), src.get_all_text(), game);
	header.hudw3d = pack_asset_sa<SectorRange>(dest, src.get_hudw3d(), game);
	pack_compressed_assets_sa(dest, ARRAY_PAIR(header.e3_level_ss), src.get_e3_level_ss(), game,FMT_TEXTURE_PIF8);
	header.nw_dnas_image = pack_compressed_asset_sa<SectorRange>(dest, src.get_nw_dnas_image(), game, FMT_TEXTURE_PIF8);
	header.split_screen_texture = pack_asset_sa<SectorRange>(dest, src.get_split_screen_texture(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.radar_maps), src.get_radar_maps(), game, FMT_TEXTURE_PIF4_SWIZZLED);
	pack_assets_sa(dest, ARRAY_PAIR(header.weapon_plates_large), src.get_weapon_plates_large(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.mission_plates_large), src.get_mission_plates_large(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.gui_plates), src.get_gui_plates(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.vendor_plates), src.get_vendor_plates(), game, FMT_TEXTURE_PIF8);
	header.loading_screen = pack_asset_sa<SectorRange>(dest, src.get_loading_screen(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.planets), src.get_planets(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.cinematics), src.get_cinematics(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.equip_large), src.get_equip_large(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.equip_small), src.get_equip_small(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.moves), src.get_moves(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.save_level), src.get_save_level(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.save_empty), src.get_save_empty(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.skills), src.get_skills(), game, FMT_TEXTURE_PIF8);
	header.reward_back = pack_asset_sa<SectorRange>(dest, src.get_reward_back(), game, FMT_TEXTURE_PIF8);
	header.complete_back = pack_asset_sa<SectorRange>(dest, src.get_complete_back(), game, FMT_TEXTURE_PIF8);
	header.complete_back_coop = pack_asset_sa<SectorRange>(dest, src.get_complete_back_coop(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.rewards), src.get_rewards(), game, FMT_TEXTURE_PIF8);
	header.leaderboard = pack_asset_sa<SectorRange>(dest, src.get_leaderboard(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.cutaways), src.get_cutaways(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.sketchbook), src.get_sketchbook(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.character_epilogues), src.get_character_epilogues(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.character_cards), src.get_character_cards(), game, FMT_TEXTURE_PIF8);
	header.equip_plate = pack_asset_sa<SectorRange>(dest, src.get_equip_plate(), game, FMT_TEXTURE_PIF8);
	header.hud_flythru = pack_asset_sa<SectorRange>(dest, src.get_hud_flythru(), game, FMT_TEXTURE_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.mp_maps), src.get_mp_maps(), game, FMT_TEXTURE_PIF4_SWIZZLED);
	pack_assets_sa(dest, ARRAY_PAIR(header.tourney_plates_large), src.get_tourney_plates_large(), game, FMT_TEXTURE_PIF8);
	
	dest.write(0, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}
