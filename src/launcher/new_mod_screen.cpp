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

#include "new_mod_screen.h"

#include <core/shell.h>
#include <core/stream.h>
#include <assetmgr/game_info.h>
#include <assetmgr/asset_types.h>
#include <gui/config.h>

bool new_mod_screen()
{
	bool result = false;
	
	ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(400, 170), ImGuiCond_Always);
	
	if (ImGui::BeginPopupModal("New Mod##the_popup", nullptr, ImGuiWindowFlags_NoResize)) {
		static size_t mods_folder = 0;
		static std::string folder;
		static GameInfo info;
		static bool open_folder = true;
		
		if (g_config.paths.mods_folders.size() >= 1) {
			bool drop_down;
			if (mods_folder < g_config.paths.mods_folders.size()) {
				drop_down = ImGui::BeginCombo("Parent Folder", g_config.paths.mods_folders[mods_folder].c_str());
			} else {
				drop_down = ImGui::BeginCombo("Parent Folder", "(select mods folder)");
			}
			
			if (drop_down) {
				for (size_t i = 0; i < g_config.paths.mods_folders.size(); i++) {
					if (ImGui::Selectable(g_config.paths.mods_folders[i].c_str())) {
						mods_folder = i;
					}
				}
				ImGui::EndCombo();
			}
			
			ImGui::InputText("Folder Name", &folder);
			ImGui::InputText("Display Name", &info.name);
			
			if (mods_folder < g_config.paths.mods_folders.size() && ImGui::Button("Create")) {
				info.format_version = ASSET_FORMAT_VERSION;
				info.type = AssetBankType::MOD;
				info.mod.supported_games = {Game::RAC, Game::GC, Game::UYA, Game::DL};
				
				std::string text;
				write_game_info(text, info);
				
				fs::path path = fs::path(g_config.paths.mods_folders[mods_folder])/folder;
				fs::create_directories(path);
				FileOutputStream stream;
				stream.open(path/"gameinfo.txt");
				stream.write_n((u8*) text.data(), text.size());
				
				if (open_folder) {
					open_in_file_manager(path.string().c_str());
				}
				
				mods_folder = 0;
				folder = "";
				info = {};
				ImGui::CloseCurrentPopup();
				
				result = true;
			}
		} else {
			ImGui::Text("Error: No mod folders set.");
			ImGui::NewLine();
		}
		
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			mods_folder = 0;
			folder = "";
			info = {};
			ImGui::CloseCurrentPopup();
		}
		
		ImGui::SameLine();
#ifdef _WIN32
		ImGui::Checkbox("Open in Explorer", &open_folder);
#else
		ImGui::Checkbox("Open in File Manager", &open_folder);
#endif
		
		ImGui::EndPopup();
	}
	
	return result;
}
