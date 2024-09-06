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
static void begin_dock_space();
static void create_dock_layout();
static void do_load();
static void do_save();
static void slot_page();
static void blocks_page();
static void blocks_sub_page(std::vector<memcard::Block>& blocks, memcard::FileSchema* file_schema);
static void draw_editor(const CppType& type, std::span<u8> data);
static void draw_editor_rows(
	const CppType& type, const std::string& name, std::span<u8> data, s32 index, s32 offset, s32 depth, s32 indent);
static void draw_build_in_editor(const CppType& type, std::span<u8> data, s32 offset);
static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type);
static u8 from_bcd(u8 value);
static u8 to_bcd(u8 value);

static FileInputStream memcardwad;
static memcard::Schema schema;

static std::string directory;
static std::vector<fs::path> file_paths;
static bool should_reload_file_list = true;
static fs::path selected_file_path;
static bool should_load_now = false;
static bool should_save_now = false;
static Opt<memcard::File> file;
static std::string error_message;

static Game game = Game::UNKNOWN;
static memcard::GameSchema* game_schema = nullptr;
std::map<std::string, CppType> game_types;

std::map<ImGuiID, bool> node_expanded;

struct Page
{
	const char* name;
	void (*draw_func)();
	bool (*cond_func)();
};

static Page PAGES[] = {
	// // net
	// {"Profiles", &profiles_page},
	// {"Profile Statistics", &profile_stats_page},
	// {"Game Modes", &game_modes_page},
	// slot
	{"Slot", &slot_page, []() {
		return file.has_value() && file->type == memcard::FileType::SLOT;
	}},
	// {"Bots", &bots_page},
	// {"Enemy Kills", &enemy_kills_page},
	// {"Gadgets", &gadget_page},
	// {"Help", &help_page},
	// {"Hero", &hero_page<memory_card::UyaHeroSave>},
	// {"Hero", &hero_page<memory_card::DlHeroSave>},
	// {"Settings", &settings_page},
	// {"Statistics", &statistics_page},
	// {"Levels", &levels_page},
	// {"Missions", &missions_page},
	// sections
	{"Blocks", &blocks_page, []() {
		return file.has_value() && (file->type == memcard::FileType::NET || file->type == memcard::FileType::SLOT);
	}}
};


int main(int argc, char** argv)
{
	WadPaths wads = find_wads(argv[0]);
	verify(g_guiwad.open(wads.gui), "Failed to open gui wad.");
	verify(memcardwad.open(wads.memcard), "Failed to open memcard wad.");
	
	memcardwad.seek(wadinfo.memcard.savegame.offset.bytes());
	std::vector<u8> schema_compressed = memcardwad.read_multiple<u8>(wadinfo.memcard.savegame.size.bytes());
	
	std::vector<u8> schema_wtf;
	decompress_wad(schema_wtf, schema_compressed);

	schema = memcard::parse_schema((char*) schema_wtf.data());
	
	if(argc > 1) {
		directory = argv[1];
	}
	
	u64 frame = 0;
	
	GLFWwindow* window = gui::startup("Wrench Save Editor", 1280, 720);
	gui::load_font(wadinfo.gui.fonts[0], 22);
	while(!glfwWindowShouldClose(window)) {
		gui::run_frame(window, update_gui);
		
		if(should_load_now) {
			do_load();
			should_load_now = false;
		}
		
		if(should_save_now) {
			do_save();
			should_save_now = false;
		}
		
		if((frame % 60) == 0) {
			should_reload_file_list = true;
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
	if(is_first_frame) {
		create_dock_layout();
		is_first_frame = false;
	}
	
	ImGui::End(); // dock space
}

static void files()
{
	static std::string listing_error;
	
	if(gui::input_folder_path(&directory, "##directory", nullptr) || should_reload_file_list) {
		file_paths.clear();
		try {
			for(auto entry : fs::directory_iterator(directory)) {
				file_paths.emplace_back(entry.path());
			}
			listing_error.clear();
		} catch(std::filesystem::filesystem_error& error) {
			listing_error = error.code().message();
		}
		std::sort(BEGIN_END(file_paths));
		should_reload_file_list = false;
	}
	
	if(listing_error.empty()) {
		ImGui::BeginChild("##files");
		if(ImGui::Selectable("[DIR] .")) {
			should_reload_file_list = true;
		}
		if(ImGui::Selectable("[DIR] ..")) {
			directory = fs::weakly_canonical(fs::path(directory)).parent_path().string();
			should_reload_file_list = true;
		}
		for(auto& path : file_paths) {
			if(fs::is_directory(path)) {
				std::string label = stringf("[DIR] %s", path.filename().string().c_str());
				if(ImGui::Selectable(label.c_str())) {
					directory = path.string();
					should_reload_file_list = true;
				}
			}
		}
		for(auto& path : file_paths) {
			if(fs::is_regular_file(path)) {
				if(ImGui::Selectable(path.filename().string().c_str(), path == selected_file_path)) {
					should_load_now = true;
					selected_file_path = path;
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
	if(ImGui::Button("Save")) {
		should_save_now = true;
	}
	
	if(ImGui::Button("Save As")) {
		nfdchar_t* path = nullptr;
		nfdresult_t result = NFD_OpenDialog("iso", nullptr, &path);
		if(result != NFD_OKAY) {
			if(result != NFD_CANCEL) {
				printf("error: %s\n", NFD_GetError());
			}
			return;
		}
		
		file->path = path;
		should_save_now = true;
		
		directory = file->path.parent_path().string();
		should_reload_file_list = true;
		
		free(path);
	}
}

static void editor()
{
	if(!error_message.empty()) {
		ImGui::Text("%s", error_message.c_str());
		return;
	}
	
	if(!file.has_value() || !file.has_value()) {
		ImGui::Text("No file loaded.");
		return;
	}
	
	if(file->checksum_does_not_match) {
		ImGui::Text("Save game checksum doesn't match!");
		ImGui::SameLine();
		if(ImGui::Button("Dismiss")) {
			file->checksum_does_not_match = false;
		}
	}
	
	if(ImGui::BeginTabBar("##tabs")) {
		for(Page& page : PAGES) {
			if(page.cond_func() && ImGui::BeginTabItem(page.name)) {
				ImGui::BeginChild("##tab");
				page.draw_func();
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}
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
	if(!selected_file_path.empty()) {
		try {
			std::vector<u8> buffer = read_file(selected_file_path);
			file = memcard::read(buffer, selected_file_path);
			error_message.clear();
			
			s32 type_index = -1;
			switch(file->slot.blocks.size()) {
				case 47: {
					game = Game::RAC;
					type_index = 0;
					break;
				}
				case 34: {
					game = Game::GC;
					type_index = 1;
					break;
				}
				case 40: {
					game = Game::UYA;
					type_index = 2;
					break;
				}
				case 29: {
					game = Game::DL;
					type_index = 3;
					break;
				}
				default: {
					game = Game::RAC;
				}
			}
			
			game_types.clear();
			game_schema = schema.game(game);
			
			// Load type information used by the editor.
			if(type_index > -1) {
				memcardwad.seek(wadinfo.memcard.types[type_index].offset.bytes());
				std::vector<u8> types_compressed =
					memcardwad.read_multiple<u8>(wadinfo.memcard.types[type_index].size.bytes());
				
				std::vector<u8> types_cpp;
				decompress_wad(types_cpp, types_compressed);
				types_cpp.emplace_back(0);
				
				std::vector<CppToken> tokens = eat_cpp_file((char*) types_cpp.data());
				parse_cpp_types(game_types, std::move(tokens));
				
				for(auto& [name, type] : game_types) {
					layout_cpp_type(type, game_types, CPP_PS2_ABI);
				}
			}
		} catch(RuntimeError& error) {
			error_message = (error.context.empty() ? "" : (error.context + ": ")) + error.message;
		}
	}
}

static void do_save()
{
	if(file.has_value()) {
		try {
			std::vector<u8> buffer;
			memcard::write(buffer, *file);
			write_file(file->path, buffer);
		} catch(RuntimeError& error) {
			error_message = (error.context.empty() ? "" : (error.context + ": ")) + error.message;
		}
		should_reload_file_list = true;
	}
}

static void slot_page()
{
	if(!game_schema) {
		return;
	}
	
	for(size_t i = 0; i < file->slot.blocks.size(); i++) {
		memcard::Block& block = file->slot.blocks[i];
		ImGui::PushID((int) i);
		
		const memcard::BlockSchema* block_schema = game_schema->game.block(block.type);
		if(!block_schema) {
			ImGui::PopID();
			continue;
		}
		
		ImGui::Text("%s", block_schema->name.c_str());
		
		auto type_iter = game_types.find(block_schema->type);
		if(type_iter == game_types.end()) {
			ImGui::PopID();
			continue;
		}
		
		const CppType& type = type_iter->second;
		draw_editor(type, block.data);
		
		ImGui::PopID();
	}
}

static void blocks_page()
{
	memcard::GameSchema* gs = game_schema;
	
	switch(file->type) {
		case memcard::FileType::NET: {
			blocks_sub_page(file->net.blocks, gs ? &gs->net : nullptr);
			break;
		}
		case memcard::FileType::SLOT: {
			if(ImGui::BeginTabBar("subpages")) {
				if(ImGui::BeginTabItem("Game")) {
					blocks_sub_page(file->slot.blocks, gs ? &gs->game : nullptr);
					ImGui::EndTabItem();
				}
				
				for(size_t i = 0; i < file->slot.levels.size(); i++) {
					if(ImGui::BeginTabItem(stringf("L%d", (s32) i).c_str())) {
						blocks_sub_page(file->slot.levels[i], gs ? &gs->level : nullptr);
						ImGui::EndTabItem();
					}
				}
				
				ImGui::EndTabBar();
			}
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
	for(size_t i = 0; i < blocks.size(); i++) {
		ImGui::PushID(i);
		memcard::Block& block = blocks[i];
		std::string name;
		if(file_schema) {
			memcard::BlockSchema* block_schema = file_schema->block(block.type);
			if(block_schema) {
				name = stringf("%4d: %s (%d bytes)", block.type, block_schema->name.c_str(), block.unpadded_size);
			}
		}
		if(name.empty()) {
			name = stringf("%4d: unknown (%d bytes)", block.type, block.unpadded_size);
		}
		if(ImGui::CollapsingHeader(name.c_str())) {
			ImGui::BeginChild("hexedit", ImVec2(0, ImGui::GetFontSize() * 20));
			editors[i].OptShowOptions = false;
			editors[i].DrawContents(block.data.data(), block.unpadded_size, 0);
			ImGui::EndChild();
		}
		ImGui::PopID();
	}
}

static void draw_editor(const CppType& type, std::span<u8> data)
{
	ImGui::BeginChild("editor", ImVec2(0, 200));
	
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	if(ImGui::BeginTable("table", 3, ImGuiTableFlags_RowBg)) {
		draw_editor_rows(type, type.name, data, -1, 0, 0, 0);
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	
	ImGui::EndChild();
}

static void draw_editor_rows(
	const CppType& type, const std::string& name, std::span<u8> data, s32 index, s32 offset, s32 depth, s32 indent)
{
	ImGui::PushID(index);
	defer([&]() { ImGui::PopID(); });
	
	if(depth > 0 && type.descriptor != CPP_TYPE_NAME) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%x", offset);
		ImGui::TableNextColumn();
		for(s32 i = 0; i < indent - 1; i++) {
			ImGui::Text(" ");
			ImGui::SameLine();
		}
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", name.c_str());
		ImGui::TableNextColumn();
	}
	
	switch(type.descriptor) {
		case CPP_ARRAY: {
			bool& expanded = node_expanded[ImGui::GetID("expanded")];
			
			verify_fatal(type.array.element_type.get());
			ImGui::AlignTextToFramePadding();
			std::string element_count_str = stringf("[%d]", type.array.element_count);
			if(ImGui::Selectable(element_count_str.c_str(), expanded, ImGuiSelectableFlags_SpanAllColumns)) {
				expanded = !expanded;
			}
			if(expanded) {
				for(s32 i = 0; i < type.array.element_count; i++) {
					std::string element_name = std::to_string(i);
					draw_editor_rows(*type.array.element_type, element_name, data, i, offset + i * type.array.element_type->size, depth + 1, indent + 1);
				}
			}
			break;
		}
		case CPP_BUILT_IN: {
			draw_build_in_editor(type, data, offset);
			break;
		}
		case CPP_ENUM: {
			//generate_enum_input(type, data, offset);
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			bool& expanded = node_expanded[ImGui::GetID("expanded")];
			
			if(depth > 0) {
				ImGui::AlignTextToFramePadding();
				std::string struct_name = stringf("struct %s", type.name.c_str());
				if(ImGui::Selectable(struct_name.c_str(), expanded, ImGuiSelectableFlags_SpanAllColumns)) {
					expanded = !expanded;
				}
			}
			if(expanded || depth == 0) {
				for(s32 i = 0; i < type.struct_or_union.fields.size(); i++) {
					const CppType& field = type.struct_or_union.fields[i];
					draw_editor_rows(field, field.name, data, i, offset + field.offset, depth + 1, indent + 1);
				}
			}
			
			break;
		}
		case CPP_TYPE_NAME: {
			auto iter = game_types.find(type.type_name.string);
			if(iter != game_types.end()) {
				draw_editor_rows(iter->second, name, data, -1, offset, depth + 1, indent);
			} else {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%x", offset);
				ImGui::TableNextColumn();
				for(s32 i = 0; i < indent - 1; i++) {
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

static void draw_build_in_editor(const CppType& type, std::span<u8> data, s32 offset)
{
	u8 temp[16];
	verify_fatal(offset >= 0 && offset + type.size <= data.size())
	verify_fatal(type.size >= 0 && type.size <= 16);
	memcpy(temp, &data[offset], type.size);
	
	ImGuiDataType imgui_type = cpp_built_in_type_to_imgui_data_type(type);
	const char* format = ImGui::DataTypeGetInfo(imgui_type)->PrintFmt;
	
	char data_as_string[64];
	ImGui::DataTypeFormatString(data_as_string, ARRAY_SIZE(data_as_string), imgui_type, temp, format);
	
	if(ImGui::InputText("##input", data_as_string, ARRAY_SIZE(data_as_string), ImGuiInputTextFlags_EnterReturnsTrue)) {
		if(ImGui::DataTypeApplyFromText(data_as_string, imgui_type, temp, format)) {
			memcpy(&data[offset], temp, type.size);
		}
	}
}

static ImGuiDataType cpp_built_in_type_to_imgui_data_type(const CppType& type)
{
	if(cpp_is_built_in_integer(type.built_in)) {
		bool is_signed = cpp_is_built_in_signed(type.built_in);
		switch(type.size) {
			case 1: return is_signed ? ImGuiDataType_S8 : ImGuiDataType_U8;
			case 2: return is_signed ? ImGuiDataType_S16 : ImGuiDataType_U16;
			case 4: return is_signed ? ImGuiDataType_S32 : ImGuiDataType_U32;
			case 8: return is_signed ? ImGuiDataType_S64 : ImGuiDataType_U64;
		}
	} else if(cpp_is_built_in_float(type.built_in)) {
		switch(type.size) {
			case 4: return ImGuiDataType_Float;
			case 8: return ImGuiDataType_Double;
		}
	}
	return ImGuiDataType_U8;
}

static u8 from_bcd(u8 value)
{
	return (value & 0xf) + ((value & 0xf0) >> 4) * 10;
}

static u8 to_bcd(u8 value)
{
	return (value % 10) | (value / 10) << 4;
}
