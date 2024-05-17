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

#include <core/filesystem.h>
#include <engine/compression.h>
#include <gui/gui.h>
#include <toolwads/wads.h>
#include <saveeditor/memory_card.h>

#include "imgui_club/imgui_memory_editor.h"

static void update_gui(f32 delta_time);
static void files();
static void sections();
static void editor();
static void begin_dock_space();
static void create_dock_layout();
static void do_load();
static void do_save();
static bool blocks_page(bool draw_gui);
static void blocks_sub_page(std::vector<memcard::Block>& blocks, memcard::FileSchema* file_schema);
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

struct Page {
	const char* name;
	bool (*func)(bool draw_gui);
};

static Page PAGES[] = {
	// // net
	// {"Profiles", &profiles_page},
	// {"Profile Statistics", &profile_stats_page},
	// {"Game Modes", &game_modes_page},
	// // slot
	// {"Slot", &slot_page},
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
	{"Blocks", &blocks_page}
};


int main(int argc, char** argv) {
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

static void update_gui(f32 delta_time) {
	begin_dock_space();
	
	ImGuiWindowClass window_class;
	window_class.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Files", nullptr, ImGuiWindowFlags_NoTitleBar);
	files();
	ImGui::End();
	
	ImGui::SetNextWindowClass(&window_class);
	ImGui::Begin("Sections", nullptr, ImGuiWindowFlags_NoTitleBar);
	sections();
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

static void files() {
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

static void sections() {
	if(ImGui::Button("Save Current File")) {
		should_save_now = true;
	}
}

static void editor() {
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
			if(page.func(false) && ImGui::BeginTabItem(page.name)) {
				ImGui::BeginChild("##tab");
				page.func(true);
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}
}

static void begin_dock_space() {
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

static void create_dock_layout() {
	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1.f, 1.f));

	ImGuiID left, editor;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 8.f / 10.f, &left, &editor);
	
	ImGuiID files, sections;
	ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 8.f / 10.f, &files, &sections);
	
	ImGui::DockBuilderDockWindow("Files", files);
	ImGui::DockBuilderDockWindow("Sections", sections);
	ImGui::DockBuilderDockWindow("Editor", editor);

	ImGui::DockBuilderFinish(dockspace_id);
}

static void do_load() {
	if(!selected_file_path.empty()) {
		try {
			std::vector<u8> buffer = read_file(selected_file_path);
			file = memcard::read(buffer, selected_file_path);
			error_message.clear();
		} catch(RuntimeError& error) {
			error_message = (error.context.empty() ? "" : (error.context + ": ")) + error.message;
		}
	}
}

static void do_save() {
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

static bool blocks_page(bool draw_gui) {
	if(!draw_gui) {
		return file.has_value() && (file->type == memcard::FileType::NET || file->type == memcard::FileType::SLOT);
	}
	
	memcard::GameSchema* gs = schema.game(Game::RAC);
	
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
	
	return true;
}

static void blocks_sub_page(std::vector<memcard::Block>& blocks, memcard::FileSchema* file_schema) {
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

template <typename T>
static ImGuiDataType get_imgui_type() {
	using Type = std::remove_reference_t<T>;
	if constexpr(std::is_same_v<Type, s8>) return ImGuiDataType_S8;
	if constexpr(std::is_same_v<Type, u8>) return ImGuiDataType_U8;
	if constexpr(std::is_same_v<Type, s16>) return ImGuiDataType_S16;
	if constexpr(std::is_same_v<Type, u16>) return ImGuiDataType_U16;
	if constexpr(std::is_same_v<Type, s32>) return ImGuiDataType_S32;
	if constexpr(std::is_same_v<Type, u32>) return ImGuiDataType_U32;
	if constexpr(std::is_same_v<Type, s64>) return ImGuiDataType_S64;
	if constexpr(std::is_same_v<Type, u64>) return ImGuiDataType_U64;
	if constexpr(std::is_same_v<Type, f32>) return ImGuiDataType_Float;
	if constexpr(std::is_same_v<Type, f64>) return ImGuiDataType_Double;
	verify_not_reached_fatal("Bad field type.");
}

enum FieldWidget {
	SCALAR, CHECKBOX
};

struct FieldDecorator {
	FieldDecorator() : widget(SCALAR) {}
	FieldDecorator(FieldWidget w) : widget(w) {}
	FieldWidget widget;
};

// I'm using macros instead of functions here so that the compiler doesn't
// complain about misaligned pointers (even if in practice they should be aligned).
#define input_scalar(label, value, ...) \
	{ \
		FieldDecorator decorator{__VA_ARGS__}; \
		if(raw_mode) decorator = SCALAR; \
		ImGuiDataType data_type = get_imgui_type<decltype(value)>();\
		switch(decorator.widget) { \
			case SCALAR: { \
				auto temp = value; \
				if(ImGui::InputScalar(label, data_type, &temp)) { \
					should_save_now = true; \
				} \
				value = temp; \
				break; \
			} \
			case CHECKBOX: { \
				bool temp = (bool) value; \
				if(ImGui::Checkbox(label, &temp)) { \
					should_save_now = true; \
				} \
				value = temp; \
				break; \
			} \
		} \
	}
#define input_array(label, array) \
	{ \
		decltype(array) temp; \
		memcpy(temp, array, sizeof(array)); \
		ImGuiDataType data_type = get_imgui_type<decltype(array[0])>(); \
		if(ImGui::InputScalarN(label, data_type, temp, ARRAY_SIZE(array), nullptr, nullptr, nullptr, ImGuiInputTextFlags_None)) { \
			should_save_now = true; \
		} \
		memcpy(array, temp, sizeof(array)); \
	}
#define input_array_2d(label, array) \
	{ \
		s32 rows = ARRAY_SIZE(array); \
		s32 columns = ARRAY_SIZE(array[0]); \
		decltype(array) temp; \
		memcpy(temp, array, sizeof(array)); \
		for(s32 i = 0; i < rows; i++) { \
			std::string row_label = stringf("%s %d", label, i); \
			ImGuiDataType data_type = get_imgui_type<decltype(array[0][0])>(); \
			if(ImGui::InputScalarN(row_label.c_str(), data_type, temp[i], columns, nullptr, nullptr, nullptr, ImGuiInputTextFlags_None)) { \
				should_save_now = true; \
			} \
		} \
		memcpy(array, temp, sizeof(array)); \
	}
#define input_text(label, value) \
	{ \
		if(ImGui::InputText(label, (char*) ARRAY_PAIR(value))) { \
			should_save_now = true; \
		} \
	}
#define input_clock(label, value) \
	{ \
		u8 clock[6]; \
		clock[0] = from_bcd((value).second); \
		clock[1] = from_bcd((value).minute); \
		clock[2] = from_bcd((value).hour); \
		clock[3] = from_bcd((value).day); \
		clock[4] = from_bcd((value).month); \
		clock[5] = from_bcd((value).year); \
		input_array(label, clock); \
		(value).second = to_bcd(clock[0]); \
		(value).minute = to_bcd(clock[1]); \
		(value).hour = to_bcd(clock[2]); \
		(value).day = to_bcd(clock[3]); \
		(value).month = to_bcd(clock[4]); \
		(value).year = to_bcd(clock[5]); \
	}

static u8 from_bcd(u8 value) {
	return (value & 0xf) + ((value & 0xf0) >> 4) * 10;
}

static u8 to_bcd(u8 value) {
	return (value % 10) | (value / 10) << 4;
}
