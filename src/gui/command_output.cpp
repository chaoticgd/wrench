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

#include "command_output.h"

#include <nfd.h>

void gui::command_output_screen(
	const char* id, CommandThread& command, void (*close_callback)(), void (*run_callback)())
{
	ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(640, 480), ImGui::GetMainViewport()->Size);
	
	ImGuiStyle& s = ImGui::GetStyle();
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 0;
	size.y -= s.WindowPadding.y * 2 + 64;
	
	if (ImGui::BeginPopupModal(id)) {
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextMultiline("output", &command.get_last_output_lines(), size, ImGuiInputTextFlags_Multiline | ImGuiInputTextFlags_ReadOnly);
		ImGui::GetCurrentContext()->InputTextState.ReloadUserBufAndKeepSelection();
		
		if (command.is_running()) {
			if (ImGui::Button("Cancel")) {
				close_callback();
				command.clear();
				ImGui::CloseCurrentPopup();
			}
		} else {
			if (run_callback && command.succeeded()) {
				if (ImGui::Button("Run")) {
					run_callback();
					command.clear();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
			}
			if (ImGui::Button("Close")) {
				close_callback();
				command.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Save Log")) {
				nfdchar_t* path;
				nfdresult_t result = NFD_SaveDialog("txt", nullptr, &path);
				if (result == NFD_OKAY) {
					FileOutputStream log;
					if (log.open(fs::path(std::string(path)))) {
						std::string output = command.copy_entire_output();
						log.write_n((u8*) output.data(), output.size());
					}
					free(path);
				}
			}
		}
		ImGui::EndPopup();
	}
}
