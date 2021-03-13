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

#ifndef GUI_H
#define GUI_H

#include <functional>
#include "imgui_includes.h"
#include <imgui_markdown/imgui_markdown.h>

#include "app.h"
#include "window.h"
#include "view_3d.h"
#include "fs_includes.h"
#include "gl_includes.h"
#include "formats/game_model.h"
#include "formats/level_impl.h"

# /*
#	Implements most of the GUI.
# */

namespace gui {
	void render(app& a);
	float render_menu_bar(app& a);
	void render_tools(app& a, float menu_bar_height);

	void create_dock_layout(const app& a);
	void begin_docking();

	template <typename T, typename... T_constructor_args>
	void render_menu_bar_window_toggle(app& a, T_constructor_args... args);

	class inspector : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
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

	class string_viewer : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};
	
	class texture_browser : public window {
	public:
		texture_browser();

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
	private:
		struct filter_parameters {
			int min_width;
		};

		void render_grid(app& a, std::vector<texture>& tex_list);
		
		std::string _list;
		std::size_t _selection = 0;
		filter_parameters _filters = { 0 };
	};
	
	class model_browser : public window {
	public:
		model_browser();
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
		moby_model* render_selection_pane(app& a);
		moby_model* render_selection_grid(
			app& a,
			std::string list_name,
			model_list& list);
		
		struct view_params {
			view_mode mode = view_mode::TEXTURED_POLYGONS;
			float zoom = 0.5f;
			glm::vec2 pitch_yaw = { 0.f, 0.f };
			bool show_vertex_indices = false;
		};
		
		static void render_preview(
			app& a,
			GLuint* target,
			moby_model& model,
			std::vector<texture>& textures,
			const gl_renderer& renderer,
			ImVec2 preview_size,
			view_params params);
		glm::vec2 get_drag_delta() const;
		
		static void render_submodel_list(moby_model& model);
		static void render_dma_debug_info(moby_model& mdl);
	
	private:
		std::map<std::string, model_list> _model_lists;
		std::string _list;
		std::size_t _model;
		bool _fullscreen_preview = false;
		view_params _view_params;
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
	
	class document_viewer : public window {
	public:
		document_viewer(const char* path);
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
		void load_page(std::string path);
		
	private:
		std::string _body;
		ImGui::MarkdownConfig _config;
	};

	class stream_viewer : public window {
	public:
		stream_viewer();
		
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
		void render_stream_tree_node(stream* node, std::size_t index);
		
		// Write out a BMP image to the Wrench directory representing the passed
		// trace stream where red areas have been read in by Wrench and
		// grayscale areas have not (the Y axis is bottom to top).
		void export_trace(trace_stream* node);
	
	private:
		// Validate this before dereferencing it!
		stream* _selection = nullptr;
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

	class file_dialog : public window {
	public:
		enum mode { open, save };

		file_dialog(const char* title, mode m, std::vector<std::string> extensions);

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		bool is_unique() const override;

		void on_okay(std::function<void(std::string)> callback);

	private:
		const char* _title;
		mode _mode;
		std::vector<std::string> _extensions;
		std::string _directory_input;
		fs::path _directory;
		std::string _file;
		std::function<void(std::string)> _callback;
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
