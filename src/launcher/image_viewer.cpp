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

#include "image_viewer.h"

void image_viewer(const std::vector<ModImage>& images)
{
	ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);
	
	if (ImGui::BeginPopupModal("Image Viewer", nullptr, ImGuiWindowFlags_NoResize)) {
		ImGuiStyle& s = ImGui::GetStyle();
		ImVec2 button_size = ImGui::CalcTextSize("Close") + s.FramePadding * 3;
		static size_t selected_image = 0;
		
		ImVec2 viewport = ImGui::GetWindowSize() - ImVec2(0, button_size.y);
		
		const ModImage& image = images[selected_image];
		ImVec2 image_pos((viewport.x - image.width) / 2.f, (viewport.y - image.height) / 2.f);
		ImVec2 image_size(image.width, image.height);
		
		ImGui::SetCursorPos(image_pos);
		ImGui::Image((ImTextureID) image.texture.id, image_size);
		
		ImGui::SetCursorPos(ImVec2(0, viewport.y));
		ImGui::NewLine();
		for (size_t i = 0; i < images.size(); i++) {
			const ModImage& image = images[i];
			ImGui::SameLine();
			if (ImGui::RadioButton(image.path.c_str(), i == selected_image)) {
				selected_image = i;
			}
		}
		
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - button_size.x);
		if (ImGui::Button("Close")) {
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}
