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

void gui::command_output_screen(const char* id, CommandStatus& status, void (*close_callback)(), void (*run_callback)()) {
	ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(640, 480), ImGui::GetMainViewport()->Size);
	
	ImGuiStyle& s = ImGui::GetStyle();
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 0;
	size.y -= s.WindowPadding.y * 2 + 64;
	
	bool finished;
	{
		std::lock_guard<std::mutex> g(status.mutex);
		
		if(!status.finished) {
			status.buffer.resize(0);
			status.buffer.reserve(16 * 1024);
			
			// Find the start of the last 15 lines.
			s64 i;
			s32 new_line_count = 0;
			for(i = (s64) status.output.size() - 1; i > 0; i--) {
				if(status.output[i] == '\n') {
					new_line_count++;
					if(new_line_count >= 15) {
						i++;
						break;
					}
				}
			}
			
			// Go through the last 15 lines of the output and strip out colour
			// codes, taking care to handle incomplete buffers.
			for(; i < (s64) status.output.size(); i++) {
				// \033[%sm%02x\033[0m
				if(i + 1 < status.output.size() && memcmp(status.output.data() + i, "\033[", 2) == 0) {
					if(i + 3 < status.output.size() && status.output[i + 3] == 'm') {
						i += 3;
					} else {
						i += 4;
					}
				} else {
					status.buffer += status.output[i];
				}
			}
		}
		finished = status.finished;
	}
	
	if(ImGui::BeginPopupModal(id)) {
		ImGui::SetNextItemWidth(-1);
		ImGui::InputTextMultiline("output", &status.buffer, size, ImGuiInputTextFlags_Multiline | ImGuiInputTextFlags_ReadOnly);
		if(finished) {
			if(run_callback) {
				if(ImGui::Button("Run")) {
					run_callback();
					status.buffer.clear();
					status.output.clear();
					finished = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
			}
			if(ImGui::Button("Close")) {
				close_callback();
				status.buffer.clear();
				status.output.clear();
				finished = false;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndPopup();
	}
}
