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

#include "settings.h"

#include <gui/book.h>
#include <gui/config.h>

static void folders_page();
static void user_interface_page();
static void level_editor_page();

static const gui::Page GENERAL_PAGES[] = {
	{"Folders", folders_page},
	{"User Interface", user_interface_page}
};

static const gui::Page EDITOR_PAGES[] = {
	{"Level Editor", level_editor_page}
};

static const gui::Chapter SETTINGS[] = {
	{"General", ARRAY_PAIR(GENERAL_PAGES)},
	{"Editor", ARRAY_PAIR(EDITOR_PAGES)}
};

void update_settings_popup() {
	static const gui::Page* page = nullptr;
	gui::BookResult result = gui::book(&page, "Settings##the_popup", ARRAY_PAIR(SETTINGS), gui::BookButtons::OKAY_CANCEL_APPLY);
	switch(result) {
		case gui::BookResult::OKAY: {
			break;
		}
		case gui::BookResult::CANCEL: {
			break;
		}
		case gui::BookResult::APPLY: {
			break;
		}
		case gui::BookResult::NONE: {}
	}
}

static void folders_page() {
	ImGui::InputText("Base Folder", &g_config.folders.base_folder);
	static size_t selection = 0;
	if(ImGui::BeginListBox("Mods Folders")) {
		for(size_t i = 0; i < g_config.folders.mods_folders.size(); i++) {
			ImGui::PushID(i);
			std::string label = g_config.folders.mods_folders[i] + "##selectable";
			if(ImGui::Selectable(label.c_str(), i == selection)) {
				selection = i;
			}
			ImGui::PopID();
		}
		ImGui::EndListBox();
	}
	
	bool selection_is_valid = selection < g_config.folders.mods_folders.size();
	
	static std::string add_path;
	if(ImGui::Button("Add")) {
		add_path = "";
		ImGui::OpenPopup("Add Mod Folder");
	}
	ImGui::SetNextWindowSize(ImVec2(400, -1));
	if(ImGui::BeginPopupModal("Add Mod Folder")) {
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##input", &add_path);
		if(ImGui::Button("Okay")) {
			g_config.folders.mods_folders.push_back(add_path);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	
	static std::string edit_path;
	ImGui::SameLine();
	if(ImGui::Button("Edit") && selection_is_valid) {
		edit_path = g_config.folders.mods_folders[selection];
		ImGui::OpenPopup("Edit Mod Folder");
	}
	ImGui::SetNextWindowSize(ImVec2(400, -1));
	if(ImGui::BeginPopupModal("Edit Mod Folder")) {
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##input", &edit_path);
		if(ImGui::Button("Okay")) {
			g_config.folders.mods_folders[selection] = edit_path;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	
	ImGui::SameLine();
	if(ImGui::Button("Remove") && selection_is_valid) {
		g_config.folders.mods_folders.erase(g_config.folders.mods_folders.begin() + selection);
	}
	
	ImGui::InputText("Cache Folder", &g_config.folders.cache_folder);
}

static void user_interface_page() {
	if(ImGui::BeginListBox("Style")) {
		bool sel = true;
		ImGui::Selectable("Dark", &sel);
		ImGui::Selectable("Light");
		ImGui::EndListBox();
	}
	
	ImGui::Separator();
	ImGui::Checkbox("Custom DPI Scaling", &g_config.ui.custom_scale);
	ImGui::BeginDisabled(!g_config.ui.custom_scale);
	ImGui::SliderFloat("Scale", &g_config.ui.scale, 0.5f, 2.f, "%.1f");
	ImGui::EndDisabled();
	
	ImGui::Separator();
	
	ImGui::Checkbox("Enable Developer Features", &g_config.ui.developer);
}

static void level_editor_page() {
	ImGui::Text("Level editor settings!");
}
