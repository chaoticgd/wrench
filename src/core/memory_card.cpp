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

#include "memory_card.h"

namespace memory_card {

packed_struct(FileHeader,
	s32 game_data_size;
	s32 level_data_size;
)

packed_struct(ChecksumHeader,
	s32 size;
	s32 checksum;
)

packed_struct(SectionHeader,
	s32 type;
	s32 size;
)

File read_save(Buffer src) {
	File save;
	s64 pos = 0;
	
	const FileHeader& file_header = src.read<FileHeader>(pos, "file header");
	pos += sizeof(FileHeader);
	
	save.game = read_sections(&save.checksum_does_not_match, src, pos);
	while(pos + 3 < src.size()) {
		save.levels.emplace_back(read_sections(&save.checksum_does_not_match, src, pos));
	}
	
	return save;
}

std::vector<Section> read_sections(bool* checksum_does_not_match_out, Buffer src, s64& pos) {
	std::vector<Section> sections;
	
	const ChecksumHeader& checksum_header = src.read<ChecksumHeader>(pos, "checksum header");
	pos += sizeof(ChecksumHeader);
	
	u32 check_value = checksum(src.subbuf(pos, checksum_header.size));
	if(check_value != checksum_header.checksum) {
		*checksum_does_not_match_out |= true;
	}
	
	for(;;) {
		const SectionHeader& section_header = src.read<SectionHeader>(pos, "section header");
		pos += sizeof(SectionHeader);
		if(section_header.type == -1) {
			break;
		}
		
		Section& section = sections.emplace_back();
		section.type = section_header.type;
		section.data = src.read_bytes(pos, section_header.size, "section data");
		pos = align64(pos + section_header.size, 4);
	}
	
	return sections;
}

void write(OutBuffer dest, const File& save) {
	
}

u32 checksum(Buffer src) {
	const u8* ptr = src.lo;
	u32 value = 0xedb88320;
	for(const u8* ptr = src.lo; ptr < src.hi; ptr++) {
		value ^= (u32) *ptr << 8;
		for(s32 repeat = 0; repeat < 8; repeat++) {
			if((value & 0x8000) == 0) {
				value <<= 1;
			} else {
				value = (value << 1) ^ 0x1f45;
			}
		}
	}
	return value & 0xffff;
}

SaveGame parse_save(const File& file) {
	SaveGame save;
	for(const Section& section : file.game) {
		Buffer buffer = section.data;
		switch(section.type) {
			case SECTION_LEVEL:                   save.level                      = buffer.read<s32>(0);                             break;
			case SECTION_ELAPSEDTIME:             save.elapsed_time               = buffer.read<s32>(0);                             break;
			case SECTION_LASTSAVETIME:            save.last_save_time             = buffer.read<Clock>(0);                           break;
			case SECTION_GLOBALFLAGS:             save.global_flags               = buffer.read_multiple<u8>(0, 12);                 break;
			case SECTION_CHEATSACTIVATED:         save.cheats_activated           = buffer.read_multiple<u8>(0, 14);                 break;
			case SECTION_SKILLPOINTS:             save.skill_points               = buffer.read_multiple<s32>(0, 15);                break;
			case SECTION_HELPDATAMESSAGES:        save.help_data_messages         = buffer.read_multiple<HelpDatum>(0, 2086).copy(); break;
			case SECTION_HELPDATAMISC:            save.help_data_messages         = buffer.read_multiple<HelpDatum>(0, 16).copy();   break;
			case SECTION_HELPDATAGADGETS:         save.help_data_messages         = buffer.read_multiple<HelpDatum>(0, 20).copy();   break;
			case SECTION_CHEATSEVERACTIVATED:     save.cheats_ever_activated      = buffer.read_multiple<u8>(0, 14);                 break;
			case SECTION_SETTINGS:                save.settings                   = buffer.read<GameSettings>(0);                    break;
			case SECTION_HEROSAVE:                save.hero_save                  = buffer.read<HeroSave>(0);                        break;
			case SECTION_MOVIESPLAYEDRECORD:      save.movies_played_record       = buffer.read_multiple<u32>(0, 64);                break;
			case SECTION_TOTALPLAYTIME:           save.total_play_time            = buffer.read<u32>(0);                             break;
			case SECTION_TOTALDEATHS:             save.total_deaths               = buffer.read<s32>(0);                             break;
			case SECTION_HELPLOG:                 save.help_log                   = buffer.read_multiple<s16>(0, 2100);              break;
			case SECTION_HELPLOGPOS:              save.help_log_pos               = buffer.read<s32>(0);                             break;
			case SECTION_HEROGADGETBOX:           save.hero_gadget_box            = buffer.read<GadgetBox>(0);                       break;
			case SECTION_PURCHASEABLEGADGETS:     save.purchaseable_gadgets       = buffer.read_multiple<u8>(0, 20);                 break;
			case SECTION_BOTSAVE:                 save.bot_save                   = buffer.read<BotSave>(0);                         break;
			case SECTION_FIRSTPERSONDESIREDMODE:  save.first_person_desired_mode  = buffer.read_multiple<s32>(0, 10);                break;
			case SECTION_SAVEDDIFFICULTYLEVEL:    save.saved_difficulty_level     = buffer.read<s32>(0);                             break;
			case SECTION_PLAYERSTATISTICS:        save.player_statistics          = buffer.read_multiple<PlayerData>(0, 2);          break;
			case SECTION_BATTLEDOMEWINSANDLOSSES: save.battledome_wins_and_losses = buffer.read_multiple<s32>(0, 2);                 break;
			case SECTION_ENEMYKILLS:              save.enemy_kills                = buffer.read_multiple<EnemyKillInfo>(0, 30);      break;
			case SECTION_QUICKSWITCHGADGETS:      save.quick_switch_gadgets       = buffer.read<QuickSwitchGadgets>(0);              break;
		}
	}
	return save;
}

const std::vector<FileFormat> FILE_FORMATS = {
	{Game::DL, SAVE, {
		SECTION_LEVEL,
		SECTION_HEROSAVE,
		SECTION_ELAPSEDTIME,
		SECTION_LASTSAVETIME,
		SECTION_TOTALPLAYTIME,
		SECTION_SAVEDDIFFICULTYLEVEL,
		SECTION_GLOBALFLAGS,
		SECTION_CHEATSACTIVATED,
		SECTION_CHEATSEVERACTIVATED,
		SECTION_SKILLPOINTS,
		SECTION_HEROGADGETBOX,
		SECTION_HELPDATAMESSAGES,
		SECTION_HELPDATAMISC,
		SECTION_HELPDATAGADGETS,
		SECTION_SETTINGS,
		SECTION_FIRSTPERSONDESIREDMODE,
		SECTION_MOVIESPLAYEDRECORD,
		SECTION_TOTALDEATHS,
		SECTION_HELPLOG,
		SECTION_HELPLOGPOS,
		SECTION_PURCHASEABLEGADGETS
	}, {
		SECTION_LEVELSAVEDATA
	}}
};


}
