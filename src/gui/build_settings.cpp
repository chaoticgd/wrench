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

#include "build_settings.h"

#include <nfd.h>
#include <gui/gui.h>

void gui::build_settings(
	PackerParams& params,
	const std::vector<std::string>* game_builds,
	const std::vector<std::string>& mod_builds,
	bool launcher)
{
	static size_t selected_build = 0;
	
	// Merge the list of builds from original game and the ones from any
	// enabled mods.
	std::vector<const char*> builds;
	if (game_builds) {
		for (const std::string& build : *game_builds) {
			builds.emplace_back(build.c_str());
		}
	}
	for (const std::string& build : mod_builds) {
		builds.emplace_back(build.c_str());
	}
	
	if (selected_build >= builds.size()) {
		selected_build = 0;
	}
	
	ImGuiStyle& s = ImGui::GetStyle();
	
	std::string combo_text;
	if (builds.empty()) {
		combo_text += "(no builds)";
	} else {
		combo_text += builds[selected_build];
		params.build = builds[selected_build];
	}
	combo_text += " / ";
	if (params.debug.single_level_enabled || params.debug.nompegs) {
		combo_text += "test";
	} else {
		combo_text += "release";
	}
	combo_text += " / ";
	combo_text += params.output_path;
	
	ImGui::SetNextWindowSizeConstraints(ImVec2(400, 0), ImVec2(400, 800));
	if (ImGui::BeginCombo("##build_settings", combo_text.c_str())) {
		ImGui::PushItemWidth(250);
		
		if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
			bool opened = false;
			
			if (builds.empty()) {
				opened = ImGui::BeginCombo("Build Asset", "(no builds)");
			} else {
				opened = ImGui::BeginCombo("Build Asset", builds[selected_build]);
			}
			if (opened) {
				for (size_t i = 0; i < builds.size(); i++) {
					if (ImGui::Selectable(builds[i], i == selected_build)) {
						selected_build = i;
					}
				}
				ImGui::EndCombo();
			}
			
			ImGui::SetNextItemWidth(250 - ImGui::CalcTextSize("Browse").x - s.FramePadding.x * 2 - s.ItemSpacing.x);
			ImGui::InputText("##output_iso", &params.output_path);
			ImGui::SameLine();
			if (ImGui::Button("Browse")) {
				nfdchar_t* path;
				nfdresult_t result = NFD_SaveDialog("iso", nullptr, &path);
				if (result == NFD_OKAY) {
					params.output_path = path;
					free(path);
				}
			}
			ImGui::SameLine();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Output ISO");
			ImGui::Checkbox("Launch emulator after building", &params.launch_emulator);
			ImGui::BeginDisabled(!params.launch_emulator);
			if (launcher) {
				ImGui::Checkbox("Keep launcher window open", &params.keep_window_open);
			}
			ImGui::EndDisabled();
		}
		
		if (ImGui::CollapsingHeader("Testing", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::SetNextItemWidth(-1);
			ImGui::Checkbox("##single_level_enable", &params.debug.single_level_enabled);
			ImGui::SameLine();
			ImGui::BeginDisabled(!params.debug.single_level_enabled);
			ImGui::SetNextItemWidth(250 - ImGui::GetFrameHeight() - s.ItemSpacing.x);
			ImGui::InputText("Single Level", &params.debug.single_level_tag);
			ImGui::EndDisabled();
			ImGui::Checkbox("No MPEG Cutscenes", &params.debug.nompegs);
		}
		
		ImGui::PopItemWidth();
		
		ImGui::EndCombo();
	}
}
