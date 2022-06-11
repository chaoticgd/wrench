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

#include "game_list.h"

#include <core/stream.h>
#include <core/filesystem.h>
#include <assetmgr/game_info.h>
#include <gui/gui.h>

struct GameData {
	std::string directory;
	GameInfo info;
};

static GameData* get_game();

static std::vector<GameData> games;
static size_t current_game = 0;

GameResult game_list() {
	std::string name;
	if(GameData* game = get_game()) {
		name = game->info.name + " (" + game->directory + ")";
	} else if(games.empty()) {
		name = "(has no games)";
	} else {
		name = "(select game)";
	}
	if(ImGui::BeginCombo("##game", name.c_str())) {
		for(size_t i = 0; i < games.size(); i++) {
			GameData& game = games[i];
			std::string option_name = game.info.name + " (" + game.directory + ")";
			if(ImGui::Selectable(option_name.c_str(), i == current_game)) {
				current_game = i;
			}
		}
		ImGui::EndCombo();
	}
	return {nullptr, nullptr};
}

void load_game_list(const std::string& games_folder) {
	free_game_list();
	
	for(fs::directory_entry game_dir : fs::directory_iterator(games_folder)) {
		FileInputStream stream;
		if(stream.open(game_dir.path()/"gameinfo.txt")) {
			std::vector<u8> game_info_txt = stream.read_multiple<u8>(stream.size());
			strip_carriage_returns(game_info_txt);
			game_info_txt.push_back(0);
			
			GameData& game = games.emplace_back();
			game.directory = game_dir.path();
			game.info = read_game_info((char*) game_info_txt.data());
		}
	}
}

void free_game_list() {
	games.clear();
}

static GameData* get_game() {
	if(current_game >= games.size()) {
		return nullptr;
	}
	return &games[current_game];
}
