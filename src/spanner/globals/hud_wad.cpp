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

void unpack_hud_wad(HudWadAsset& dest, BinaryAsset& src) {
	auto [file, header] = open_wad_file<HudWadHeaderDL>(src);
	
	unpack_binaries(dest.online_images().switch_files(), *file, ARRAY_PAIR(header.online_images), ".pif");
	unpack_binaries(dest.ratchet_seqs().switch_files(), *file, ARRAY_PAIR(header.ratchet_seqs));
	unpack_binaries(dest.hud_seqs().switch_files(), *file, ARRAY_PAIR(header.hud_seqs));
	unpack_binary(dest.vendor(), *file, header.vendor, "vendor.bin");
	unpack_binaries(dest.all_text().switch_files(), *file, ARRAY_PAIR(header.all_text));
	unpack_binary(dest.hudw3d(), *file, header.hudw3d, "hudw3d.bin");
	unpack_compressed_binaries(dest.e3_level_ss().switch_files(), *file, ARRAY_PAIR(header.e3_level_ss), ".pif");
	unpack_compressed_binary(dest.nw_dnas_image(), *file, header.nw_dnas_image, "nw_dnas_image.pif");
	unpack_binary(dest.split_screen_texture(), *file, header.split_screen_texture, "split_screen_texture.pif");
	unpack_binaries(dest.radar_maps().switch_files(), *file, ARRAY_PAIR(header.radar_maps), "radar_maps");
	unpack_binaries(dest.weapon_plates_large().switch_files(), *file, ARRAY_PAIR(header.weapon_plates_large), ".pif");
	unpack_binaries(dest.mission_plates_large().switch_files(), *file, ARRAY_PAIR(header.mission_plates_large), ".pif");
	unpack_binaries(dest.gui_plates().switch_files(), *file, ARRAY_PAIR(header.gui_plates), ".pif");
	unpack_binaries(dest.vendor_plates().switch_files(), *file, ARRAY_PAIR(header.vendor_plates), ".pif");
	unpack_binary(dest.loading_screen(), *file, header.loading_screen, "loading_screen.pif");
	unpack_binaries(dest.planets().switch_files(), *file, ARRAY_PAIR(header.planets), ".pif");
	unpack_binaries(dest.cinematics().switch_files(), *file, ARRAY_PAIR(header.cinematics), ".pif");
	unpack_binaries(dest.equip_large().switch_files(), *file, ARRAY_PAIR(header.equip_large), ".pif");
	unpack_binaries(dest.equip_small().switch_files(), *file, ARRAY_PAIR(header.equip_small), ".pif");
	unpack_binaries(dest.moves().switch_files(), *file, ARRAY_PAIR(header.moves), ".pif");
	unpack_binaries(dest.save_level().switch_files(), *file, ARRAY_PAIR(header.save_level));
	unpack_binaries(dest.save_empty().switch_files(), *file, ARRAY_PAIR(header.save_empty), ".pif");
	unpack_binaries(dest.skills().switch_files(), *file, ARRAY_PAIR(header.skills), ".pif");
	unpack_binary(dest.reward_back(), *file, header.reward_back, "reward_back.pif");
	unpack_binary(dest.complete_back(), *file, header.complete_back, "complete_back.pif");
	unpack_binary(dest.complete_back_coop(), *file, header.complete_back_coop, "complete_back_coop.pif");
	unpack_binaries(dest.rewards().switch_files(), *file, ARRAY_PAIR(header.rewards), ".pif");
	unpack_binary(dest.leaderboard().switch_files(), *file, header.leaderboard, "leaderboard.pif");
	unpack_binaries(dest.cutaways().switch_files(), *file, ARRAY_PAIR(header.cutaways), ".pif");
	unpack_binaries(dest.sketchbook().switch_files(), *file, ARRAY_PAIR(header.sketchbook), ".pif");
	unpack_binaries(dest.character_epilogues().switch_files(), *file, ARRAY_PAIR(header.character_epilogues), ".pif");
	unpack_binaries(dest.character_cards().switch_files(), *file, ARRAY_PAIR(header.character_cards), ".pif");
	unpack_binary(dest.equip_plate(), *file, header.equip_plate, "equip_plate.pif");
	unpack_binary(dest.hud_flythru(), *file, header.hud_flythru, "hud_flythru.pif");
	unpack_binaries(dest.mp_maps().switch_files(), *file, ARRAY_PAIR(header.mp_maps), ".pif");
	unpack_binaries(dest.tourney_plates_large().switch_files(), *file, ARRAY_PAIR(header.tourney_plates_large), ".pif");
}

void pack_hud_wad(OutputStream& dest, HudWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	HudWadHeaderDL header = {0};
	header.header_size = sizeof(HudWadHeaderDL);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_assets_sa(dest, ARRAY_PAIR(header.online_images), src.get_online_images(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.ratchet_seqs), src.get_ratchet_seqs(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.hud_seqs), src.get_hud_seqs(), game, base);
	header.vendor = pack_asset_sa<SectorRange>(dest, src.get_vendor(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.all_text), src.get_all_text(), game, base);
	header.hudw3d = pack_asset_sa<SectorRange>(dest, src.get_hudw3d(), game, base);
	compress_assets_sa(dest, ARRAY_PAIR(header.e3_level_ss), src.get_e3_level_ss(), game, base,FMT_TEXTURE_PIF_IDTEX8);
	header.nw_dnas_image = compress_asset_sa<SectorRange>(dest, src.get_nw_dnas_image(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.split_screen_texture = pack_asset_sa<SectorRange>(dest, src.get_split_screen_texture(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.radar_maps), src.get_radar_maps(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.weapon_plates_large), src.get_weapon_plates_large(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.mission_plates_large), src.get_mission_plates_large(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.gui_plates), src.get_gui_plates(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.vendor_plates), src.get_vendor_plates(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.loading_screen = pack_asset_sa<SectorRange>(dest, src.get_loading_screen(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.planets), src.get_planets(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.cinematics), src.get_cinematics(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.equip_large), src.get_equip_large(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.equip_small), src.get_equip_small(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.moves), src.get_moves(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.save_level), src.get_save_level(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.save_empty), src.get_save_empty(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.skills), src.get_skills(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.reward_back = pack_asset_sa<SectorRange>(dest, src.get_reward_back(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.complete_back = pack_asset_sa<SectorRange>(dest, src.get_complete_back(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.complete_back_coop = pack_asset_sa<SectorRange>(dest, src.get_complete_back_coop(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.rewards), src.get_rewards(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.leaderboard = pack_asset_sa<SectorRange>(dest, src.get_leaderboard(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.cutaways), src.get_cutaways(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.sketchbook), src.get_sketchbook(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.character_epilogues), src.get_character_epilogues(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.character_cards), src.get_character_cards(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.equip_plate = pack_asset_sa<SectorRange>(dest, src.get_equip_plate(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	header.hud_flythru = pack_asset_sa<SectorRange>(dest, src.get_hud_flythru(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.mp_maps), src.get_mp_maps(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	pack_assets_sa(dest, ARRAY_PAIR(header.tourney_plates_large), src.get_tourney_plates_large(), game, base, FMT_TEXTURE_PIF_IDTEX8);
	
	dest.write(base, header);
}
