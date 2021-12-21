/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include <functional>

#include "../app.h"
#include "../fs_includes.h"
#include "../gl_includes.h"
#include "../formats/level_impl.h"
#include "window.h"
#include "view_3d.h"
#include "inspector.h"
#include "imgui_includes.h"

namespace gui {
	void init();
	
	void render(app& a);
	float render_menu_bar(app& a);
	void render_tools(app& a, float menu_bar_height);

	void create_dock_layout(const app& a);
	void begin_docking();
	
	void render_tree_menu(app& a);
	
	template <typename T, typename... T_constructor_args>
	void render_menu_bar_window_toggle(app& a, T_constructor_args... args);
	
	class start_screen : public window {
	public:
		start_screen();
		
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	
	private:
		bool button(const char* str, ImTextureID user_texture_id, const ImVec2& icon_size) const;
	
		GlTexture dvd, folder, floppy;
	};

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
	
	class settings : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	
	private:
		void render_general_page(app& a);
		void render_gui_page(app& a);
		void render_debug_page(app& a);
		
		std::size_t _new_game_type = 0;
		std::string _new_game_path;
	};
	
	class alert_box {
	public:
		alert_box(const char* title) : _title(title) {}
		
		void render();
		void open(std::string new_text);
	
	private:
		const char* _title;
		bool _is_open = false;
		std::string _text;
	};
	
	class prompt_box {
	public:
		prompt_box(const char* text) : _button_text(text), _title(text) {}
		prompt_box(const char* button_text, const char* title) :
			_button_text(button_text), _title(title) {}
		
		// Returns the entered text for one frame when the "Okay" button is
		// pressed, otherwise returns an empty std::optional.
		std::optional<std::string> prompt(); // With button.
		std::optional<std::string> render(); // Without button.
	
		void open();
	
	private:
		const char* _button_text;
		const char* _title;
		bool _is_open = false;
		std::string _text;
	};
	
	class hex_dump : public window {
	public:
		hex_dump(uint8_t* data, std::size_t size_in_bytes);
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	private:
		std::vector<std::string> _lines;
	};
	
	// Don't pass untrusted input to this!
	void open_in_browser(const char* url);
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
