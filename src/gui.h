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
#include "window.h"
#include "commands/property_changed_command.h"

namespace gui {
	void render(app& a);
	void render_menu_bar(app& a);

	template <typename T>
	void render_menu_bar_window_toggle(app& a);

	void file_import_rc2_level(app& a);

	class moby_list : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};

	class inspector : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;

	private:
		template <typename T_data_type, typename T_input_func>
		static std::function<void(const char* name, rf::property<T_data_type> p)>
			render_property(level& lvl, int& i, T_input_func input);
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

template <typename T>
void gui::render_menu_bar_window_toggle(app& a) {
	auto window = std::find_if(a.windows.begin(), a.windows.end(),
		[](auto& current) { return dynamic_cast<T*>(current.get()) != nullptr; });
	std::string prefix = window == a.windows.end() ? "[ ] " : "[X] ";
	std::string item_text = prefix + T().title_text();
	if(ImGui::MenuItem(item_text.c_str())) {
		if(window == a.windows.end()) {
			a.windows.emplace_back(std::make_unique<T>());
		} else {
			a.windows.erase(window);
		}
	}
}

template <typename T_data_type, typename T_input_func>
std::function<void(const char* name, rf::property<T_data_type> p)>
	gui::inspector::render_property(level& lvl, int& i, T_input_func input) {
	
	return [=, &lvl, &i](const char* name, rf::property<T_data_type> p) {
		ImGui::PushID(i++);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", name);
		ImGui::NextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::PushItemWidth(-1);
		T_data_type value = p.get();
		if(input("##nolabel", &value)) {
			lvl.emplace_command<property_changed_command<T_data_type>>(p, value);
		}
		ImGui::NextColumn();
		ImGui::PopID();
		ImGui::PopItemWidth();
	};
}

#endif
