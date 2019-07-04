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

#ifndef GUI_H
#define GUI_H

#include <functional>

#include "imgui_includes.h"
#include "app.h"
#include "level.h"
#include "window.h"
#include "commands/property_changed_command.h"

namespace gui {
	void render(app& a);
	void render_menu_bar(app& a);

	template <typename T, typename... T_constructor_args>
	void render_menu_bar_window_toggle(app& a, T_constructor_args... args);

	void file_new_project(app& a);

	class iso_tree : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;

	private:
		static void render_tree_node(app& a, stream* node, int depth);
	};

	class moby_list : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};

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
			render_property(level& lvl, int& i, T_input_func input);
		
		T* _subject;
	};

	class viewport_information : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};

	class string_viewer : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	
	private:
		std::string _selected_language;
	};

	class message_box : public window {
	public:
		message_box(const char* title, std::string message);

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	private:
		const char* _title;
		std::string _message;
	};

	class string_input : public window {
	public:
		string_input(const char* title);

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;

		void on_okay(std::function<void(app&, std::string)> callback);

	private:
		const char* _title_text;
		std::vector<char> _buffer;
		std::function<void(app&, std::string)> _callback;
	};
}

template <typename T, typename... T_constructor_args>
void gui::render_menu_bar_window_toggle(app& a, T_constructor_args... args) {
	auto window = std::find_if(a.windows.begin(), a.windows.end(),
		[](auto& current) { return dynamic_cast<T*>(current.get()) != nullptr; });
	std::string prefix = window == a.windows.end() ? "[ ] " : "[X] ";
	std::string item_text = prefix + T(args...).title_text();
	if(ImGui::MenuItem(item_text.c_str())) {
		if(window == a.windows.end()) {
			a.windows.emplace_back(std::make_unique<T>(args...));
		} else {
			a.windows.erase(window);
		}
	}
}

/*
	inspector
*/

template <typename T>
gui::inspector<T>::inspector(T* subject) : _subject(subject) {}

template <typename T>
const char* gui::inspector<T>::title_text() const {
	return "Inspector";
}

template <typename T>
ImVec2 gui::inspector<T>::initial_size() const {
	return ImVec2(250, 500);
}

template <typename T>
void gui::inspector<T>::render(app& a) {
	if(!a.has_level()) {
		ImGui::Text("<no level open>");
		return;
	}

	/*a.bind_level([this](level& lvl) {
		if(lvl.selection.size() < 1) {
			ImGui::Text("<no selection>");
			return;
		} else if(lvl.selection.size() > 1) {
			ImGui::Text("<multiple mobies selected>");
			return;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
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

		ImGui::PopStyleVar();
	});*/
}

template <typename T>
template <typename T_data_type, typename T_input_func>
std::function<void(const char* name, rf::property<T_data_type> p)>
	gui::inspector<T>::render_property(level& lvl, int& i, T_input_func input) {
	
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

#endif
