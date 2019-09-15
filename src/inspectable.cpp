#include "inspectable.h"

inspector_callbacks::inspector_callbacks(inspectable* subject)
	: _subject(subject), i(0) {}

void inspector_callbacks::category(const char* name) {
	ImGui::Columns(1);
	ImGui::Text("%s", name);
	ImGui::Columns(2);
}

void inspector_callbacks::begin_property(const char* name) {
	ImGui::PushID(i++);
	ImGui::AlignTextToFramePadding();
	ImGui::Text(" %s", name);
	ImGui::NextColumn();
	ImGui::AlignTextToFramePadding();
	ImGui::PushItemWidth(-1);
}

void inspector_callbacks::end_property() {
	ImGui::NextColumn();
	ImGui::PopID();
	ImGui::PopItemWidth();
}
