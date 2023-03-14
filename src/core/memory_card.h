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
	ST_9                    = 9,
	ST_10                   = 10,
	ST_11                   = 11,
	ST_12                   = 12,
	ST_13                   = 13,
	ST_14                   = 14,
	ST_15                   = 15,
	ST_HELPDATAMESSAGES     = 16,
	ST_HELPDATAMISC         = 17,
	ST_HELPDATAGADGETS      = 18,
	ST_20                   = 20,
	ST_30                   = 30,
	ST_32                   = 32,
	ST_CHEATSEVERACTIVATED  = 37,
	ST_SETTINGS             = 38,
	ST_HEROSAVE             = 39,
	ST_40                   = 40,
	ST_41                   = 41,
	ST_42                   = 42,
	ST_44                   = 44,
	ST_45                   = 45,
	ST_46                   = 46,
	ST_47                   = 47,
	ST_MOVIESPLAYEDRECORD   = 43,
	ST_TOTALPLAYTIME        = 1003,
	ST_TOTALDEATHS          = 1005,
	ST_HELPLOG              = 1010,
	ST_HELPLOGPOS           = 1011,
	ST_GAMEMODEOPTIONS      = 7000,
	ST_MPPROFILES           = 7001,
	ST_7002                 = 7002,
	ST_7003                 = 7003,
	ST_7004                 = 7004,
	ST_7005                 = 7005,
	ST_7006                 = 7006,
	ST_7007                 = 7007,
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
	ST_QUICKSWITCHGADGETS   = 7020,
};

template <u8 games, typename Value>
struct GameOpt {
	Opt<Value> data;
	using value_type = Value;
	static const constexpr u8 valid_games = games;
	bool check(u8 current_game) const { return valid_games & current_game; }
	Value& operator*() {
		verify(data.has_value(), "Tried to access GameOpt without a value.");
		return *data;
	}
	const Value& operator*() const {
		verify(data.has_value(), "Tried to access GameOpt without a value.");
		return *data;
	}
	Value* operator->() {
		verify(data.has_value(), "Tried to access GameOpt without a value.");
		return &(*data);
	}
};

// For sections where the structure differs for each game, this macro is used
// to generate an accessor function template to retrieve a reference to the
// structure corresponding to a particular game.
#define MULTI_GAME_ACCESSOR(container, name, rac_field, gc_field, uya_field, dl_field) \
	template <typename T> \
	T& name() { \
		if constexpr(std::is_same_v<T, typename decltype(container().rac_field)::value_type>) return *rac_field; \
		if constexpr(std::is_same_v<T, typename decltype(container().gc_field)::value_type>) return *gc_field; \
		if constexpr(std::is_same_v<T, typename decltype(container().uya_field)::value_type>) return *uya_field; \
		if constexpr(std::is_same_v<T, typename decltype(container().dl_field)::value_type>) return *dl_field; \
		assert_not_reached("Multi-game accessor called with bad template parameter."); \
	}

struct LevelSaveGame {
	GameOpt<RAC|GC|UYA|DL, LevelSave> level;
};

struct DummyType {
	using value_type = void;
	void* operator*() { return nullptr; }
};

struct SaveGame {
	bool loaded = false;
	FileType type;
	u8 game = 0;
	DummyType _no_field; // Dummy field used for MULTI_GAME_ACCESSOR.
	// net
	GameOpt<RAC|GC|UYA|DL, GameModeStruct> game_mode_options;
	GameOpt<RAC|GC|UYA|DL, FixedArray<ProfileStruct, 8>> mp_profiles;
	// slot
	GameOpt<UYA|DL, s32> level;
	GameOpt<UYA|DL, s32> elapsed_time;
	GameOpt<UYA|DL, Clock> last_save_time;
	GameOpt<    DL, FixedArray<u8, 12>> global_flags;
	GameOpt<    DL, FixedArray<u8, 14>> cheats_activated;
	GameOpt<    DL, FixedArray<s32, 15>> skill_points;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_9;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_10;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_11;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_12;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_13;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_14;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_15;
	GameOpt<    DL, FixedArray<HelpDatum, 2088>> help_data_messages;
	GameOpt<    DL, FixedArray<HelpDatum, 16>> help_data_misc;
	GameOpt<    DL, FixedArray<HelpDatum, 20>> help_data_gadgets;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_20;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_30;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_32;
	GameOpt<    DL, FixedArray<u8, 14>> cheats_ever_activated;
	GameOpt<    DL, GameSettings> settings;
	GameOpt<UYA   , UyaHeroSave> hero_save_uya;
	GameOpt<    DL, DlHeroSave> hero_save_dl;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_40;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_41;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_42;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_44;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_45;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_46;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_47;
	GameOpt<UYA|DL, FixedArray<u32, 64>> movies_played_record;
	GameOpt<UYA|DL, u32> total_play_time;
	GameOpt<UYA|DL, s32> total_deaths;
	GameOpt<    DL, FixedArray<s16, 2100>> help_log;
	GameOpt<    DL, s32> help_log_pos;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_7002;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_7003;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_7004;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_7005;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_7006;
	GameOpt<UYA   , FixedArray<u8, 123>> unknown_7007;
	GameOpt<    DL, GadgetBox> hero_gadget_box;
	GameOpt<    DL, FixedArray<u8, 20>> purchaseable_gadgets;
	GameOpt<    DL, FixedArray<u8, 17>> purchaseable_bot_upgrades;
	GameOpt<    DL, u32> purchaseable_wrench_level;
	GameOpt<    DL, FixedArray<u8, 9>> purchaseable_post_fx_mods;
	GameOpt<    DL, BotSave> bot_save;
	GameOpt<    DL, FixedArray<s32, 10>> first_person_desired_mode;
	GameOpt<    DL, s32> saved_difficulty_level;
	GameOpt<    DL, FixedArray<PlayerData, 2>> player_statistics;
	GameOpt<    DL, FixedArray<s32, 2>> battledome_wins_and_losses;
	GameOpt<    DL, FixedArray<EnemyKillInfo, 30>> enemy_kills;
	GameOpt<    DL, QuickSwitchGadgets> quick_switch_gadgets;
	std::vector<LevelSaveGame> levels;
	MULTI_GAME_ACCESSOR(SaveGame, hero_save, _no_field, _no_field, hero_save_uya, hero_save_dl)
};

struct FileFormat {
	GameBitfield game;
	FileType type;
	std::vector<SectionType> sections;
	std::vector<SectionType> level_sections;
};

SaveGame parse(const File& file);
void update(File& dest, const SaveGame& save);
SaveGame parse_net(const File& file);
void update_net(File& dest, const SaveGame& save);
SaveGame parse_slot(const File& file);
void update_slot(File& dest, const SaveGame& save);
GameBitfield identify_game(const File& file);
const char* section_type(u32 type);

extern const std::vector<FileFormat> FILE_FORMATS;

}

#endif
