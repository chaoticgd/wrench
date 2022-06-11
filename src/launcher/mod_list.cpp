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

#include "mod_list.h"

#include <core/png.h>
#include <core/stream.h>
#include <core/filesystem.h>
#include <gui/config.h>
#include <gui/gui_state.h>

void mod_list(const std::string& filter) {
	std::string filter_lower = filter;
	for(char& c : filter_lower) c = tolower(c);
	
	ImGuiTableFlags flags = ImGuiTableFlags_RowBg;
	
	static bool author_column = false;
	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	if(ImGui::BeginTable("mods", 2 + author_column, flags)) {
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		if(author_column) {
			ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthStretch);
		}
		ImGui::TableHeadersRow();
		
		std::vector<gui::StateNode>& mods = g_gui.subnodes("mods");
		for(size_t i = 0; i < mods.size(); i++) {
			auto mod = Mod(mods[i]);
			
			std::string mod_name_lowercase = mod.name();
			for(char& c : mod_name_lowercase) c = tolower(c);
			
			if(mod_name_lowercase.find(filter_lower) != std::string::npos) {
				ImGui::PushID(i);
				
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				ImGui::SetCursorPosX(ImGui::GetStyle().FramePadding.x);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
				ImGui::Checkbox("##enabled", &mod.enabled());
				ImGui::PopStyleVar();
				
				ImGui::TableNextColumn();
				ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns;
				s32& selected_mod = g_gui.integer("selected_mod");
				if(ImGui::Selectable(mod.name().c_str(), i == selected_mod, selectable_flags)) {
					selected_mod = i;
					g_gui.boolean("mod_images_dirty") = true;
				}
				
				if(author_column) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", mod.author().c_str());
				}
				
				ImGui::PopID();
			}
		}
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	
	if(ImGui::BeginPopupContextItem("mods")) {
		ImGui::MenuItem("Author", nullptr, &author_column);
		ImGui::EndPopup();
	}
}

void load_mod_list(const std::vector<std::string>& mods_folders) {
	free_mod_list();
	
	auto& mods = g_gui.subnodes("mods");
	mods.clear();
	
	for(std::string mods_dir : mods_folders) {
		for(fs::directory_entry mod_dir : fs::directory_iterator(mods_dir)) {
			FileInputStream stream;
			if(stream.open(mod_dir.path()/"gameinfo.txt")) {
				std::vector<u8> game_info_txt = stream.read_multiple<u8>(stream.size());
				strip_carriage_returns(game_info_txt);
				game_info_txt.push_back(0);
				
				GameInfo info = read_game_info((char*) game_info_txt.data());
				
				auto mod = Mod(mods.emplace_back());
				mod.path() = mod_dir.path().string();
				mod.name() = info.name;
				mod.author() = info.author;
				mod.description() = info.description;
				mod.version() = info.version;
				mod.images() = info.images;
			}
		}
	}
	
	std::sort(BEGIN_END(g_gui.subnodes("mods")), [](gui::StateNode& lhs, gui::StateNode& rhs)
		{ return Mod(lhs).name() < Mod(rhs).name(); });
	
	g_gui.integer("selected_mod") = INT32_MAX;
}

void free_mod_list() {
	g_gui.subnodes("mods").clear();
}
