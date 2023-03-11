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
#include <core/filesystem.h>
#include <core/build_config.h>

namespace memory_card {

// *****************************************************************************
// Container format
// *****************************************************************************

struct Section {
	s32 type;
	s32 unpadded_size;
	std::vector<u8> data;
};

enum class FileType {
	MAIN,
	NET,
	PATCH,
	SLOT,
	SYS
};

struct File {
	fs::path path;
	bool checksum_does_not_match = false;
	FileType type;
	struct {
		std::vector<u8> data;
	} main;
	struct {
		std::vector<Section> sections;
	} net;
	struct {
		std::vector<u8> data;
	} patch;
	struct {
		std::vector<Section> sections;
		std::vector<std::vector<Section>> levels;
	} slot;
	struct {
		std::vector<u8> data;
	} sys;
};

File read(Buffer src, const fs::path& path);
FileType identify(std::string filename);
std::vector<Section> read_sections(bool* checksum_does_not_match_out, Buffer src, s64& pos);
void write(OutBuffer dest, File& file);
s64 write_sections(OutBuffer dest, std::vector<Section>& sections);
u32 checksum(Buffer src);

// *****************************************************************************
// Save game
// *****************************************************************************

enum SectionType : s32 {
	ST_LEVEL                = 0,
	ST_ELAPSEDTIME          = 3,
	ST_LASTSAVETIME         = 4,
	ST_GLOBALFLAGS          = 5,
	ST_CHEATSACTIVATED      = 7,
	ST_SKILLPOINTS          = 8,
	ST_HELPDATAMESSAGES     = 10,
	ST_HELPDATAMISC         = 11,
	ST_HELPDATAGADGETS      = 12,
	ST_CHEATSEVERACTIVATED  = 37,
	ST_SETTINGS             = 38,
	ST_HEROSAVE             = 39,
	ST_MOVIESPLAYEDRECORD   = 43,
	ST_TOTALPLAYTIME        = 1003,
	ST_TOTALDEATHS          = 1005,
	ST_HELPLOG              = 1010,
	ST_HELPLOGPOS           = 1011,
	ST_GAMEMODEOPTIONS      = 7000,
	ST_MPPROFILES           = 7001,
	ST_HEROGADGETBOX        = 7008,
	ST_LEVELSAVEDATA        = 7009,
	ST_PURCHASEABLEGADGETS  = 7010,
	ST_BOTSAVE              = 7014,
	ST_FIRSTPERSONMODE      = 7015,
	ST_SAVEDDIFFICULTYLEVEL = 7016,
	ST_PLAYERSTATISTICS     = 7017,
	ST_BATTLEDOMEWINSLOSSES = 7018,
	ST_ENEMYKILLS           = 7019,
	ST_QUICKSWITCHGADGETS   = 7020
};

struct FileFormat {
	Game game;
	FileType type;
	std::vector<SectionType> sections;
	std::vector<SectionType> level_sections;
};

packed_struct(SiegeMatch,
	/* 0x000 */ s32 time_limit;
	/* 0x004 */ bool nodes_on;
	/* 0x005 */ bool ais_on;
	/* 0x006 */ bool vehicles_on;
	/* 0x007 */ bool friendlyfire_on;
)
static_assert(sizeof(SiegeMatch) == 0x8);

packed_struct(TimeDeathMatch,
	/* 0x000 */ s32 time_limit;
	/* 0x004 */ bool vehicles_on;
	/* 0x005 */ bool friendly_fire_on;
	/* 0x006 */ bool suicide_on;
	/* 0x007 */ u8 pad_7;
)
static_assert(sizeof(TimeDeathMatch) == 0x8);

packed_struct(FragDeathMatch,
	/* 0x000 */ s32 frag_limit;
	/* 0x004 */ bool vechicles_on;
	/* 0x005 */ bool suicide_on;
	/* 0x006 */ bool friendly_fire_on;
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
	/* 0x03c */ s32 noof_deaths;
)
static_assert(sizeof(DeadMatchStatStruct) == 0x40);

packed_struct(CameraMode,
	/* 0x000 */ bool normal_left_right_mode;
	/* 0x001 */ bool normal_up_down_mode;
	/* 0x002 */ u8 pad_2[2];
	/* 0x004 */ s32 camera_speed;
)
static_assert(sizeof(CameraMode) == 0x8);

packed_struct(ProfileStruct,
	/* 0x000 */ s32 skin;
	/* 0x004 */ CameraMode camera_options[3];
	/* 0x01c */ u8 first_person_mode_on;
	/* 0x01d */ char name[16];
	/* 0x02d */ char password[16];
	/* 0x03d */ u8 map_access;
	/* 0x03e */ u8 pal_server;
	/* 0x03f */ u8 help_msg_off;
	/* 0x040 */ u8 save_password;
	/* 0x041 */ u8 location_idx;
	/* 0x042 */ u8 pad_42[2];
	/* 0x044 */ GeneralStatStruct general_stats;
	/* 0x058 */ SiegeMatchStatStruct siege_match_stats;
	/* 0x09c */ DeadMatchStatStruct dead_match_stats;
	/* 0x0dc */ u8 active;
	/* 0x0dd */ u8 pad_dd[3];
	/* 0x0e0 */ s32 help_data[32];
	/* 0x160 */ u8 net_enabled;
	/* 0x161 */ u8 vibration;
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

struct LevelSaveGame {
	Opt<LevelSave> level;
};

struct SaveGame {
	bool loaded = false;
	FileType type;
	// net
	Opt<GameModeStruct> game_mode_options;
	Opt<FixedArray<ProfileStruct, 8>> mp_profiles;
	// slot
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
	std::vector<LevelSaveGame> levels;
};


SaveGame parse(const File& file);
SaveGame parse_net(const File& file);
SaveGame parse_slot(const File& file);
void update(File& dest, const SaveGame& save);
void update_net(File& dest, const SaveGame& save);
void update_slot(File& dest, const SaveGame& save);

extern const std::vector<FileFormat> FILE_FORMATS;

}

#endif
