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

#include "hud_wad.h"

#include <spanner/asset_packer.h>

packed_struct(HudWadHeaderDL,
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

HudWadAsset& unpack_hud_wad(AssetPack& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<HudWadHeaderDL>(src);
	AssetFile& asset_file = dest.asset_file("hud/hud.asset");
	
	HudWadAsset& wad = asset_file.root().child<HudWadAsset>("hud");
	wad.set_online_images(unpack_binaries(wad, *file, ARRAY_PAIR(header.online_images), "online_images", ".pif"));
	wad.set_ratchet_seqs(unpack_binaries(wad, *file, ARRAY_PAIR(header.ratchet_seqs), "ratchet_seqs"));
	wad.set_hud_seqs(unpack_binaries(wad, *file, ARRAY_PAIR(header.hud_seqs), "hud_seqs"));
	wad.set_vendor(unpack_binary(wad, *file, header.vendor, "vendor", "vendor.bin"));
	wad.set_all_text(unpack_binaries(wad, *file, ARRAY_PAIR(header.all_text), "all_text"));
	wad.set_hudw3d(unpack_binary(wad, *file, header.hudw3d, "hudw3d", "hudw3d.bin"));
	wad.set_e3_level_ss(unpack_compressed_binaries(wad, *file, ARRAY_PAIR(header.e3_level_ss), "e3_level_ss", ".pif"));
	wad.set_nw_dnas_image(unpack_compressed_binary(wad, *file, header.nw_dnas_image, "nw_dnas_image", "nw_dnas_image.pif"));
	wad.set_split_screen_texture(unpack_binary(wad, *file, header.split_screen_texture, "split_screen_texture", "split_screen_texture.pif"));
	wad.set_radar_maps(unpack_binaries(wad, *file, ARRAY_PAIR(header.radar_maps), "radar_maps"));
	wad.set_weapon_plates_large(unpack_binaries(wad, *file, ARRAY_PAIR(header.weapon_plates_large), "weapon_plates_large", ".pif"));
	wad.set_mission_plates_large(unpack_binaries(wad, *file, ARRAY_PAIR(header.mission_plates_large), "mission_plates_large", ".pif"));
	wad.set_gui_plates(unpack_binaries(wad, *file, ARRAY_PAIR(header.gui_plates), "gui_plates", ".pif"));
	wad.set_vendor_plates(unpack_binaries(wad, *file, ARRAY_PAIR(header.vendor_plates), "vendor_plates", ".pif"));
	wad.set_loading_screen(unpack_binary(wad, *file, header.loading_screen, "loading_screen", "loading_screen.pif"));
	wad.set_planets(unpack_binaries(wad, *file, ARRAY_PAIR(header.planets), "planets", ".pif"));
	wad.set_cinematics(unpack_binaries(wad, *file, ARRAY_PAIR(header.cinematics), "cinematics", ".pif"));
	wad.set_equip_large(unpack_binaries(wad, *file, ARRAY_PAIR(header.equip_large), "equip_large", ".pif"));
	wad.set_equip_small(unpack_binaries(wad, *file, ARRAY_PAIR(header.equip_small), "equip_small", ".pif"));
	wad.set_moves(unpack_binaries(wad, *file, ARRAY_PAIR(header.moves), "moves", ".pif"));
	wad.set_save_level(unpack_binaries(wad, *file, ARRAY_PAIR(header.save_level), "save_level"));
	wad.set_save_empty(unpack_binaries(wad, *file, ARRAY_PAIR(header.save_empty), "save_empty", ".pif"));
	wad.set_skills(unpack_binaries(wad, *file, ARRAY_PAIR(header.skills), "skills", ".pif"));
	wad.set_reward_back(unpack_binary(wad, *file, header.reward_back, "reward_back", "reward_back.pif"));
	wad.set_complete_back(unpack_binary(wad, *file, header.complete_back, "complete_back", "complete_back.pif"));
	wad.set_complete_back_coop(unpack_binary(wad, *file, header.complete_back_coop, "complete_back_coop", "complete_back_coop.pif"));
	wad.set_rewards(unpack_binaries(wad, *file, ARRAY_PAIR(header.rewards), "rewards", ".pif"));
	wad.set_leaderboard(unpack_binary(wad, *file, header.leaderboard, "leaderboard", "leaderboard.pif"));
	wad.set_cutaways(unpack_binaries(wad, *file, ARRAY_PAIR(header.cutaways), "cutaways", ".pif"));
	wad.set_sketchbook(unpack_binaries(wad, *file, ARRAY_PAIR(header.sketchbook), "sketchbook", ".pif"));
	wad.set_character_epilogues(unpack_binaries(wad, *file, ARRAY_PAIR(header.character_epilogues), "character_epilogues", ".pif"));
	wad.set_character_cards(unpack_binaries(wad, *file, ARRAY_PAIR(header.character_cards), "character_cards", ".pif"));
	wad.set_equip_plate(unpack_binary(wad, *file, header.equip_plate, "equip_plate", "equip_plate.pif"));
	wad.set_hud_flythru(unpack_binary(wad, *file, header.hud_flythru, "hud_flythru", "hud_flythru.pif"));
	wad.set_mp_maps(unpack_binaries(wad, *file, ARRAY_PAIR(header.mp_maps), "mp_maps", ".pif"));
	wad.set_tourney_plates_large(unpack_binaries(wad, *file, ARRAY_PAIR(header.tourney_plates_large), "tourney_plates_large", ".pif"));
	
	return wad;
}

void pack_hud_wad(OutputStream& dest, HudWadAsset& wad, Game game) {
	s64 base = dest.tell();
	
	HudWadHeaderDL header = {0};
	header.header_size = sizeof(HudWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.online_images), wad.online_images(), game, base, "online_images", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.ratchet_seqs), wad.ratchet_seqs(), game, base, "ratchet_seqs");
	pack_assets_sa(dest, ARRAY_PAIR(header.hud_seqs), wad.hud_seqs(), game, base, "hud_seqs");
	header.vendor = pack_asset_sa<SectorRange>(dest, wad.vendor(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.all_text), wad.all_text(), game, base, "all_text");
	header.hudw3d = pack_asset_sa<SectorRange>(dest, wad.hudw3d(), game, base);
	compress_assets_sa(dest, ARRAY_PAIR(header.e3_level_ss), wad.e3_level_ss(), game, base, "e3_level_ss", FMT_TEXTURE_PIF_IDTEX8);
	header.nw_dnas_image = compress_asset_sa<SectorRange>(dest, *wad.nw_dnas_image(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.split_screen_texture = pack_asset_sa<SectorRange>(dest, wad.split_screen_texture(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.radar_maps), wad.radar_maps(), game, base, "radar_maps", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.weapon_plates_large), wad.weapon_plates_large(), game, base, "weapon_plates_large", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.mission_plates_large), wad.mission_plates_large(), game, base, "mission_plates_large", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.gui_plates), wad.gui_plates(), game, base, "gui_plates", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.vendor_plates), wad.vendor_plates(), game, base, "vendor_plates", FMT_TEXTURE_PIF_IDTEX8);
	header.loading_screen = pack_asset_sa<SectorRange>(dest, wad.loading_screen(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.planets), wad.planets(), game, base, "planets", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.cinematics), wad.cinematics(), game, base, "cinematics", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.equip_large), wad.equip_large(), game, base, "equip_large", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.equip_small), wad.equip_small(), game, base, "equip_small", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.moves), wad.moves(), game, base, "moves", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.save_level), wad.save_level(), game, base, "save_level", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.save_empty), wad.save_empty(), game, base, "save_empty", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.skills), wad.skills(), game, base, "skills", FMT_TEXTURE_PIF_IDTEX8);
	header.reward_back = pack_asset_sa<SectorRange>(dest, wad.reward_back(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.complete_back = pack_asset_sa<SectorRange>(dest, wad.complete_back(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.complete_back_coop = pack_asset_sa<SectorRange>(dest, wad.complete_back_coop(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.rewards), wad.rewards(), game, base, "rewards", FMT_TEXTURE_PIF_IDTEX8);
	header.leaderboard = pack_asset_sa<SectorRange>(dest, wad.leaderboard(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.cutaways), wad.cutaways(), game, base, "cutaways", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.sketchbook), wad.sketchbook(), game, base, "sketchbook", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.character_epilogues), wad.character_epilogues(), game, base, "character_epilogues", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.character_cards), wad.character_cards(), game, base, "character_cards", FMT_TEXTURE_PIF_IDTEX8);
	header.equip_plate = pack_asset_sa<SectorRange>(dest, wad.equip_plate(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.hud_flythru = pack_asset_sa<SectorRange>(dest, wad.hud_flythru(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.mp_maps), wad.mp_maps(), game, base, "mp_maps", FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.tourney_plates_large), wad.tourney_plates_large(), game, base, "tourney_plates_large", FMT_TEXTURE_PIF_IDTEX8);
	
	dest.write(base, header);
}
