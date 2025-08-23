/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2024 chaoticgd

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

#include <algorithm>
#include <functional>
#include <span>

#include <nfd.h>

#include <core/filesystem.h>
#include <cppparser/cpp_parser.h>
#include <engine/compression.h>
#include <gui/gui.h>
#include <toolwads/wads.h>
#include <saveeditor/memory_card.h>

#include <imgui_club/imgui_memory_editor.h>

static void update_gui(f32 delta_time);

static void files();
static void controls();
static void editor();
static bool should_display_page(const std::string& tag, memcard::PageLayout layout);

static void begin_dock_space();
static void create_dock_layout();

static void do_load();
static void do_save();
static void update_save_changes_dialog();

static void draw_blocks_page();
static void blocks_sub_page(std::vector<memcard::Block>& blocks, memcard::FileSchema* file_schema);

static void draw_tree(const char* page, std::vector<memcard::Block>& blocks, memcard::FileSchema& file_schema);
static void draw_tree_node(
	const CppType& type, const std::string& name, std::span<u8> data, s32 index, s32 offset, s32 depth, s32 indent);
	
static void draw_table(const char* page, const char* names);
static void draw_level_table(const memcard::Page& page);
static void draw_table_editor(const CppType& type, std::span<u8> data, s32 offset);

static void draw_built_in_editor(const CppType& type, std::span<u8> data, s32 offset);
static void draw_enum_editor(const CppType& type, const CppType& underlying_type, std::span<u8> data, s32 offset);
static void draw_bitfield_editor(const CppType& type, std::span<u8> data, s32 offset);

using BlockCallback = std::function<void(memcard::Block&, memcard::BlockSchema&, CppType&)>;
static void for_each_block_from_page(
	const char* page, std::vector<memcard::Block>& blocks, memcard::FileSchema& file_schema, BlockCallback callback);

static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type);
static const CppType& lookup_save_enum(const std::string& name);
static std::string lookup_level_name(s32 level_number);
static u8 from_bcd(u8 value);
static u8 to_bcd(u8 value);

static FileInputStream s_memcardwad;
static memcard::Schema s_schema;

static std::string s_directory;
static std::vector<fs::path> s_file_paths;
static bool s_should_reload_file_list = true;
static fs::path s_selected_file_path;
static bool s_should_load_now = false;
static Opt<memcard::File> s_file;
static std::vector<u8> s_last_loaded_buffer;
static std::string s_error_message;

static Game s_game = Game::UNKNOWN;
static memcard::GameSchema* s_game_schema = nullptr;
static std::map<std::string, CppType> s_game_types;

static std::map<ImGuiID, bool> s_node_expanded;

enum class SaveChangesDialogState
{
	CLOSED,
	OPENING,
	OPENED,
	CLOSING_YES,
	CLOSING_NO
};

static SaveChangesDialogState s_save_changes_dialog_state = SaveChangesDialogState::CLOSED;
static std::function<void()> s_save_changes_dialog_callback;

int main(int argc, char** argv)
{
	WadPaths wads = find_wads(argv[0]);
	verify(g_guiwad.open(wads.gui), "Failed to open gui wad.");
	verify(s_memcardwad.open(wads.memcard), "Failed to open memcard wad.");
	
	s_memcardwad.seek(wadinfo.memcard.savegame.offset.bytes());
	std::vector<u8> schema_compressed = s_memcardwad.read_multiple<u8>(wadinfo.memcard.savegame.size.bytes());
	
	std::vector<u8> schema_wtf;
	decompress_wad(schema_wtf, schema_compressed);

	s_schema = memcard::parse_schema((char*) schema_wtf.data());
	
	if (argc > 1) {
		s_directory = argv[1];
	}
	
	u64 frame = 0;
	
	GLFWwindow* window = gui::startup("Wrench Save Editor", 1280, 720);
	gui::load_font(wadinfo.gui.fonts[0], 22);
	
	glfwSetWindowCloseCallback(window, [](GLFWwindow* window) {
		glfwSetWindowShouldClose(window, GLFW_FALSE);
		
		s_save_changes_dialog_state = SaveChangesDialogState::OPENING;
		s_save_changes_dialog_callback = [window]() {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		};
	});
	
	while (!glfwWindowShouldClose(window)) {
		gui::run_frame(window, update_gui);
		
		if (s_should_load_now) {
			do_load();
			s_should_load_now = false;
		}
		
		if ((frame % 60) == 0) {
			s_should_reload_file_list = true;
		}
		
		frame++;
	}
	
	gui::shutdown(window);
}

static void update_gui(f32 delta_time)
{
	begin_dock_space();
	
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Files", nullptr, ImGuiWindowFlags_NoTitleBar);
	files();
	ImGui::End();
	
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoTitleBar);
	controls();
	ImGui::End();
	
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_NoTitleBar);
	editor();
	ImGui::End();
	
	static bool is_first_frame = true;
	if (is_first_frame) {
		create_dock_layout();
		is_first_frame = false;
	}
	
	ImGui::End(); // dock space
	
	update_save_changes_dialog();
}

static void files()
{
	static std::string listing_error;
	
	if (gui::input_folder_path(&s_directory, "##directory", nullptr) || s_should_reload_file_list) {
		s_file_paths.clear();
		try {
			for (auto entry : fs::directory_iterator(s_directory)) {
				s_file_paths.emplace_back(entry.path());
			}
			listing_error.clear();
		} catch (std::filesystem::filesystem_error& error) {
			listing_error = error.code().message();
		}
		std::sort(BEGIN_END(s_file_paths));
		s_should_reload_file_list = false;
	}
	
	if (listing_error.empty()) {
		ImGui::BeginChild("##files");
		if (ImGui::Selectable("[DIR] .")) {
			s_should_reload_file_list = true;
		}
		if (ImGui::Selectable("[DIR] ..")) {
			s_directory = fs::weakly_canonical(fs::path(s_directory)).parent_path().string();
			s_should_reload_file_list = true;
		}
		for (auto& path : s_file_paths) {
			if (fs::is_directory(path)) {
				std::string label = stringf("[DIR] %s", path.filename().string().c_str());
				if (ImGui::Selectable(label.c_str())) {
					s_directory = path.string();
					s_should_reload_file_list = true;
				}
			}
		}
		for (auto& path : s_file_paths) {
			if (fs::is_regular_file(path)) {
				if (ImGui::Selectable(path.filename().string().c_str(), path == s_selected_file_path)) {
					s_save_changes_dialog_state = SaveChangesDialogState::OPENING;
					s_save_changes_dialog_callback = [path]() {
						s_should_load_now = true;
						s_selected_file_path = path;
					};
				}
			}
		}
		ImGui::EndChild();
	} else {
		ImGui::Text("%s", listing_error.c_str());
	}
}

static void controls()
{
	if (ImGui::Button("Save")) {
		do_save();
	}
	
	if (ImGui::Button("Save As")) {
		nfdchar_t* path = nullptr;
		nfdresult_t result = NFD_SaveDialog(nullptr, s_directory.c_str(), &path);
		if (result != NFD_OKAY) {
			if (result != NFD_CANCEL) {
				printf("error: %s\n", NFD_GetError());
			}
			return;
		}
		
		s_file->path = path;
		do_save();
		
		s_selected_file_path = s_file->path;
		s_directory = s_selected_file_path.parent_path().string();
		s_should_reload_file_list = true;
		
		free(path);
	}
}

static void editor()
{
	if (!s_error_message.empty()) {
		ImGui::Text("%s", s_error_message.c_str());
		return;
	}
	
	if (!s_file.has_value()) {
		ImGui::Text("No file loaded.");
		return;
	}
	
	if (s_file->checksum_does_not_match) {
		ImGui::Text("Save game checksum doesn't match!");
		ImGui::SameLine();
		if (ImGui::Button("Dismiss")) {
			s_file->checksum_does_not_match = false;
		}
	}
	
	if (ImGui::BeginTabBar("##tabs")) {
		for (memcard::Page& page : s_schema.pages) {
			bool should_display = should_display_page(page.tag, page.layout);
			
			if (should_display && ImGui::BeginTabItem(page.name.c_str())) {
				ImGui::BeginChild("##tab");
				
				switch (page.layout) {
					case memcard::PageLayout::TREE: {
						draw_tree(page.tag.c_str(), s_file->blocks, s_game_schema->game);
						break;
					}
					case memcard::PageLayout::TABLE: {
						draw_table(page.tag.c_str(), page.element_names.c_str());
						break;
					}
					case memcard::PageLayout::LEVEL_TABLE: {
						draw_level_table(page);
						break;
					}
					case memcard::PageLayout::DATA_BLOCKS: {
						draw_blocks_page();
						break;
					}
				}
				
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}
}

static bool should_display_page(const std::string& tag, memcard::PageLayout layout)
{
	if (layout == memcard::PageLayout::DATA_BLOCKS) {
		return true;
	}
	
	if (s_game_schema) {
		for (memcard::Block& block : s_file->blocks) {
			memcard::BlockSchema* block_schema = s_game_schema->game.block(block.iff);
			
			if (!block_schema) {
				block_schema = s_game_schema->net.block(block.iff);
			}
			
			if (block_schema && block_schema->page == tag) {
				return true;
			}
		}
		
		if (!s_file->levels.empty()) {
			for (memcard::Block& block : s_file->levels[0]) {
				memcard::BlockSchema* block_schema = s_game_schema->level.block(block.iff);
				
				if (block_schema && block_schema->page == tag) {
					return true;
				}
			}
		}
	}
	
	return false;
}

static void begin_dock_space()
{
	ImRect viewport = ImRect(ImVec2(0, 0), ImGui::GetMainViewport()->Size);
	ImGuiWindowFlags window_flags =  ImGuiWindowFlags_NoDocking;
	ImGui::SetNextWindowPos(viewport.Min);
	ImGui::SetNextWindowSize(viewport.Max - viewport.Min);
	ImGui::SetNextWindowViewport(ImGui::GetWindowViewport()->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	static bool p_open;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("dock_space", &p_open, window_flags);
	ImGui::PopStyleVar();
	
	ImGui::PopStyleVar(2);

	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
}

static void create_dock_layout()
{
	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1.f, 1.f));

	ImGuiID left, editor;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 8.f / 10.f, &left, &editor);
	
	ImGuiID files, controls;
	ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 8.f / 10.f, &files, &controls);
	
	ImGui::DockBuilderDockWindow("Files", files);
	ImGui::DockBuilderDockWindow("Controls", controls);
	ImGui::DockBuilderDockWindow("Editor", editor);

	ImGui::DockBuilderFinish(dockspace_id);
}

static void do_load()
{
	if (!s_selected_file_path.empty()) {
		try {
			s_last_loaded_buffer = read_file(s_selected_file_path);
			s_file = memcard::read(s_last_loaded_buffer, s_selected_file_path);
			s_error_message.clear();
			
			s32 type_index = -1;
			switch (s_file->blocks.size()) {
				case 47: {
					s_game = Game::RAC;
					type_index = 0;
					break;
				}
				case 34: {
					s_game = Game::GC;
					type_index = 1;
					break;
				}
				case 40: {
					s_game = Game::UYA;
					type_index = 2;
					break;
				}
				case 29: {
					s_game = Game::DL;
					type_index = 3;
					break;
				}
				default: {
					s_game = Game::RAC;
				}
			}
			
			s_game_types.clear();
			s_game_schema = s_schema.game(s_game);
			
			// Load type information used by the editor.
			if (type_index > -1) {
				s_memcardwad.seek(wadinfo.memcard.types[type_index].offset.bytes());
				std::vector<u8> types_compressed =
					s_memcardwad.read_multiple<u8>(wadinfo.memcard.types[type_index].size.bytes());
				
				std::vector<u8> types_cpp;
				decompress_wad(types_cpp, types_compressed);
				types_cpp.emplace_back(0);
				
				std::vector<CppToken> tokens = eat_cpp_file((char*) types_cpp.data());
				parse_cpp_types(s_game_types, std::move(tokens));
				
				for (auto& [name, type] : s_game_types) {
					layout_cpp_type(type, s_game_types, CPP_PS2_ABI);
					
					// Make enums get displayed a little nicer.
					if (type.descriptor == CPP_ENUM) {
						for (auto& [value, constant_name] : type.enumeration.constants) {
							if (constant_name.starts_with("_")) {
								constant_name = constant_name.substr(1);
							}
							
							for (char& c : constant_name) {
								if (c == '_') {
									c = ' ';
								}
							}
						}
					}
				}
			}
		} catch (RuntimeError& error) {
			s_error_message = (error.context.empty() ? "" : (error.context + ": ")) + error.message;
		}
	}
}

static void do_save()
{
	if (s_file.has_value()) {
		try {
			std::vector<u8> buffer;
			memcard::write(buffer, *s_file);
			write_file(s_file->path, buffer);
		} catch (RuntimeError& error) {
			s_error_message = (error.context.empty() ? "" : (error.context + ": ")) + error.message;
		}
		s_should_reload_file_list = true;
	}
}

static void update_save_changes_dialog()
{
	switch (s_save_changes_dialog_state) {
		case SaveChangesDialogState::CLOSED: {
			break;
		}
		case SaveChangesDialogState::OPENING: {
			if (s_file.has_value()) {
				std::vector<u8> buffer;
				memcard::write(buffer, *s_file);
				if (buffer != s_last_loaded_buffer) {
					// The save file has been modified, so we need to show the
					// save changes dialog.
					ImGui::OpenPopup("Confirmation##save_changes_dialog");
					s_save_changes_dialog_state = SaveChangesDialogState::OPENED;
					break;
				}
			}
			// There are no changes, or a file hasn't been loaded, so skip
			// showing the dialog and run the callback immediately.
			s_save_changes_dialog_state = SaveChangesDialogState::CLOSING_NO;
			break;
		}
		case SaveChangesDialogState::OPENED: {
			if (ImGui::BeginPopupModal("Confirmation##save_changes_dialog")) {
				ImGui::Text("Save changes to '%s'?", s_selected_file_path.string().c_str());
				
				if (ImGui::Button("Yes")) {
					s_save_changes_dialog_state = SaveChangesDialogState::CLOSING_YES;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				
				if (ImGui::Button("No")) {
					s_save_changes_dialog_state = SaveChangesDialogState::CLOSING_NO;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				
				if (ImGui::Button("Cancel")) {
					s_save_changes_dialog_state = SaveChangesDialogState::CLOSED;
					ImGui::CloseCurrentPopup();
				}
				
				ImGui::EndPopup();
			}
			
			break;
		}
		case SaveChangesDialogState::CLOSING_YES: {
			do_save();
			s_save_changes_dialog_callback();
			s_save_changes_dialog_state = SaveChangesDialogState::CLOSED;
			break;
		}
		case SaveChangesDialogState::CLOSING_NO: {
			s_save_changes_dialog_callback();
			s_save_changes_dialog_state = SaveChangesDialogState::CLOSED;
			break;
		}
	}
}

static void draw_blocks_page()
{
	memcard::GameSchema* gs = s_game_schema;
	
	switch (s_file->type) {
		case memcard::FileType::NET: {
			blocks_sub_page(s_file->blocks, gs ? &gs->net : nullptr);
			break;
		}
		case memcard::FileType::SLOT: {
			static size_t selected_level = SIZE_MAX;
			
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Blocks:");
			ImGui::SameLine();
			
			std::string label;
			if (selected_level >= 0 && selected_level < s_file->levels.size()) {
				label = "Level " + std::to_string(selected_level) + " " + lookup_level_name(static_cast<s32>(selected_level));
			} else {
				label = "Global";
			}
			
			ImGui::SetNextItemWidth(-1);
			if (ImGui::BeginCombo("##enum", label.c_str())) {
				if (ImGui::Selectable("Global")) {
					selected_level = SIZE_MAX;
				}
				
				for (size_t i = 0; i < s_file->levels.size(); i++) {
					std::string level_label = "Level " + std::to_string(i) + " " + lookup_level_name(static_cast<s32>(i));
					if (ImGui::Selectable(level_label.c_str())) {
						selected_level = i;
					}
				}
				
				ImGui::EndCombo();
			}
			
			ImGui::BeginChild("##blocks");
			if (selected_level >= 0 && selected_level < s_file->levels.size()) {
				blocks_sub_page(s_file->levels[selected_level], gs ? &gs->level : nullptr);
			} else {
				blocks_sub_page(s_file->blocks, gs ? &gs->game : nullptr);
			}
			ImGui::EndChild();
			
			break;
		}
		default: {
			break;
		}
	}
}

static void blocks_sub_page(std::vector<memcard::Block>& blocks, memcard::FileSchema* file_schema)
{
	static std::vector<MemoryEditor> editors;
	editors.resize(blocks.size());
	for (size_t i = 0; i < blocks.size(); i++) {
		ImGui::PushID(i);
		memcard::Block& block = blocks[i];
		std::string name;
		if (file_schema) {
			memcard::BlockSchema* block_schema = file_schema->block(block.iff);
			if (block_schema) {
				name = stringf("%4d: %s (%d bytes)", block.iff, block_schema->name.c_str(), block.unpadded_size);
			}
		}
		if (name.empty()) {
			name = stringf("%4d: unknown (%d bytes)", block.iff, block.unpadded_size);
		}
		if (ImGui::CollapsingHeader(name.c_str())) {
			ImGui::BeginChild("hexedit", ImVec2(0, ImGui::GetFontSize() * 20));
			editors[i].OptShowOptions = false;
			editors[i].DrawContents(block.data.data(), block.unpadded_size, 0);
			ImGui::EndChild();
		}
		ImGui::PopID();
	}
}

static void draw_tree(const char* page, std::vector<memcard::Block>& blocks, memcard::FileSchema& file_schema)
{
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	if (ImGui::BeginTable("table", 2, ImGuiTableFlags_RowBg)) {
		for (size_t i = 0; i < blocks.size(); i++) {
			memcard::Block& block = blocks[i];
			
			const memcard::BlockSchema* block_schema = file_schema.block(block.iff);
			if (!block_schema) {
				continue;
			}
			
			if (block_schema->page != page) {
				continue;
			}
			
			auto type_iter = s_game_types.find(block_schema->name);
			verify(type_iter != s_game_types.end(), "Cannot find type '%s'.", block_schema->name.c_str());
			
			const CppType& type = type_iter->second;
			draw_tree_node(type, type.name, block.data, i, 0, 0, 0);
		}
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}

static void draw_tree_node(
	const CppType& type, const std::string& name, std::span<u8> data, s32 index, s32 offset, s32 depth, s32 indent)
{
	ImGui::PushID(index);
	defer([&]() { ImGui::PopID(); });
	
	if (type.descriptor != CPP_TYPE_NAME) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		for (s32 i = 0; i < indent - 1; i++) {
			ImGui::Text(" ");
			ImGui::SameLine();
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", name.c_str());
		ImGui::TableNextColumn();
	}
	
	switch (type.descriptor) {
		case CPP_ARRAY: {
			bool& expanded = s_node_expanded[ImGui::GetID("expanded")];
			
			verify_fatal(type.array.element_type.get());
			ImGui::AlignTextToFramePadding();
			std::string element_count_str = stringf("[%d]", type.array.element_count);
			if (ImGui::Selectable(element_count_str.c_str(), expanded, ImGuiSelectableFlags_SpanAllColumns)) {
				expanded = !expanded;
			}
			
			const CppType* element_names_type = nullptr;
			if (const CppPreprocessorDirective* element_names = cpp_directive(type, CPP_DIRECTIVE_ELEMENTNAMES)) {
				element_names_type = &lookup_save_enum(element_names->string);
			}
			
			if (expanded) {
				for (s32 i = 0; i < type.array.element_count; i++) {
					std::string element_name;
					if (element_names_type) {
						for (const auto& [value, name] : element_names_type->enumeration.constants) {
							if (value == i) {
								element_name = name;
							}
						}
					}
					
					if (element_name.empty()) {
						element_name = std::to_string(i);
					}
					
					draw_tree_node(*type.array.element_type, element_name, data, i, offset + i * type.array.element_type->size, depth + 1, indent + 1);
				}
			}
			break;
		}
		case CPP_BITFIELD: {
			draw_bitfield_editor(type, data, offset);
			break;
		}
		case CPP_BUILT_IN: {
			draw_built_in_editor(type, data, offset);
			break;
		}
		case CPP_ENUM: {
			draw_enum_editor(type, type, data, offset);
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			bool& expanded = s_node_expanded[ImGui::GetID("expanded")];
			
			ImGui::AlignTextToFramePadding();
			std::string struct_name = stringf("struct %s", type.name.c_str());
			if (ImGui::Selectable(struct_name.c_str(), expanded, ImGuiSelectableFlags_SpanAllColumns)) {
				expanded = !expanded;
			}
			
			if (expanded) {
				for (s32 i = 0; i < type.struct_or_union.fields.size(); i++) {
					const CppType& field = type.struct_or_union.fields[i];
					draw_tree_node(field, field.name, data, i, offset + field.offset, depth + 1, indent + 1);
				}
			}
			
			break;
		}
		case CPP_TYPE_NAME: {
			auto iter = s_game_types.find(type.type_name.string);
			if (iter != s_game_types.end()) {
				draw_tree_node(iter->second, name, data, -1, offset, depth + 1, indent);
			} else {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%x", offset);
				ImGui::TableNextColumn();
				for (s32 i = 0; i < indent - 1; i++) {
					ImGui::Text(" ");
					ImGui::SameLine();
				}
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", name.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("(no definition available)");
			}
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			break;
		}
	}
}

static void draw_table(const char* page, const char* names)
{
	// Determine the number of block columns to draw.
	std::vector<memcard::Block*> active_blocks;
	std::vector<CppType*> block_types;
	s32 element_count = -1;
	for_each_block_from_page(
		page, s_file->blocks, s_game_schema->game,
		[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
			verify_fatal(type.descriptor == CPP_ARRAY);
			
			active_blocks.emplace_back(&block);
			block_types.emplace_back(type.array.element_type.get());
			
			if (element_count == -1) {
				element_count = type.array.element_count;
			} else {
				verify_fatal(element_count == type.array.element_count);
			}
		}
	);
	
	auto names_type_iter = s_game_types.find(names);
	verify_fatal(names_type_iter != s_game_types.end());
	CppType& names_type = names_type_iter->second;
	verify_fatal(names_type.descriptor == CPP_ENUM);
	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	if (ImGui::BeginTable("table", 1 + (s32) active_blocks.size(), ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn(names, ImGuiTableColumnFlags_None);
		for_each_block_from_page(
			page, s_file->blocks, s_game_schema->game,
			[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
				ImGui::TableSetupColumn(block_schema.name.c_str(), ImGuiTableColumnFlags_None);
			}
		);
		ImGui::TableHeadersRow();
		
		for (s32 row = 0; row < element_count; row++) {
			std::string row_name;
			for (auto& [value, name] : names_type.enumeration.constants) {
				if (value == row) {
					row_name = name;
				}
			}
			
			if (row_name.empty()) {
				row_name = stringf("%d", row);
			}
			
			ImGui::TableNextRow();
			
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", row_name.c_str());
			
			ImGui::PushID(row);
			
			for (s32 column = 0; column < (s32) active_blocks.size(); column++) {
				ImGui::PushID(column);
				
				ImGui::TableNextColumn();
				draw_table_editor(*block_types[column], active_blocks[column]->data, row * block_types[column]->size);
				
				ImGui::PopID();
			}
			
			ImGui::PopID();
		}
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}

static void draw_level_table(const memcard::Page& page)
{
	if (s_file->levels.empty()) {
		return;
	}
	
	// Determine the number of block columns to draw.
	s32 block_count = 0;
	for_each_block_from_page(
		page.tag.c_str(), s_file->levels[0], s_game_schema->level,
		[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
			block_count++;
		}
	);
	
	auto names_type_iter = s_game_types.find("Level");
	verify(names_type_iter != s_game_types.end(), "Cannot find type 'Level'.");
	CppType& names_type = names_type_iter->second;
	verify_fatal(names_type.descriptor == CPP_ENUM);
	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	if (ImGui::BeginTable("table", 1 + block_count, ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_None);
		for_each_block_from_page(
			page.tag.c_str(), s_file->levels[0], s_game_schema->level,
			[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
				ImGui::TableSetupColumn(block_schema.name.c_str(), ImGuiTableColumnFlags_None);
			}
		);
		ImGui::TableHeadersRow();
		
		// Generate the totals rows.
		if (page.display_stored_totals) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("TOTALS  (STORED)");
			
			s32 column = 0;
			for_each_block_from_page(
				page.tag.c_str(), s_file->levels[0], s_game_schema->level,
				[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
					ImGui::PushID(column);
					
					memcard::BlockSchema* total_block_schema = s_game_schema->game.block(block_schema.buddy);
					
					memcard::Block* total_block = nullptr;
					if (total_block_schema) {
						for (memcard::Block& block : s_file->blocks) {
							if (block.iff == total_block_schema->iff) {
								total_block = &block;
								break;
							}
						}
					}
					
					if (total_block) {
						ImGui::TableNextColumn();
						draw_table_editor(type, total_block->data, 0);
					} else {
						ImGui::TableNextColumn();
						ImGui::AlignTextToFramePadding();
						ImGui::Text("N/A");
					}
					
					ImGui::PopID();
					
					column++;
				}
			);
		}
		
		if (page.display_calculated_int_totals) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("TOTALS  (CALCULATED)");
			
			std::vector<s32> totals(block_count, 0);
			for (size_t row = 0; row < s_file->levels.size(); row++) {
				s32 column = 0;
				for_each_block_from_page(
					page.tag.c_str(), s_file->levels[row], s_game_schema->level,
					[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
						verify_fatal(column < block_count);
						verify_fatal(block.data.size() >= 4);
						
						totals[column] += *(s32*) &block.data[0];
						
						column++;
					}
				);
			}
			
			s32 column = 0;
			for_each_block_from_page(
				page.tag.c_str(), s_file->levels[0], s_game_schema->level,
				[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
					verify_fatal(column < block_count);
					
					ImGui::PushID(column);
					
					ImGui::TableNextColumn();
					ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
					ImGui::InputInt("##total", &totals[column], 0, 0, ImGuiInputTextFlags_ReadOnly);
					ImGui::PopStyleColor();
					
					ImGui::PopID();
					
					column++;
				}
			);
		}
		
		// Generate a divider.
		if (page.display_stored_totals || page.display_calculated_int_totals) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("---");
			for_each_block_from_page(
				page.tag.c_str(), s_file->levels[0], s_game_schema->level,
				[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
					ImGui::TableNextColumn();
					ImGui::AlignTextToFramePadding();
					ImGui::Text("---");
				}
			);
		}
		
		// Generate the level rows.
		for (size_t row = 0; row < s_file->levels.size(); row++) {
			std::string row_name;
			for (auto& [value, name] : names_type.enumeration.constants) {
				if (value == row) {
					row_name = name;
				}
			}
			
			if (row_name.empty()) {
				row_name = stringf("%d", (s32) row);
			}
			
			ImGui::TableNextRow();
			
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", row_name.c_str());
			
			ImGui::PushID(row);
			
			s32 column = 0;
			for_each_block_from_page(
				page.tag.c_str(), s_file->levels[row], s_game_schema->level,
				[&](memcard::Block& block, memcard::BlockSchema& block_schema, CppType& type) {
					ImGui::PushID(column++);
					
					ImGui::TableNextColumn();
					draw_table_editor(type, block.data, 0);
					
					ImGui::PopID();
				}
			);
			
			ImGui::PopID();
		}
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
}

static void draw_table_editor(const CppType& type, std::span<u8> data, s32 offset)
{
	switch (type.descriptor) {
		case CPP_BUILT_IN: {
			draw_built_in_editor(type, data, offset);
			break;
		}
		case CPP_ENUM: {
			draw_enum_editor(type, type, data, offset);
			break;
		}
		case CPP_TYPE_NAME: {
			auto iter = s_game_types.find(type.type_name.string);
			if (iter != s_game_types.end()) {
				draw_table_editor(type, data, offset);
			}
			break;
		}
		default: {}
	}
}

static void draw_built_in_editor(const CppType& type, std::span<u8> data, s32 offset)
{
	u64 temp = 0;
	verify_fatal(offset >= 0 && offset + type.size <= data.size());
	verify_fatal(type.size >= 0 && type.size <= 8);
	memcpy(&temp, &data[offset], type.size);
	
	if (type.built_in == CPP_BOOL) {
		bool value = temp;
		if (ImGui::Checkbox("##input", &value)) {
			memcpy(&data[offset], &value, 1);
		}
	} else if (const CppPreprocessorDirective* enum_name = cpp_directive(type, CPP_DIRECTIVE_ENUM)) {
		const CppType& enum_type = lookup_save_enum(enum_name->string);
		
		std::string name = std::to_string(temp);
		for (auto& [other_value, other_name] : enum_type.enumeration.constants) {
			if (other_value == temp) {
				name = other_name.c_str();
			}
		}
		
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::BeginCombo("##enum", name.c_str())) {
			for (auto& [other_value, other_name] : enum_type.enumeration.constants) {
				if (ImGui::Selectable(other_name.c_str(), other_value == temp)) {
					temp = other_value;
					memcpy(&data[offset], &temp, type.size);
				}
			}
			ImGui::EndCombo();
		}
	} else {
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		
		ImGuiDataType imgui_type = cpp_built_in_type_to_imgui_data_type(type);
		const char* format = ImGui::DataTypeGetInfo(imgui_type)->PrintFmt;
		
		bool bcd = cpp_directive(type, CPP_DIRECTIVE_BCD);
		if (bcd) {
			temp = from_bcd(static_cast<u8>(temp));
		}
		
		char data_as_string[64];
		ImGui::DataTypeFormatString(data_as_string, ARRAY_SIZE(data_as_string), imgui_type, &temp, format);
		
		if (ImGui::InputText("##input", data_as_string, ARRAY_SIZE(data_as_string))) {
			if (ImGui::DataTypeApplyFromText(data_as_string, imgui_type, &temp, format)) {
				if (bcd) {
					temp = to_bcd(static_cast<u8>(temp));
				}
				
				memcpy(&data[offset], &temp, type.size);
			}
		}
		
		ImGui::PopStyleColor();
	}
}

static void draw_enum_editor(const CppType& type, const CppType& underlying_type, std::span<u8> data, s32 offset)
{
	s32 value = 0;
	verify_fatal(offset >= 0 && offset + type.size <= data.size())
	verify_fatal(type.size >= 0 && type.size <= 4);
	memcpy(&value, &data[offset], type.size);
	
	std::string name = std::to_string(value);
	for (auto& [other_value, other_name] : type.enumeration.constants) {
		if (other_value == value) {
			name = other_name.c_str();
		}
	}
	
	ImGui::SetNextItemWidth(-1.f);
	if (ImGui::BeginCombo("##enum", name.c_str())) {
		for (auto& [other_value, other_name] : type.enumeration.constants) {
			if (ImGui::Selectable(other_name.c_str(), other_value == value)) {
				value = other_value;
				memcpy(&data[offset], &value, type.size);
			}
		}
		ImGui::EndCombo();
	}
}

static void draw_bitfield_editor(const CppType& type, std::span<u8> data, s32 offset)
{
	verify(type.bitfield.storage_unit_type->descriptor == CPP_BUILT_IN, "Bitfield has bad storage unit type.");
	verify(!cpp_is_built_in_signed(type.bitfield.storage_unit_type->built_in), "Bitfield can't be signed");
	
	u64 storage_unit = 0;
	verify_fatal(offset >= 0 && offset + type.size <= data.size());
	verify_fatal(type.size >= 0 && type.size <= 8);
	memcpy(&storage_unit, &data[offset], type.size);
	
	u64 bitfield = cpp_unpack_unsigned_bitfield(storage_unit, type.bitfield.bit_offset, type.bitfield.bit_size);
	
	s32 directive = -1;
	std::string directive_string;
	if (!type.preprocessor_directives.empty()) {
		directive = type.preprocessor_directives[0].type;
		directive_string = type.preprocessor_directives[0].string;
	}
	
	const CppType* enum_type = nullptr;
	if (directive == CPP_DIRECTIVE_BITFLAGS || directive == CPP_DIRECTIVE_ENUM) {
		enum_type = &lookup_save_enum(directive_string);
	}
	
	switch (directive) {
		case CPP_DIRECTIVE_BITFLAGS: {
			std::string flags;
			for (auto& [flag_pos, flag_name] : enum_type->enumeration.constants) {
				if (bitfield & flag_pos) {
					if (!flags.empty()) {
						flags += " | ";
					}
					flags += flag_name;
				}
			}
			
			// Create a combo box containing a check box for each bit flag
			// defined by the enum.
			ImGui::SetNextItemWidth(-1.f);
			if (ImGui::BeginCombo("##enum", flags.c_str())) {
				for (auto& [flag_pos, flag_name] : enum_type->enumeration.constants) {
					bool flag = bitfield & flag_pos;
					if (ImGui::Checkbox(flag_name.c_str(), &flag)) {
						bitfield = (bitfield & ~flag_pos) | (flag ? flag_pos : 0);
						storage_unit = cpp_zero_bitfield(storage_unit, type.bitfield.bit_offset, type.bitfield.bit_size);
						storage_unit |= cpp_pack_unsigned_bitfield(bitfield, type.bitfield.bit_offset, type.bitfield.bit_size);
						memcpy(&data[offset], &storage_unit, type.size);
					}
				}
				ImGui::EndCombo();
			}
			
			break;
		}
		case CPP_DIRECTIVE_ENUM: {
			std::string name = std::to_string(bitfield);
			for (auto& [other_value, other_name] : enum_type->enumeration.constants) {
				if (other_value == bitfield) {
					name = other_name.c_str();
				}
			}
			
			ImGui::SetNextItemWidth(-1.f);
			if (ImGui::BeginCombo("##enum", name.c_str())) {
				for (auto& [other_value, other_name] : enum_type->enumeration.constants) {
					if (ImGui::Selectable(other_name.c_str(), other_value == bitfield)) {
						bitfield = other_value;
						storage_unit = cpp_zero_bitfield(storage_unit, type.bitfield.bit_offset, type.bitfield.bit_size);
						storage_unit |= cpp_pack_unsigned_bitfield(bitfield, type.bitfield.bit_offset, type.bitfield.bit_size);
						memcpy(&data[offset], &storage_unit, type.size);
					}
				}
				ImGui::EndCombo();
			}
			
			break;
		}
		default: {
			ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
			
			ImGuiDataType imgui_type = cpp_built_in_type_to_imgui_data_type(*type.bitfield.storage_unit_type);
			const char* format = ImGui::DataTypeGetInfo(imgui_type)->PrintFmt;
			
			char bitfield_as_string[64];
			ImGui::DataTypeFormatString(bitfield_as_string, ARRAY_SIZE(bitfield_as_string), imgui_type, &bitfield, format);
			
			if (ImGui::InputText("##input", bitfield_as_string, ARRAY_SIZE(bitfield_as_string))) {
				if (ImGui::DataTypeApplyFromText(bitfield_as_string, imgui_type, &bitfield, format)) {
					storage_unit = cpp_zero_bitfield(storage_unit, type.bitfield.bit_offset, type.bitfield.bit_size);
					storage_unit |= cpp_pack_unsigned_bitfield(bitfield, type.bitfield.bit_offset, type.bitfield.bit_size);
					memcpy(&data[offset], &storage_unit, type.size);
				}
			}
			
			ImGui::PopStyleColor();
			break;
		}
	}
}

static void for_each_block_from_page(
	const char* page, std::vector<memcard::Block>& blocks, memcard::FileSchema& file_schema, BlockCallback callback)
{
	for (memcard::Block& block : blocks) {
		memcard::BlockSchema* block_schema = file_schema.block(block.iff);
		if (!block_schema || block_schema->page != page) {
			continue;
		}
		
		auto type = s_game_types.find(block_schema->name);
		verify_fatal(type != s_game_types.end());
		
		callback(block, *block_schema, type->second);
	}
}

static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type)
{
	if (cpp_is_built_in_integer(type.built_in)) {
		bool is_signed = cpp_is_built_in_signed(type.built_in);
		switch (type.size) {
			case 1: return is_signed ? ImGuiDataType_S8 : ImGuiDataType_U8;
			case 2: return is_signed ? ImGuiDataType_S16 : ImGuiDataType_U16;
			case 4: return is_signed ? ImGuiDataType_S32 : ImGuiDataType_U32;
			case 8: return is_signed ? ImGuiDataType_S64 : ImGuiDataType_U64;
		}
	} else if (cpp_is_built_in_float(type.built_in)) {
		switch (type.size) {
			case 4: return ImGuiDataType_Float;
			case 8: return ImGuiDataType_Double;
		}
	}
	return ImGuiDataType_U8;
}

static const CppType& lookup_save_enum(const std::string& name)
{
	auto enum_type_iter = s_game_types.find(name);
	verify(enum_type_iter != s_game_types.end(), "Failed to lookup enum type '%s'.\n", name.c_str());
	const CppType& enum_type = enum_type_iter->second;
	verify(enum_type.descriptor == CPP_ENUM, "Type '%s' is not an enum.", enum_type.name.c_str());
	return enum_type;
}

static std::string lookup_level_name(s32 level_number)
{
	const CppType& level_enum = lookup_save_enum("Level");
	for (auto& [value, name] : level_enum.enumeration.constants) {
		if (value == level_number) {
			return name;
		}
	}
	
	return "UNKNOWN";
}

static u8 from_bcd(u8 value)
{
	return (value & 0xf) + ((value & 0xf0) >> 4) * 10;
}

static u8 to_bcd(u8 value)
{
	return (value % 10) | (value / 10) << 4;
}
