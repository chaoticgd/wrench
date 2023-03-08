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

#ifndef CORE_MEMORY_CARD_H
#define CORE_MEMORY_CARD_H

#include <core/buffer.h>
#include <core/build_config.h>

namespace memory_card {

// *****************************************************************************
// Container format
// *****************************************************************************

struct Section {
	s32 type;
	std::vector<u8> data;
};

struct File {
	bool checksum_does_not_match = false;
	std::vector<Section> game;
	std::vector<std::vector<Section>> levels;
};

File read_save(Buffer src);
std::vector<Section> read_sections(bool* checksum_does_not_match_out, Buffer src, s64& pos);
void write(OutBuffer dest, const File& save);
u32 checksum(Buffer src);

// *****************************************************************************
// Save game
// *****************************************************************************

enum FileType {
	SAVE
};

enum SectionType : s32 {
	SECTION_LEVEL                   = 0,
	SECTION_ELAPSEDTIME             = 3,
	SECTION_LASTSAVETIME            = 4,
	SECTION_GLOBALFLAGS             = 5,
	SECTION_CHEATSACTIVATED         = 7,
	SECTION_SKILLPOINTS             = 8,
	SECTION_HELPDATAMESSAGES        = 10,
	SECTION_HELPDATAMISC            = 11,
	SECTION_HELPDATAGADGETS         = 12,
	SECTION_CHEATSEVERACTIVATED     = 37,
	SECTION_SETTINGS                = 38,
	SECTION_HEROSAVE                = 39,
	SECTION_MOVIESPLAYEDRECORD      = 43,
	SECTION_TOTALPLAYTIME           = 1003,
	SECTION_TOTALDEATHS             = 1005,
	SECTION_HELPLOG                 = 1010,
	SECTION_HELPLOGPOS              = 1011,
	SECTION_HEROGADGETBOX           = 7008,
	SECTION_LEVELSAVEDATA           = 7009,
	SECTION_PURCHASEABLEGADGETS     = 7010,
	SECTION_BOTSAVE                 = 7014,
	SECTION_FIRSTPERSONDESIREDMODE  = 7015,
	SECTION_SAVEDDIFFICULTYLEVEL    = 7016,
	SECTION_PLAYERSTATISTICS        = 7017,
	SECTION_BATTLEDOMEWINSANDLOSSES = 7018,
	SECTION_ENEMYKILLS              = 7019,
	SECTION_QUICKSWITCHGADGETS      = 7020
};

struct FileFormat {
	Game game;
	FileType type;
	std::vector<SectionType> sections;
	std::vector<SectionType> level_sections;
};

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
	/* 0x04 */ u8 help_voice_on;
	/* 0x05 */ u8 help_text_on;
	/* 0x06 */ u8 subtitles_active;
	/* 0x07 */ u8 pad_7;
	/* 0x08 */ s32 stereo;
	/* 0x0c */ s32 music_volume;
	/* 0x10 */ s32 effects_volume;
	/* 0x14 */ s32 voice_volume;
	/* 0x18 */ s32 camera_elevation_dir[3][4];
	/* 0x48 */ s32 camera_azimuth_dir[3][4];
	/* 0x78 */ s32 camera_rotate_speed[3][4];
	/* 0xa8 */ u8 first_person_mode_on[10];
	/* 0xb2 */ u8 was_ntsc_progessive;
	/* 0xb3 */ u8 wide;
	/* 0xb4 */ u8 controller_vibration_on[8];
	/* 0xbc */ u8 quick_select_pause_on;
	/* 0xbd */ u8 language;
	/* 0xbe */ u8 aux_setting_2;
	/* 0xbf */ u8 aux_setting_3;
	/* 0xc0 */ u8 aux_setting_4;
	/* 0xc1 */ u8 auto_save_on;
	/* 0xc2 */ u8 pad_c2[2];
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
	/* 0x1e */ u8 bolt_mult_level;
	/* 0x1f */ u8 bolt_mult_sub_level;
	/* 0x20 */ u8 old_game_save_data;
	/* 0x21 */ u8 blue_badges;
	/* 0x22 */ u8 red_badges;
	/* 0x23 */ u8 green_badges;
	/* 0x24 */ u8 gold_badges;
	/* 0x25 */ u8 black_badges;
	/* 0x26 */ u8 completes;
	/* 0x27 */ u8 last_equipped_gadget[2];
	/* 0x29 */ u8 temp_weapons[4];
	/* 0x2d */ u8 pad_2d[3];
	/* 0x30 */ s32 current_max_limit_break;
	/* 0x34 */ s16 armor_level_2;
	/* 0x36 */ s16 progression_armor_level;
	/* 0x38 */ s32 start_limit_break_diff;
)
static_assert(sizeof(HeroSave) == 0x3c);

packed_struct(GadgetEventMessage,
	/* 0x00 */ s16 gadget_id;
	/* 0x02 */ u8 player_index;
	/* 0x03 */ u8 gadget_event_type;
	/* 0x04 */ u8 extra_data;
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
	/* 0x02 */ u8 gadget_type;
	/* 0x03 */ u8 gadget_event_type;
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
	/* 0x000 */ u8 initialized;
	/* 0x001 */ u8 level;
	/* 0x002 */ u8 button_down[10];
	/* 0x00c */ s16 button_up_frames[10];
	/* 0x020 */ u8 num_gadget_events;
	/* 0x021 */ u8 mod_basic[8];
	/* 0x02a */ s16 mod_post_fx;
	/* 0x02b */ u8 pad_2b;
	/* 0x02c */ u32 p_next_gadget_event;
	/* 0x030 */ GadgetEvent gadget_event_slots[32];
	/* 0xa30 */ GadgetEntry gadgets[32];
)
static_assert(sizeof(GadgetBox) == 0x12b0);

packed_struct(BotSave,
	/* 0x00 */ u8 bot_upgrades[17];
	/* 0x11 */ u8 bot_paintjobs[11];
	/* 0x1c */ u8 bot_heads[8];
	/* 0x24 */ u8 cur_bot_paint_job[2];
	/* 0x26 */ u8 cur_bot_head[2];
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

struct SaveGame {
	bool loaded = false;
	Opt<s32> level;
	Opt<s32> elapsed_time;
	Opt<Clock> last_save_time;
	Opt<FixedArray<u8, 12>> global_flags;
	Opt<FixedArray<u8, 14>> cheats_activated;
	Opt<FixedArray<s32, 15>> skill_points;
	Opt<std::vector<HelpDatum>> help_data_messages;
	Opt<std::vector<HelpDatum>> help_data_misc;
	Opt<std::vector<HelpDatum>> help_data_gadgets;
	Opt<FixedArray<u8, 14>> cheats_ever_activated;
	Opt<GameSettings> settings;
	Opt<HeroSave> hero_save;
	Opt<FixedArray<u32, 64>> movies_played_record;
	Opt<u32> total_play_time;
	Opt<s32> total_deaths;
	Opt<FixedArray<s16, 2100>> help_log;
	Opt<s32> help_log_pos;
	Opt<GadgetBox> hero_gadget_box;
	Opt<FixedArray<u8, 20>> purchaseable_gadgets;
	Opt<BotSave> bot_save;
	Opt<FixedArray<s32, 10>> first_person_desired_mode;
	Opt<s32> saved_difficulty_level;
	Opt<FixedArray<PlayerData, 2>> player_statistics;
	Opt<FixedArray<s32, 2>> battledome_wins_and_losses;
	Opt<FixedArray<EnemyKillInfo, 30>> enemy_kills;
	Opt<QuickSwitchGadgets> quick_switch_gadgets;
};


SaveGame parse_save(const File& file);

extern const std::vector<FileFormat> FILE_FORMATS;

}

#endif
