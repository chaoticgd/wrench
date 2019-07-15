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
#include <GL/glew.h>

#include "imgui_includes.h"
#include "app.h"
#include "level.h"
#include "window.h"
#include "inspector.h"
#include "commands/property_changed_command.h"

namespace gui {
	void render(app& a);
	void render_menu_bar(app& a);

	template <typename T, typename... T_constructor_args>
	void render_menu_bar_window_toggle(app& a, T_constructor_args... args);

	void file_new_project(app& a);

	class moby_list : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
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
	
	class texture_browser : public window {
	public:
		texture_browser();
		~texture_browser();

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
	private:
		struct filter_parameters {
			int min_width;
		};

		void render_grid(app& a, texture_provider* provider);
		void cache_texture(texture* tex);

		void import_bmp(app& a, texture* tex);
		void export_bmp(app& a, texture* tex);

		std::map<texture*, GLuint> _gl_textures;
		texture* _selection;
		std::any _selection_any;
		texture_provider* _provider;
		filter_parameters _filters;
		inspector _texture_inspector;
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
		string_input(const char* title, std::string default_text = "");

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;

		void on_okay(std::function<void(app&, std::string)> callback);

	private:
		const char* _title_text;
		std::string _input;
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

#endif
