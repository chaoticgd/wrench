/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "inspector.h"

inspector::inspector(std::any* subject) : _subject(subject) {}

const char* inspector::title_text() const {
	return "Inspector";
}

ImVec2 inspector::initial_size() const {
	return ImVec2(250, 500);
}

void inspector::render(app& a) {
	if(auto lvl = a.get_level()) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 80);

		int i = 0;
		reflect(
			render_property<uint16_t>(lvl, i,
				inspector_input_int<uint16_t>),
			render_property<uint32_t>(lvl, i,
				inspector_input_int<uint32_t>),
			render_property<int>(lvl, i,
				inspector_input_int<int>),
			render_property<std::string>(lvl, i,
				[](const char* label, std::string* data)
					{ return ImGui::InputText(label, data, ImGuiInputTextFlags_EnterReturnsTrue); }),
			render_property<glm::vec3>(lvl, i,
				[](const char* label, glm::vec3* data)
					{ return ImGui::InputFloat3(label, &data->x, 3, ImGuiInputTextFlags_EnterReturnsTrue); })
		);

		ImGui::Columns(1);
		ImGui::PopStyleVar();

		if(i == 0) {
			ImGui::Text("<no properties>");
		}
	} else {
		ImGui::Text("<no level open>");
	}
}
