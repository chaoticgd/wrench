/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef MEMORY_CARD_STRUCTS_H
#define MEMORY_CARD_STRUCTS_H

#include <core/util.h>

namespace memory_card {

packed_struct(SiegeMatch,
	/* 0x000 */ s32 time_limit;
	/* 0x004 */ u8 nodes_on;
	/* 0x005 */ u8 ais_on;
	/* 0x006 */ u8 vehicles_on;
	/* 0x007 */ u8 friendlyfire_on;
)
static_assert(sizeof(SiegeMatch) == 0x8);

packed_struct(TimeDeathMatch,
	/* 0x000 */ s32 time_limit;
	/* 0x004 */ u8 vehicles_on;
	/* 0x005 */ u8 friendly_fire_on;
	/* 0x006 */ u8 suicide_on;
	/* 0x007 */ u8 pad_7;
)
static_assert(sizeof(TimeDeathMatch) == 0x8);

packed_struct(FragDeathMatch,
	/* 0x000 */ s32 frag_limit;
	/* 0x004 */ u8 vechicles_on;
	/* 0x005 */ u8 suicide_on;
	/* 0x006 */ u8 friendly_fire_on;
	/* 0x007 */ u8 pad_7;
)
static_assert(sizeof(FragDeathMatch) == 0x8);

packed_struct(GameModeStruct,
	/* 0x000 */ s32 mode_chosen;
	/* 0x004 */ SiegeMatch siege_options;
	/* 0x00c */ TimeDeathMatch time_death_match_options;
	/* 0x014 */ FragDeathMatch frag_death_match_options;
)
static_assert(sizeof(GameModeStruct) == 0x1c);

packed_struct(GeneralStatStruct,
	/* 0x000 */ s32 no_of_games_played;
	/* 0x004 */ s32 no_of_games_won;
	/* 0x008 */ s32 no_of_games_lost;
	/* 0x00c */ s32 no_of_kills;
	/* 0x010 */ s32 no_of_deaths;
)
static_assert(sizeof(GeneralStatStruct) == 0x14);

packed_struct(SiegeMatchStatStruct,
	/* 0x000 */ s32 no_of_wins;
	/* 0x004 */ s32 no_of_losses;
	/* 0x008 */ s32 wins_per_level[6];
	/* 0x020 */ s32 losses_per_level[6];
	/* 0x038 */ s32 no_of_base_captures;
	/* 0x03c */ s32 no_of_kills;
	/* 0x040 */ s32 no_of_deaths;
)
static_assert(sizeof(SiegeMatchStatStruct) == 0x44);

packed_struct(DeadMatchStatStruct,
	/* 0x000 */ s32 no_of_wins;
	/* 0x004 */ s32 no_of_losses;
	/* 0x008 */ s32 wins_per_level[6];
	/* 0x020 */ s32 losses_per_level[6];
	/* 0x038 */ s32 no_of_kills;
	/* 0x03c */ s32 no_of_deaths;
)
static_assert(sizeof(DeadMatchStatStruct) == 0x40);

packed_struct(CameraMode,
	/* 0x000 */ u8 normal_left_right_mode;
	/* 0x001 */ u8 normal_up_down_mode;
	/* 0x002 */ u8 pad_2[2];
	/* 0x004 */ s32 camera_speed;
)
static_assert(sizeof(CameraMode) == 0x8);

packed_struct(ProfileStruct,
	/* 0x000 */ s32 skin;
	/* 0x004 */ CameraMode camera_options[3];
	/* 0x01c */ u8 first_person_mode_on;
	/* 0x01d */ s8 name[16];
	/* 0x02d */ s8 password[16];
	/* 0x03d */ s8 map_access;
	/* 0x03e */ s8 pal_server;
	/* 0x03f */ s8 help_msg_off;
	/* 0x040 */ s8 save_password;
	/* 0x041 */ s8 location_idx;
	/* 0x042 */ u8 pad_42[2];
	/* 0x044 */ GeneralStatStruct general_stats;
	/* 0x058 */ SiegeMatchStatStruct siege_match_stats;
	/* 0x09c */ DeadMatchStatStruct dead_match_stats;
	/* 0x0dc */ s8 active;
	/* 0x0dd */ u8 pad_dd[3];
	/* 0x0e0 */ s32 help_data[32];
	/* 0x160 */ s8 net_enabled;
	/* 0x161 */ s8 vibration;
	/* 0x162 */ s16 music_volume;
	/* 0x164 */ s32 extra_data_padding[31];
)
static_assert(sizeof(ProfileStruct) == 0x1e0);

packed_struct(Clock,
	u8 stat;
	u8 second;
	u8 minute;
	u8 hour;
	u8 pad;
	u8 day;
	u8 month;
	u8 year;
)
static_assert(sizeof(Clock) == 0x8);

packed_struct(HelpDatum,
	/* 0x0 */ u16 times_used;
	/* 0x2 */ u16 counter;
	/* 0x4 */ u32 last_time;
	/* 0x8 */ u32 level_die;
)
static_assert(sizeof(HelpDatum) == 0xc);

packed_struct(GameSettings,
	/* 0x00 */ s32 pal_mode;
	/* 0x04 */ s8 help_voice_on;
	/* 0x05 */ s8 help_text_on;
	/* 0x06 */ s8 subtitles_active;
	/* 0x07 */ u8 pad_7;
	/* 0x08 */ s32 stereo;
	/* 0x0c */ s32 music_volume;
	/* 0x10 */ s32 effects_volume;
	/* 0x14 */ s32 voice_volume;
	/* 0x18 */ s32 camera_elevation_dir[3][4];
	/* 0x48 */ s32 camera_azimuth_dir[3][4];
	/* 0x78 */ s32 camera_rotate_speed[3][4];
	/* 0xa8 */ u8 first_person_mode_on[10];
	/* 0xb2 */ s8 was_ntsc_progessive;
	/* 0xb3 */ s8 wide;
	/* 0xb4 */ s8 controller_vibration_on[8];
	/* 0xbc */ s8 quick_select_pause_on;
	/* 0xbd */ s8 language;
	/* 0xbe */ s8 aux_setting_2;
	/* 0xbf */ s8 aux_setting_3;
	/* 0xc0 */ s8 aux_setting_4;
	/* 0xc1 */ s8 auto_save_on;
	/* 0xc2 */ s8 pad_c2[2];
)
static_assert(sizeof(GameSettings) == 0xc4);

packed_struct(HeroSave,
	/* 0x00 */ s32 bolts;
	/* 0x04 */ s32 bolt_deficit;
	/* 0x08 */ s32 xp;
	/* 0x0c */ s32 points;
	/* 0x10 */ s16 hero_max_hp;
	/* 0x12 */ s16 armor_level;
	/* 0x14 */ f32 limit_break;
	/* 0x18 */ s32 purchased_skins;
	/* 0x1c */ s16 spent_diff_stars;
	/* 0x1e */ s8 bolt_mult_level;
	/* 0x1f */ s8 bolt_mult_sub_level;
	/* 0x20 */ s8 old_game_save_data;
	/* 0x21 */ s8 blue_badges;
	/* 0x22 */ s8 red_badges;
	/* 0x23 */ s8 green_badges;
	/* 0x24 */ s8 gold_badges;
	/* 0x25 */ s8 black_badges;
	/* 0x26 */ s8 completes;
	/* 0x27 */ s8 last_equipped_gadget[2];
	/* 0x29 */ s8 temp_weapons[4];
	/* 0x2d */ u8 pad_2d[3];
	/* 0x30 */ s32 current_max_limit_break;
	/* 0x34 */ s16 armor_level_2;
	/* 0x36 */ s16 progression_armor_level;
	/* 0x38 */ s32 start_limit_break_diff;
)
static_assert(sizeof(HeroSave) == 0x3c);

packed_struct(GadgetEventMessage,
	/* 0x00 */ s16 gadget_id;
	/* 0x02 */ s8 player_index;
	/* 0x03 */ s8 gadget_event_type;
	/* 0x04 */ s8 extra_data;
	/* 0x05 */ u8 pad_5[3];
	/* 0x08 */ s32 active_time;
	/* 0x0c */ u32 target_uid;
	/* 0x10 */ f32 firing_loc[3];
	/* 0x1c */ f32 target_dir[3];
)
static_assert(sizeof(GadgetEventMessage) == 0x28);

packed_struct(GadgetEvent,
	/* 0x00 */ u8 gadget_id;
	/* 0x01 */ u8 player_index;
	/* 0x02 */ s8 gadget_type;
	/* 0x03 */ s8 gadget_event_type;
	/* 0x04 */ s32 active_time;
	/* 0x08 */ u32 target_uid;
	/* 0x0c */ u8 pad_c[4];
	/* 0x10 */ f32 target_offset_quat[4];
	/* 0x20 */ u32 p_next_gadget_event;
	/* 0x24 */ GadgetEventMessage gadget_event_msg;
	/* 0x4c */ u8 pad_4c[4];
)
static_assert(sizeof(GadgetEvent) == 0x50);

enum ModBasicType : u32 {
	MOD_BSC_UNDEFINED = 0,
	MOD_BSC_SPEED = 1,
	MOD_BSC_AMMO = 2,
	MOD_BSC_AIMING = 3,
	MOD_BSC_IMPACT = 4,
	MOD_BSC_AREA = 5,
	MOD_BSC_XP = 6,
	MOD_BSC_JACKPOT = 7,
	MOD_BSC_NANOLEECH = 8,
	TOTAL_BASIC_MODS = 9,
	TOTAL_MOD_BSC_DEFS_SIZE = 9
};

enum ModPostFXType : u32 {
	MOD_PFX_UNDEFINED = 0,
	MOD_PFX_NAPALM = 1,
	MOD_PFX_ELECTRICDOOM = 2,
	MOD_PFX_FREEZING = 3,
	MOD_PFX_BOMBLETS = 4,
	MOD_PFX_MORPHING = 5,
	MOD_PFX_INFECTION = 6,
	MOD_PFX_PLAGUE = 7,
	MOD_PFX_SHOCK = 8,
	TOTAL_POSTFX_MODS = 9,
	TOTAL_MOD_PFX_DEFS_SIZE = 9
};

enum ModWeaponType : u32 {
	MOD_WPN_UNDEFINED = 0,
	MOD_WPN_ROCKET_GUIDANCE = 1,
	MOD_WPN_SHOTGUN_WIDTH = 2,
	MOD_WPN_SHOTGUN_LENGTH = 3,
	MOD_WPN_GRENADE_MANUALDET = 4,
	MOD_WPN_MACHGUN_BEAM = 5,
	MOD_WPN_SNIPER_PIERCING = 6,
	TOTAL_WPN_MODS = 7,
	TOTAL_MOD_WPN_DEFS_SIZE = 7
};

packed_struct(GadgetEntry,
	/* 0x00 */ s16 level;
	/* 0x02 */ s16 ammo;
	/* 0x04 */ u32 xp;
	/* 0x08 */ s32 action_frame;
	/* 0x0c */ s32 mod_active_post_fx;
	/* 0x10 */ s32 mod_active_weapon;
	/* 0x14 */ s32 mod_active_basic[10];
	/* 0x3c */ s32 mod_weapon[2];
)
static_assert(sizeof(GadgetEntry) == 0x44);

packed_struct(GadgetBox,
	/* 0x000 */ s8 initialized;
	/* 0x001 */ s8 level;
	/* 0x002 */ s8 button_down[10];
	/* 0x00c */ s16 button_up_frames[10];
	/* 0x020 */ s8 num_gadget_events;
	/* 0x021 */ s8 mod_basic[8];
	/* 0x02a */ s16 mod_post_fx;
	/* 0x02b */ s8 pad_2b;
	/* 0x02c */ u32 p_next_gadget_event;
	/* 0x030 */ GadgetEvent gadget_event_slots[32];
	/* 0xa30 */ GadgetEntry gadgets[32];
)
static_assert(sizeof(GadgetBox) == 0x12b0);

packed_struct(BotSave,
	/* 0x00 */ s8 bot_upgrades[17];
	/* 0x11 */ s8 bot_paintjobs[11];
	/* 0x1c */ s8 bot_heads[8];
	/* 0x24 */ s8 cur_bot_paint_job[2];
	/* 0x26 */ s8 cur_bot_head[2];
)
static_assert(sizeof(BotSave) == 0x28);

packed_struct(PlayerData,
	/* 0x000 */ u32 health_received;
	/* 0x004 */ u32 damage_received;
	/* 0x008 */ u32 ammo_received;
	/* 0x00c */ u32 time_charge_booting;
	/* 0x010 */ u32 num_deaths;
	/* 0x014 */ u32 weapon_kills[20];
	/* 0x064 */ f32 weapon_kill_percentage[20];
	/* 0x0b4 */ u32 ammo_used[20];
	/* 0x104 */ u32 shots_that_hit[20];
	/* 0x154 */ u32 shots_that_miss[20];
	/* 0x1a4 */ f32 shot_accuracy[20];
	/* 0x1f4 */ u32 func_mod_kills[9];
	/* 0x218 */ u32 func_mod_used[9];
	/* 0x23c */ u32 time_spent_in_vehicles[4];
	/* 0x24c */ u32 kills_with_vehicle_weaps[4];
	/* 0x25c */ u32 kills_from_vehicle_squashing[4];
	/* 0x26c */ u32 kills_while_in_vehicle;
	/* 0x270 */ u32 vehicle_shots_that_hit[4];
	/* 0x280 */ u32 vehicle_shots_that_miss[4];
	/* 0x290 */ f32 vehicle_shot_accuracy[4];
)
static_assert(sizeof(PlayerData) == 0x2a0);

packed_struct(EnemyKillInfo,
	/* 0x0 */ s32 o_class;
	/* 0x4 */ s32 kills;
)
static_assert(sizeof(EnemyKillInfo) == 0x8);

packed_struct(QuickSwitchGadgets,
	s32 array[4][3];
)
static_assert(sizeof(QuickSwitchGadgets) == 0x30);

packed_struct(MissionSave,
	/* 0x000 */ s32 xp;
	/* 0x004 */ s32 bolts;
	/* 0x008 */ u8 status;
	/* 0x009 */ u8 completes;
	/* 0x00a */ u8 difficulty;
	/* 0x00b */ u8 pad_b;
)
static_assert(sizeof(MissionSave) == 0xc);

packed_struct(LevelSave,
	/* 0x000 */ MissionSave mission[64];
	/* 0x300 */ u8 status;
	/* 0x301 */ u8 jackpot;
	/* 0x302 */ u8 pad_302[2];
)
static_assert(sizeof(LevelSave) == 0x304);

}

#endif
