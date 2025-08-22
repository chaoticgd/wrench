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

#include "book.h"

static void navigation(
	const gui::Page** current_page, const gui::Chapter* chapters, s32 chapter_count, f32 buttons_height);

gui::BookResult gui::book(
	const Page** current_page,
	const char* id,
	const Chapter* chapters,
	s32 chapter_count,
	BookButtons buttons)
{
	ImGuiStyle& s = ImGui::GetStyle();
	ImVec2 buttons_size(0, 0);
	switch (buttons) {
		case BookButtons::CLOSE: {
			ImVec2 close_size = ImGui::CalcTextSize("Close");
			buttons_size.x += close_size.x + s.FramePadding.x * 8;
			buttons_size.y = close_size.y + s.FramePadding.y * 4 + 4;
			break;
		}
		case BookButtons::OKAY_CANCEL_APPLY: {
			ImVec2 okay_size = ImGui::CalcTextSize("Okay");
			ImVec2 cancel_size = ImGui::CalcTextSize("Cancel");
			ImVec2 apply_size = ImGui::CalcTextSize("Apply");
			buttons_size.x += okay_size.x + s.FramePadding.x * 8 + s.ItemSpacing.x;
			buttons_size.x += cancel_size.x + s.FramePadding.x * 8 + s.ItemSpacing.x;
			buttons_size.x += apply_size.x + s.FramePadding.x * 8;
			buttons_size.y = okay_size.y + s.FramePadding.y * 4 + 4;
		}
	}
	
	BookResult result = BookResult::NONE;
	
	ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(640, 480), ImGuiCond_Appearing);
	ImGui::SetNextWindowSizeConstraints(ImVec2(640, 480), ImGui::GetMainViewport()->Size);
	
	if (ImGui::BeginPopupModal(id)) {
		if (ImGui::BeginTable("layout", 2, 0)) {
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, 200.f);
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthStretch);
			
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			navigation(current_page, chapters, chapter_count, buttons_size.y);
			
			ImGui::TableNextColumn();
			(*current_page)->function();
			
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding * ImVec2(4.f, 2.f));
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x - buttons_size.x - 16);
			switch (buttons) {
				case BookButtons::CLOSE: {
					if (ImGui::Button("Close")) {
						ImGui::CloseCurrentPopup();
						result = BookResult::CLOSE;
					}
					break;
				}
				case BookButtons::OKAY_CANCEL_APPLY: {
					if (ImGui::Button("Okay")) {
						ImGui::CloseCurrentPopup();
						result = BookResult::OKAY;
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel")) {
						ImGui::CloseCurrentPopup();
						result = BookResult::CANCEL;
					}
					ImGui::SameLine();
					if (ImGui::Button("Apply")) {
						result = BookResult::APPLY;
					}
					break;
				}
			}
			ImGui::PopStyleVar();
			
			ImGui::EndTable();
		}
		ImGui::EndPopup();
	}
	
	return result;
}

static void navigation(
	const gui::Page** current_page, const gui::Chapter* chapters, s32 chapter_count, f32 buttons_height)
{
	if (*current_page == nullptr) {
		verify_fatal(chapter_count > 0 && chapters[0].count > 0);
		*current_page = &chapters[0].pages[0];
	}
	
	ImVec2 size = ImGui::GetContentRegionAvail();
	size.y -= ImGui::GetStyle().FramePadding.y + buttons_height;
	
	ImGui::PushItemWidth(-1);
	if (ImGui::BeginListBox("##navigation", size)) {
		for (s32 i = 0; i < chapter_count; i++) {
			const gui::Chapter& chapter = chapters[i];
			if (ImGui::TreeNodeEx(chapter.name, ImGuiTreeNodeFlags_DefaultOpen)) {
				for (s32 j = 0; j < chapter.count; j++) {
					const gui::Page* page = &chapter.pages[j];
					bool selected = page == *current_page;
					if (ImGui::Selectable(page->name, &selected)) {
						*current_page = page;
					}
				}
				ImGui::TreePop();
			}
		}
		ImGui::EndListBox();
	}
	ImGui::PopItemWidth();
}
