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
#include "window.h"
#include "view_3d.h"
#include "formats/game_model.h"
#include "formats/level_impl.h"

# /*
#	Implements most of the GUI.
# */

namespace fs = boost::filesystem;

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
		void set_property(std::size_t index, T_value T::*member, T_value new_value) {
			
			class property_changed_command : public command {
			public:
				property_changed_command(
						std::size_t level_offset, std::size_t object_index, T_value T::*member, T_value new_value)
					: _level_offset(level_offset), _object_index(object_index), _member(member), _new_value(new_value) {}
				
			protected:
				void apply(wrench_project* project) override {
					auto& ref = member(project);
					_old_value = ref;
					ref = _new_value;
				}
				
				void undo(wrench_project* project) override {
					member(project) = _old_value;
				}
				
			private:
				auto& member(wrench_project* project) {
					level* lvl = project->level_at(_level_offset);
					if(lvl == nullptr) {
						throw command_error(
							"The level for which this operation should "
							"be applied to is not currently loaded.");
					}
					return lvl->world.object_at<T>(_object_index).*_member;
				}
			
				std::size_t _level_offset;
				std::size_t _object_index;
				T_value T::*_member;
				T_value _new_value;
				T_value _old_value;
			};
			
			_project->template emplace_command<property_changed_command>
				(_project->selected_level()->offset, index, member, new_value);
		}
		
		// Functions to draw different types of input fields.
		
		template <typename T>
		void input_u32(std::size_t index, const char* name, uint32_t T::*member) {
			begin_property(name);
			int copy = _lvl->world.object_at<T>(index).*member;
			if(ImGui::InputInt("##input", &copy, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
				set_property(index, member, static_cast<uint32_t>(copy));
			}
			end_property();
		}
		
		template <typename T>
		void input_i32(std::size_t index, const char* name, int32_t T::*member) {
			begin_property(name);
			int copy = _lvl->world.object_at<T>(index).*member;
			if(ImGui::InputInt("##input", &copy, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
				set_property(index, member, static_cast<int32_t>(copy));
			}
			end_property();
		}
		
		template <typename T>
		void input_vec3(std::size_t index, const char* name, vec3f T::*member) {
			begin_property(name);
			vec3f copy = _lvl->world.object_at<T>(index).*member;
			if(ImGui::InputFloat3("##input", &copy.x, 3, ImGuiInputTextFlags_EnterReturnsTrue)) {
				set_property(index, member, copy);
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
		
		static void render_dma_debug_info(game_model& mdl);
		static void render_hex_dump(std::vector<uint32_t> data, std::size_t starting_offset);
	
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

#endif
