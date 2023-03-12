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
#include <core/memory_card_structs.h>

namespace memory_card {

// *****************************************************************************
// Container format
// *****************************************************************************

struct Section {
	s32 offset;
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
	ST_HELPDATAMESSAGES     = 16,
	ST_HELPDATAMISC         = 17,
	ST_HELPDATAGADGETS      = 18,
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
	ST_PURCHASEABLEBOTUPGRD = 7011,
	ST_PURCHASEABLEWRENCH   = 7012,
	ST_PURCHASEABLEPOSTMODS = 7013,
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
	Opt<FixedArray<u8, 17>> purchaseable_bot_upgrades;
	Opt<u8> purchaseable_wrench_level;
	Opt<FixedArray<u8, 9>> purchaseable_post_fx_mods;
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
const char* section_type(u32 type);

extern const std::vector<FileFormat> FILE_FORMATS;

}

#endif
