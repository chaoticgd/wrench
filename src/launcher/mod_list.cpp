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

static std::vector<Mod> mods;
static size_t current_mod = 0;
static std::vector<GlTexture> images;

ModResult mod_list(const std::string& filter) {
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
		
		for(size_t i = 0; i < mods.size(); i++) {
			Mod& mod = mods[i];
			
			std::string mod_name_lowercase = mod.game_info.name;
			for(char& c : mod_name_lowercase) c = tolower(c);
			
			if(mod_name_lowercase.find(filter_lower) != std::string::npos) {
				ImGui::PushID(i);
				
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				ImGui::SetCursorPosX(ImGui::GetStyle().FramePadding.x);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
				ImGui::Checkbox("##enabled", &mod.selected);
				ImGui::PopStyleVar();
				
				ImGui::TableNextColumn();
				ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns;
				if(ImGui::Selectable(mod.game_info.name.c_str(), i == current_mod, selectable_flags)) {
					current_mod = i;
				}
				
				if(author_column) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", mod.game_info.author.c_str());
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
	
	return {};
}

void load_mod_list(const std::vector<std::string>& mods_folders) {
	free_mod_list();
	
	for(std::string mods_dir : mods_folders) {
		for(fs::directory_entry mod_dir : fs::directory_iterator(mods_dir)) {
			FileInputStream stream;
			if(stream.open(mod_dir.path()/"gameinfo.txt")) {
				std::vector<u8> game_info_txt = stream.read_multiple<u8>(stream.size());
				strip_carriage_returns(game_info_txt);
				game_info_txt.push_back(0);
				
				Mod& mod = mods.emplace_back();
				mod.game_info = read_game_info((char*) game_info_txt.data());
				mod.absolute_path = fs::absolute(mod_dir.path());
				
				for(std::string& image_path : mod.game_info.images) {
					//GlTexture& texture = mod.images.emplace_back();
					//
					//FileInputStream image_stream;
					//if(image_stream.open(mod_dir.path()/image_path)) {
					//	Opt<Texture> image = read_png(image_stream);
					//	if(image.has_value()) {
					//		image->to_rgba();
					//		
					//		GlTexture texture;
					//		glGenTextures(1, &texture.id);
					//		glBindTexture(GL_TEXTURE_2D, texture.id);
					//		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, *(s32*) &image[0], *(s32*) &image[4], 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[16]);
					//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
					//		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					//	}
					//}
				}
			}
		}
	}
	
	std::sort(BEGIN_END(mods), [](const Mod& lhs, const Mod& rhs)
		{ return lhs.game_info.name < rhs.game_info.name; });
}

void free_mod_list() {
	mods.clear();
	images.clear();
}
