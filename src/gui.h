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
#include <boost/filesystem.hpp>

#include "gl_includes.h"
#include "imgui_includes.h"
#include "app.h"
#include "level.h"
#include "window.h"
#include "view_3d.h"
#include "commands/property_changed_command.h"
#include "formats/game_model.h"

# /*
#	Implements most of the GUI.
# */

namespace fs = boost::filesystem;

#define INSPECTOR_PROPERTY(type) \
	typename property<T, type>::getter const get, \
	typename property<T, type>::setter set

namespace gui {
	void render(app& a);
	void render_menu_bar(app& a);

	void create_dock_layout(const app& a);
	void begin_docking();

	template <typename T, typename... T_constructor_args>
	void render_menu_bar_window_toggle(app& a, T_constructor_args... args);

	class project_tree : public window {
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
		void category(const char* name);
	
		template <typename T> void input_integer(const char* name, INSPECTOR_PROPERTY(int        ) = nullptr);
		template <typename T> void input_uint16 (const char* name, INSPECTOR_PROPERTY(uint16_t   ) = nullptr);
		template <typename T> void input_size_t (const char* name, INSPECTOR_PROPERTY(std::size_t) = nullptr);
		template <typename T> void input_string (const char* name, INSPECTOR_PROPERTY(std::string) = nullptr);
		template <typename T> void input_vector3(const char* name, INSPECTOR_PROPERTY(glm::vec3  ) = nullptr);	
		
		void begin_property(const char* name);
		void end_property();
		
		template <typename T, typename T_value>
		void set_property(T_value value, INSPECTOR_PROPERTY(T_value));

		wrench_project* _project;
		game_object* _subject;
		int _num_properties;
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
	
	class tools : public window {
	public:
		tools();
		~tools();
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	
	private:
		GLuint _picker_icon;
		GLuint _selection_icon;
		GLuint _translate_icon;
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

		int _project_id;
		std::map<texture*, GLuint> _gl_textures;
		std::size_t _provider;
		std::size_t _selection;
		filter_parameters _filters;
	};
	
	class model_browser : public window {
	public:
		model_browser();
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
		GLuint render_preview(
			const game_model& model,
			const gl_renderer& renderer,
			ImVec2 preview_size);
		glm::vec2 get_drag_delta() const;
	
	private:
		float _zoom = 1.f;
		glm::vec2 _pitch_yaw = { 0.f, 0.f };
	};

	class settings : public window {
	public:
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};
	
	class manual_patcher : public window {
	public:
		manual_patcher();
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	private:
		std::string _scroll_offset_str;
		std::size_t _scroll_offset;
	};

	class message_box : public window {
	public:
		message_box(const char* title, std::string message);

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		bool is_unique() const override;
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
		bool is_unique() const override;

		void on_okay(std::function<void(app&, std::string)> callback);

	private:
		const char* _title_text;
		std::string _input;
		std::function<void(app&, std::string)> _callback;
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
	
	GLuint load_icon(std::string path);
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

template <typename T>
void gui::inspector::input_integer(const char* name, INSPECTOR_PROPERTY(int)) {
	begin_property(name);
	auto value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputInt("##input", &value, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		set_property<T>(value, get, set);
	}
	end_property();
}

template <typename T>
void gui::inspector::input_uint16(const char* name, INSPECTOR_PROPERTY(uint16_t)) {
	begin_property(name);
	auto value = (static_cast<T*>(_subject)->*get)();
	int value_int = static_cast<int>(value);
	if(ImGui::InputInt("##input", &value_int, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		set_property<T>(static_cast<decltype(value)>(value_int), get, set);
	}
	end_property();
}

template <typename T>
void gui::inspector::input_size_t(const char* name, INSPECTOR_PROPERTY(std::size_t)) {
	begin_property(name);
	auto value = (static_cast<T*>(_subject)->*get)();
	int value_int = static_cast<int>(value);
	if(ImGui::InputInt("##input", &value_int, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		set_property<T>(static_cast<decltype(value)>(value_int), get, set);
	}
	end_property();
}

template <typename T>
void gui::inspector::input_string(const char* name, INSPECTOR_PROPERTY(std::string)) {
	begin_property(name);
	auto value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputText("##input", &value, ImGuiInputTextFlags_EnterReturnsTrue)) {
		set_property<T>(value, get, set);
	}
	end_property();
}

template <typename T>
void gui::inspector::input_vector3(const char* name, INSPECTOR_PROPERTY(glm::vec3)) {
	begin_property(name);
	auto value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputFloat3("##input", &value.x, 3, ImGuiInputTextFlags_EnterReturnsTrue)) {
		set_property<T>(value, get, set);
	}
	end_property();
}

template <typename T, typename T_value>
void gui::inspector::set_property(T_value value, INSPECTOR_PROPERTY(T_value)) {
	if(set == nullptr) {
		return;
	}
	
	T* subject = static_cast<T*>(_subject);
	property<T, T_value> p { get, set };
	
	using cmd = property_changed_command<T, T_value>;
	_project->template emplace_command<cmd>(*subject, p, value);
}

#endif
