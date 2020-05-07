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
		static void category(const char* name) {
			ImGui::Columns(1);
			ImGui::Text("%s", name);
			ImGui::Columns(2);
		}
		
		void begin_property(const char* name) {
			ImGui::PushID(_num_properties++);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(" %s", name);
			ImGui::NextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::PushItemWidth(-1);
		}
		
		void end_property() {
			ImGui::NextColumn();
			ImGui::PopID();
			ImGui::PopItemWidth();
		}
		
		// Handle pushing a new undo/redo command onto the history stack
		// whenever the value of a property is changed.
		template <typename T, typename T_value>
		void set_property(std::vector<object_id> ids, T_value T::*member, T_value new_value) {
			
			class property_changed_command : public command {
			public:
				property_changed_command(
						std::string level_name, std::vector<object_id> object_ids, T_value T::*member, T_value new_value)
					: _level_name(level_name), _object_ids(object_ids), _member(member), _new_value(new_value) {}
				
			protected:
				level& get_level(wrench_project* project) {
					level* lvl = project->level_from_name(_level_name);
					if (lvl == nullptr) {
						throw command_error(
							"The level for which this operation should "
							"be applied to is not currently loaded.");
					}
					return *lvl;
				}
			
				auto& member(level& lvl, std::size_t id_index) {
					return lvl.world.object_from_id<T>(_object_ids[id_index]).*_member;
				}

				void apply(wrench_project* project) override {
					level& lvl = get_level(project);
					
					_old_values.resize(_object_ids.size());
					for(std::size_t i = 0; i < _object_ids.size(); i++) {
						auto& ref = member(lvl, i);
						_old_values[i] = ref;
						ref = _new_value;
					}
				}
				
				void undo(wrench_project* project) override {
					level& lvl = get_level(project);
					
					for(std::size_t i = 0; i < _object_ids.size(); i++) {
						member(lvl, i) = _old_values[i];
					}
				}

			private:
				std::string _level_name;
				std::vector<object_id> _object_ids;
				T_value T::*_member;
				T_value _new_value;
				std::vector<T_value> _old_values;
			};
			
			// Filter out IDs for objects that don't current exist.
			std::vector<object_id> validated_ids;
			for(object_id id : ids) {
				if(_lvl->world.object_exists<T>(id)) {
					validated_ids.push_back(id);
				}
			}
			
			_project->template emplace_command<property_changed_command>
				(_project->selected_level_name(), validated_ids, member, new_value);
		}
		
		// If all the objects with the given indices have the same value for
		// the specified member, return said value, otherwise return a default
		// value. This is used to handle multiple selection.
		template <typename T, typename T_value>
		std::pair<T_value, ImGuiTextFlags> unique_or(std::vector<object_id> ids, T_value T::*member, T_value default_value) {
			constexpr ImGuiTextFlags normal_flags = ImGuiInputTextFlags_EnterReturnsTrue;
			constexpr ImGuiTextFlags default_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_Password;
			
			if(ids.size() < 1) {
				return { default_value, default_flags };
			}
			
			std::optional<T_value> previous;
			for(std::size_t i = 0; i < ids.size(); i++) {
				if(_lvl->world.object_exists<T>(ids[i])) {
					T_value current = _lvl->world.object_from_id<T>(ids[i]).*member;
					if(previous && *previous != current) {
						return { default_value, default_flags };
					}
					previous = current;
				}
			}
			
			return { *previous, normal_flags };
		}
		
		// Functions to draw different types of input fields.
		
		template <typename T>
		void input_u32(std::vector<object_id> ids, const char* name, uint32_t T::*member) {
			begin_property(name);
			auto [value, flags] = unique_or<T, uint32_t>(ids, member, 0);
			int copy = static_cast<int>(value);
			if(ImGui::InputInt("##input", &copy, 1, 100, flags)) {
				set_property(ids, member, static_cast<uint32_t>(copy));
			}
			end_property();
		}
		
		template <typename T>
		void input_i32(std::vector<object_id> ids, const char* name, int32_t T::*member) {
			begin_property(name);
			auto [value, flags] = unique_or<T, int32_t>(ids, member, 0);
			int copy = static_cast<int>(value);
			if(ImGui::InputInt("##input", &copy, 1, 100, flags)) {
				set_property(ids, member, static_cast<int32_t>(copy));
			}
			end_property();
		}
		
		template <typename T>
		void input_vec3(std::vector<object_id> ids, const char* name, vec3f T::*member) {
			begin_property(name);
			auto [value, flags] = unique_or<T, vec3f>(ids, member, vec3f());
			if(ImGui::InputFloat3("##input", &value.x, 3, flags)) {
				set_property(ids, member, value);
			}
			end_property();
		}
			
		int _num_properties;
		wrench_project* _project;
		level* _lvl;
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

		void render_grid(app& a, std::vector<texture>& tex_list);
		void cache_texture(texture* tex);

		void import_bmp(app& a, texture* tex);
		void export_bmp(app& a, texture* tex);

		int _project_id = 0;
		std::map<texture*, GLuint> _gl_textures;
		std::string _list;
		std::size_t _selection = 0;
		filter_parameters _filters = { 0 };
	};
	
	class model_browser : public window {
	public:
		model_browser();
		~model_browser();
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
		
		game_model* render_selection_pane(app& a);
		game_model* render_selection_grid(
			app& a,
			std::string list,
			std::vector<game_model>& models);
		
		void render_preview(
			GLuint* target,
			const game_model& model,
			const gl_renderer& renderer,
			ImVec2 preview_size,
			float zoom,
			glm::vec2 pitch_yaw);
		glm::vec2 get_drag_delta() const;
		
		static void render_dma_debug_info(game_model& mdl);
	
	private:
		std::string _list;
		std::size_t _model;
	
		float _zoom = 1.f;
		glm::vec2 _pitch_yaw = { 0.f, 0.f };
		
		int _project_id = 0;
		std::map<game_model*, GLuint> _model_thumbnails;
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
	
	class hex_dump : public window {
	public:
		hex_dump(uint8_t* data, std::size_t size_in_bytes);
	
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	private:
		std::vector<std::string> _lines;
	};
	
	GLuint load_icon(std::string path);
	
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
