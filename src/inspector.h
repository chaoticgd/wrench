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

#ifndef INSPECTOR_H
#define INSPECTOR_H

#include "stream.h"
#include "window.h"
#include "project.h"
#include "texture.h"
#include "reflection/refolder.h"
#include "commands/property_changed_command.h"

# /*
#	Property editor GUI.
# */

#ifndef INSPECTABLE_DEF
#define INSPECTABLE_DEF
struct inspectable { virtual ~inspectable() = default; };
#endif

class inspector : public window {
public:
	inspector(inspectable* subject);

	const char* title_text() const override;
	ImVec2 initial_size() const override;
	void render(app& a) override;

private:
	template <typename T_data_type, typename T_input_func>
	static std::function<void(const char* name, rf::property<T_data_type> p)>
		render_property(wrench_project* project, int& i, T_input_func input);
	
	template <typename... T_callbacks>
	void reflect(T_callbacks... callbacks);

	inspectable* _subject;
};

template <typename T_data_type, typename T_input_func>
std::function<void(const char* name, rf::property<T_data_type> p)>
	inspector::render_property(wrench_project* project, int& i, T_input_func input) {
	
	return [=, &i](const char* name, rf::property<T_data_type> p) {
		ImGui::PushID(i++);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(" %s", name);
		ImGui::NextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::PushItemWidth(-1);
		T_data_type value = p.get();
		if(input("##input", &value)) {
			if(auto lvl = project->selected_level()) {
				lvl->emplace_command<property_changed_command<T_data_type>>(p, value);
			}
		}
		ImGui::NextColumn();
		ImGui::PopID();
		ImGui::PopItemWidth();
	};
}

template <typename... T_callbacks>
void inspector::reflect(T_callbacks... callbacks) {
	if(auto subject = dynamic_cast<app*>(_subject)) {
		subject->reflect(callbacks...);
	}

	if(auto subject = dynamic_cast<texture*>(_subject)) {
		subject->reflect(callbacks...);
	}
}

template <typename T>
bool inspector_input_int(const char* label, T* data) {
	int temp = *data;
	if(ImGui::InputInt(label, &temp, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		*data = temp;
		return true;
	}
	return false;
}

#endif
