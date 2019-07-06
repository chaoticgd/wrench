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
#include "texture.h"
#include "reflection/refolder.h"

template <typename T>
class inspector : public window {
public:
	inspector(T* subject);

	const char* title_text() const override;
	ImVec2 initial_size() const override;
	void render(app& a) override;

private:
	template <typename T_data_type, typename T_input_func>
	static std::function<void(const char* name, rf::property<T_data_type> p)>
		render_property(level* lvl, int& i, T_input_func input);
	
	T* _subject;
};

// Passed as the template parameter to the inspector.
class inspector_reflector {
public:
	inspector_reflector(stream** selection)
		: _selection(selection) {}

	template <typename... T_callbacks>
	void reflect(T_callbacks... callbacks);

private:
	stream** _selection;
};

template <typename T>
inspector<T>::inspector(T* subject) : _subject(subject) {}

template <typename T>
const char* inspector<T>::title_text() const {
	return "Inspector";
}

template <typename T>
ImVec2 inspector<T>::initial_size() const {
	return ImVec2(250, 500);
}

template <typename T>
void inspector<T>::render(app& a) {
	if(auto lvl = a.get_level()) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 80);

		int i = 0;
		_subject->reflect(
			render_property<uint16_t>(lvl, i,
				[](const char* label, uint16_t* data) {
					int temp = *data;
					if(ImGui::InputInt(label, &temp, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
						*data = temp;
						return true;
					}
					return false;
				}),
			render_property<uint32_t>(lvl, i,
				[](const char* label, uint32_t* data) {
					int temp = *data;
					if(ImGui::InputInt(label, &temp, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
						*data = temp;
						return true;
					}
					return false;
				}),
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

template <typename T>
template <typename T_data_type, typename T_input_func>
std::function<void(const char* name, rf::property<T_data_type> p)>
	inspector<T>::render_property(level* lvl, int& i, T_input_func input) {
	
	return [=, &lvl, &i](const char* name, rf::property<T_data_type> p) {
		ImGui::PushID(i++);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", name);
		ImGui::NextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::PushItemWidth(-1);
		T_data_type value = p.get();
		if(input("##nolabel", &value)) {
			//lvl.emplace_command<property_changed_command<T_data_type>>(p, value);
		}
		ImGui::NextColumn();
		ImGui::PopID();
		ImGui::PopItemWidth();
	};
}

template <typename... T_callbacks>
void inspector_reflector::reflect(T_callbacks... callbacks) {
	stream* selection = *_selection;

	if(selection == nullptr) {
		return;
	}

	if(auto subject = dynamic_cast<moby*>(selection)) {
		subject->reflect(callbacks...);
	}
}

#endif
