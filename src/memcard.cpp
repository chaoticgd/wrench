/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include <core/filesystem.h>
#include <core/memory_card.h>
#include <gui/gui.h>
#include <toolwads/wads.h>

static void update_gui(f32 delta_time);
static void files();
static void sections();
static void editor();
static void begin_dock_space();
static void create_dock_layout();
static void do_load();
static void do_save();
static bool game_modes_page(bool draw_gui);
static bool profiles_page(bool draw_gui);
static bool slot_page(bool draw_gui);
static bool bots_page(bool draw_gui);
static bool enemy_kills_page(bool draw_gui);
static bool gadget_page(bool draw_gui);
static void gadget_general_subpage();
static void gadget_entries_subpage();
static void gadget_events_subpage();
static void gadget_messages_subpage();
static bool help_page(bool draw_gui);
static void help_subpage(const char* label, std::vector<memory_card::HelpDatum>& help);
static bool hero_page(bool draw_gui);
static bool settings_page(bool draw_gui);
static bool statistics_page(bool draw_gui);
static bool levels_page(bool draw_gui);
static bool missions_page(bool draw_gui);
static u8 from_bcd(u8 value);
static u8 to_bcd(u8 value);

struct Page {
	const char* name;
	bool (*func)(bool draw_gui);
	bool visible = false;
};

static Page PAGES[] = {
	// net
	{"Game Modes", &game_modes_page},
	{"Profiles", &profiles_page},
	{"General Stats", &profiles_page},
	{"Siege Match Stats", &profiles_page},
	{"Death Match Stats", &profiles_page},
	// slot
	{"Slot", &slot_page},
	{"Bots", &bots_page},
	{"Enemy Kills", &enemy_kills_page},
	{"Gadgets", &gadget_page},
	{"Help", &help_page},
	{"Hero", &hero_page},
	{"Settings", &settings_page},
	{"Statistics", &statistics_page},
	{"Levels", &levels_page},
	{"Missions", &missions_page}
};

static std::string directory = "/home/thomas/pcsx2/memcards/folder_card.ps2/BESCES-53285RATCHET/";
static std::vector<fs::path> file_paths;
static bool should_reload_file_list = true;
static fs::path selected_file_path;
static bool should_load_now = false;
static bool should_save_now = false;
static Opt<memory_card::File> file;
static std::string error_message;
static memory_card::SaveGame save;

int main(int argc, char** argv) {
	WadPaths wads = find_wads(argv[0]);
	g_guiwad.open(wads.gui);
	
	s64 frame = 0;
	
	GLFWwindow* window = gui::startup("Wrench Memory Card Editor", 1280, 720);
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
			fflush(stdout);
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
				if(entry.path().extension() != ".backup") {
					file_paths.emplace_back(entry.path());
				}
			}
			listing_error.clear();
		} catch(std::filesystem::filesystem_error& error) {
			listing_error = error.code().message();
		}
		std::sort(BEGIN_END(file_paths));
		should_reload_file_list = false;
	}
	
	if(!listing_error.empty()) {
		ImGui::Text("%s", listing_error.c_str());
	}
	
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
				directory = path;
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
}

static void sections() {
	
}

static void editor() {
	if(!error_message.empty()) {
		ImGui::Text("%s", error_message.c_str());
		return;
	}
	
	if(!file.has_value() || !save.loaded) {
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
			if(page.visible && ImGui::BeginTabItem(page.name)) {
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
			file = memory_card::read(buffer, selected_file_path);
			error_message.clear();
			save = memory_card::parse(*file);
			for(Page& page : PAGES) {
				page.visible = page.func(false);
			}
		} catch(RuntimeError& error) {
			error_message = error.message;
		}
	}
}

static void do_save() {
	if(file.has_value() && save.loaded) {
		try {
			memory_card::update(*file, save);
			std::vector<u8> buffer;
			memory_card::write(buffer, *file);
			fs::path backup_path = file->path.replace_extension("backup");
			if(fs::exists(file->path) && !fs::exists(backup_path)) {
				fs::copy(file->path, backup_path);
			}
			write_file(file->path, buffer);
		} catch(RuntimeError& error) {
			error_message = error.message;
		}
		should_reload_file_list = true;
	}
}

// I'm using macros instead of functions here so that the compiler doesn't
// complain about misaligned pointers (even if in practice they should be aligned).
#define input_scalar(data_type, label, value) \
	{ \
		auto temp = value; \
		if(ImGui::InputScalar(label, data_type, &temp)) { \
			should_save_now = true; \
		} \
		value = temp; \
	}
#define input_scalar_n(data_type, label, array) \
	{ \
		decltype(array) temp; \
		memcpy(temp, array, sizeof(array)); \
		if(ImGui::InputScalarN(label, data_type, temp, ARRAY_SIZE(array), nullptr, nullptr, nullptr, ImGuiInputTextFlags_None)) { \
			should_save_now = true; \
		} \
		memcpy(array, temp, sizeof(array)); \
	}
#define input_scalar_multi(data_type, label, value, rows, columns) \
	{ \
		decltype(value) temp; \
		memcpy(temp, value, sizeof(value)); \
		for(s32 i = 0; i < rows; i++) { \
			std::string row_label = stringf("%s %d", label, i); \
			if(ImGui::InputScalarN(row_label.c_str(), data_type, temp[i], columns, nullptr, nullptr, nullptr, ImGuiInputTextFlags_None)) { \
				should_save_now = true; \
			} \
		} \
		memcpy(value, temp, sizeof(value)); \
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
		input_scalar_n(ImGuiDataType_U8, label, clock); \
		(value).second = to_bcd(clock[0]); \
		(value).minute = to_bcd(clock[1]); \
		(value).hour = to_bcd(clock[2]); \
		(value).day = to_bcd(clock[3]); \
		(value).month = to_bcd(clock[4]); \
		(value).year = to_bcd(clock[5]); \
	}

static bool game_modes_page(bool draw_gui) {
	return true;
}

static bool profiles_page(bool draw_gui) {
	return true;
}

static bool slot_page(bool draw_gui) {
	if(save.type != memory_card::FileType::SLOT) return false;
	if(!draw_gui) return true;
	
	if(save.level.has_value()) input_scalar(ImGuiDataType_S32, "Level", *save.level);
	if(save.elapsed_time.has_value()) input_scalar(ImGuiDataType_S32, "Elapsed Time", *save.elapsed_time);
	if(save.last_save_time.has_value()) input_clock("Last Save Time (smhdmy)", *save.last_save_time)
	if(save.global_flags.has_value()) input_scalar_n(ImGuiDataType_U8, "Global Flags", save.global_flags->array);
	if(save.global_flags.has_value()) input_scalar_n(ImGuiDataType_U8, "Global Flags", save.global_flags->array);
	if(save.cheats_activated.has_value()) input_scalar_n(ImGuiDataType_U8, "Cheats Activated", save.cheats_activated->array);
	if(save.skill_points.has_value()) input_scalar_n(ImGuiDataType_S32, "Skill Points", save.skill_points->array);
	if(save.cheats_ever_activated.has_value()) input_scalar_n(ImGuiDataType_U8, "Cheats Ever Activated", save.cheats_ever_activated->array);
	if(save.movies_played_record.has_value()) input_scalar_n(ImGuiDataType_U32, "Movies Played Record", save.movies_played_record->array);
	if(save.total_play_time.has_value()) input_scalar(ImGuiDataType_S32, "Total Play Time", *save.total_play_time);
	if(save.total_deaths.has_value()) input_scalar(ImGuiDataType_S32, "Total Deaths", *save.total_deaths);
	if(save.purchaseable_gadgets.has_value()) input_scalar_n(ImGuiDataType_U8, "Purchaseable Gadgets", save.purchaseable_gadgets->array);
	if(save.first_person_desired_mode.has_value()) input_scalar_n(ImGuiDataType_S32, "First Person Desired Mode", save.first_person_desired_mode->array);
	if(save.saved_difficulty_level.has_value()) input_scalar(ImGuiDataType_S32, "Saved Difficulty Level", *save.saved_difficulty_level);
	if(save.battledome_wins_and_losses.has_value()) input_scalar_n(ImGuiDataType_S32, "Battledome Wins and Losses", save.battledome_wins_and_losses->array);
	if(save.quick_switch_gadgets.has_value()) input_scalar_multi(ImGuiDataType_S32, "Quick Select Gadgets", save.quick_switch_gadgets->array, 4, 3);
	
	return true;
}

static bool bots_page(bool draw_gui) {
	if(!save.bot_save.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::BotSave& s = *save.bot_save;
	
	input_scalar_n(ImGuiDataType_S32, "Bot Upgrades", s.bot_upgrades);
	input_scalar_n(ImGuiDataType_S32, "Bot Paintjobs", s.bot_upgrades);
	input_scalar_n(ImGuiDataType_S32, "Bot Heads", s.bot_upgrades);
	input_scalar_n(ImGuiDataType_S32, "Current Bot Paint Job", s.bot_upgrades);
	input_scalar_n(ImGuiDataType_S32, "Current Bot Head", s.bot_upgrades);
	
	return true;
}

static bool enemy_kills_page(bool draw_gui) {
	if(!save.enemy_kills.has_value()) return false;
	if(!draw_gui) return true;
	
	ImGui::BeginTable("##enemy_kills", 3, ImGuiTableFlags_RowBg);
	ImGui::TableSetupColumn("Index");
	ImGui::TableSetupColumn("Enemy Class");
	ImGui::TableSetupColumn("Kill Count");
	ImGui::TableHeadersRow();
	for(s32 i = 0; i < ARRAY_SIZE(save.enemy_kills->array); i++) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%d", i);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##o_class", save.enemy_kills->array[i].o_class);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##kills", save.enemy_kills->array[i].kills);
		ImGui::PopID();
	}
	ImGui::EndTable();
	
	return true;
}

static bool gadget_page(bool draw_gui) {
	if(!save.hero_gadget_box.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	if(ImGui::BeginTabBar("##gadget_tabs")) {
		if(ImGui::BeginTabItem("General")) {
			gadget_general_subpage();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Entries")) {
			gadget_entries_subpage();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Events")) {
			gadget_events_subpage();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Event Messages")) {
			gadget_messages_subpage();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static void gadget_general_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	input_scalar(ImGuiDataType_U8, "Initialized", g.initialized);
	input_scalar(ImGuiDataType_U8, "Level", g.level);
	input_scalar_n(ImGuiDataType_U8, "Button Down", g.button_down);
	input_scalar_n(ImGuiDataType_S16, "Button Up Frames", g.button_up_frames);
	input_scalar(ImGuiDataType_U8, "Num Gadget Events", g.num_gadget_events);
	input_scalar_n(ImGuiDataType_U8, "Mod Basic", g.mod_basic);
	input_scalar(ImGuiDataType_S16, "Mod Post FX", g.mod_post_fx);
	input_scalar(ImGuiDataType_U32, "Gadget Event Pointer", g.p_next_gadget_event);
}

static void gadget_entries_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	ImGui::BeginTable("##gadget_entries", 9, ImGuiTableFlags_RowBg);
	ImGui::TableSetupColumn("Index");
	ImGui::TableSetupColumn("Level");
	ImGui::TableSetupColumn("Ammo");
	ImGui::TableSetupColumn("XP");
	ImGui::TableSetupColumn("Action Frame");
	ImGui::TableSetupColumn("Mod Active Post FX");
	ImGui::TableSetupColumn("Most Active Weapon");
	ImGui::TableSetupColumn("Mod Active Basic");
	ImGui::TableSetupColumn("Mod Weapon");
	ImGui::TableHeadersRow();
	for(s32 i = 0; i < ARRAY_SIZE(save.hero_gadget_box->gadgets); i++) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%d", i);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S16, "##level", save.hero_gadget_box->gadgets[i].level);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S16, "##ammo", save.hero_gadget_box->gadgets[i].ammo);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U32, "##xp", save.hero_gadget_box->gadgets[i].xp);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##action_frame", save.hero_gadget_box->gadgets[i].action_frame);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##mod_active_post_fx", save.hero_gadget_box->gadgets[i].mod_active_post_fx);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##mod_active_weapon", save.hero_gadget_box->gadgets[i].mod_active_weapon);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar_n(ImGuiDataType_S32, "##mod_active_basic", save.hero_gadget_box->gadgets[i].mod_active_basic);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar_n(ImGuiDataType_S32, "##mod_weapon", save.hero_gadget_box->gadgets[i].mod_weapon);
		ImGui::PopID();
	}
	ImGui::EndTable();
}

static void gadget_events_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	ImGui::BeginTable("##gadget_entries", 9, ImGuiTableFlags_RowBg);
	ImGui::TableSetupColumn("Index");
	ImGui::TableSetupColumn("Gadget ID");
	ImGui::TableSetupColumn("Player Index");
	ImGui::TableSetupColumn("Gadget Type");
	ImGui::TableSetupColumn("Gadget Event Type");
	ImGui::TableSetupColumn("Active Time");
	ImGui::TableSetupColumn("Target UID");
	ImGui::TableSetupColumn("Target Offset Quat");
	ImGui::TableSetupColumn("Next Gadget Event Pointer");
	ImGui::TableHeadersRow();
	for(s32 i = 0; i < ARRAY_SIZE(save.hero_gadget_box->gadget_event_slots); i++) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%d", i);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##gadget_id", save.hero_gadget_box->gadget_event_slots[i].gadget_id);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##player_index", save.hero_gadget_box->gadget_event_slots[i].player_index);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##gadget_type", save.hero_gadget_box->gadget_event_slots[i].gadget_type);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##gadget_event_type", save.hero_gadget_box->gadget_event_slots[i].gadget_event_type);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##active_time", save.hero_gadget_box->gadget_event_slots[i].active_time);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U32, "##target_uid", save.hero_gadget_box->gadget_event_slots[i].target_uid);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar_n(ImGuiDataType_Float, "##target_offset_quat", save.hero_gadget_box->gadget_event_slots[i].target_offset_quat);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U32, "##p_next_gadget_event", save.hero_gadget_box->gadget_event_slots[i].p_next_gadget_event);
		ImGui::PopID();
	}
	ImGui::EndTable();
}

static void gadget_messages_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	ImGui::BeginTable("##gadget_messages", 9, ImGuiTableFlags_RowBg);
	ImGui::TableSetupColumn("Index");
	ImGui::TableSetupColumn("Gadget ID");
	ImGui::TableSetupColumn("Player Index");
	ImGui::TableSetupColumn("Gadget Event Type");
	ImGui::TableSetupColumn("Extra Data");
	ImGui::TableSetupColumn("Active Time");
	ImGui::TableSetupColumn("Target UID");
	ImGui::TableSetupColumn("Firing Location");
	ImGui::TableSetupColumn("Target Direction");
	ImGui::TableHeadersRow();
	for(s32 i = 0; i < ARRAY_SIZE(save.hero_gadget_box->gadget_event_slots); i++) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%d", i);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S16, "##gadget_id", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.gadget_id);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##player_index", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.player_index);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##gadget_event_type", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.gadget_event_type);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U8, "##extra_data", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.extra_data);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_S32, "##active_time", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.active_time);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U32, "##target_uid", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.target_uid);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar_n(ImGuiDataType_Float, "##firing_loc", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.firing_loc);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar_n(ImGuiDataType_Float, "##target_dir", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.target_dir);
		ImGui::PopID();
	}
	ImGui::EndTable();
}

static bool help_page(bool draw_gui) {
	if(!draw_gui) {
		return save.help_data_messages.has_value()
			|| save.help_data_misc.has_value()
			|| save.help_data_gadgets.has_value();
	}
	
	if(ImGui::BeginTabBar("##help_tabs")) {
		if(save.help_data_messages.has_value() && ImGui::BeginTabItem("Messages")) {
			help_subpage("##help_messages", *save.help_data_messages);
			ImGui::EndTabItem();
		}
		if(save.help_data_misc.has_value() && ImGui::BeginTabItem("Misc")) {
			help_subpage("##help_misc", *save.help_data_misc);
			ImGui::EndTabItem();
		}
		if(save.help_data_gadgets.has_value() && ImGui::BeginTabItem("Gadgets")) {
			help_subpage("##help_gadgets", *save.help_data_gadgets);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static void help_subpage(const char* label, std::vector<memory_card::HelpDatum>& help) {
	ImGui::BeginTable(label, 9, ImGuiTableFlags_RowBg);
	ImGui::TableSetupColumn("Index");
	ImGui::TableSetupColumn("Times Used");
	ImGui::TableSetupColumn("Counter");
	ImGui::TableSetupColumn("Last Time");
	ImGui::TableSetupColumn("Level Die");
	ImGui::TableHeadersRow();
	for(s32 i = 0; i < (s32) help.size(); i++) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%d", i);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U16, "##times_used", help[i].times_used);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U16, "##counter", help[i].counter);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U32, "##last_time", help[i].last_time);
		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-1);
		input_scalar(ImGuiDataType_U32, "##level_die", help[i].level_die);
		
		ImGui::PopID();
	}
	ImGui::EndTable();
}

static bool hero_page(bool draw_gui) {
	if(!save.hero_save.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::HeroSave& h = *save.hero_save;
	
	input_scalar(ImGuiDataType_S32, "Bolts", h.bolts);
	input_scalar(ImGuiDataType_S32, "Bolts Deficit", h.bolt_deficit);
	input_scalar(ImGuiDataType_S32, "XP", h.xp);
	input_scalar(ImGuiDataType_S32, "Points", h.points);
	input_scalar(ImGuiDataType_S16, "Hero Max HP", h.hero_max_hp);
	input_scalar(ImGuiDataType_S16, "Armor Level", h.armor_level);
	input_scalar(ImGuiDataType_Float, "Limit Break", h.limit_break);
	input_scalar(ImGuiDataType_S32, "Purchased Skins", h.purchased_skins);
	input_scalar(ImGuiDataType_S16, "Spent Diff Stars", h.spent_diff_stars);
	input_scalar(ImGuiDataType_U8, "Bolt Mult Level", h.bolt_mult_level);
	input_scalar(ImGuiDataType_U8, "Bolt Mult Sub Level", h.bolt_mult_sub_level);
	input_scalar(ImGuiDataType_U8, "Old Game Save Data", h.old_game_save_data);
	input_scalar(ImGuiDataType_U8, "Blue Badges", h.blue_badges);
	input_scalar(ImGuiDataType_U8, "Red Badges", h.red_badges);
	input_scalar(ImGuiDataType_U8, "Green Badges", h.green_badges);
	input_scalar(ImGuiDataType_U8, "Gold Badges", h.gold_badges);
	input_scalar(ImGuiDataType_U8, "Black Badges", h.black_badges);
	input_scalar(ImGuiDataType_U8, "Completes", h.completes);
	input_scalar_n(ImGuiDataType_U8, "Last Equipped Gadget", h.last_equipped_gadget);
	input_scalar_n(ImGuiDataType_U8, "Temp Weapons", h.temp_weapons);
	input_scalar(ImGuiDataType_S32, "Current Max Limit Break", h.current_max_limit_break);
	input_scalar(ImGuiDataType_S16, "Armor Level 2", h.armor_level_2);
	input_scalar(ImGuiDataType_S16, "Progression Armor Level", h.progression_armor_level);
	input_scalar(ImGuiDataType_S32, "Start Limit Break Diff", h.start_limit_break_diff);
	
	return true;
}

static bool settings_page(bool draw_gui) {
	if(!save.settings.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::GameSettings& s = *save.settings;
	
	input_scalar(ImGuiDataType_S32, "PAL Mode", s.pal_mode);
	input_scalar(ImGuiDataType_U8, "Help Voice On", s.help_voice_on);
	input_scalar(ImGuiDataType_U8, "Help Text On", s.help_text_on);
	input_scalar(ImGuiDataType_U8, "Subtitles Active", s.subtitles_active);
	input_scalar(ImGuiDataType_S32, "Stereo", s.stereo);
	input_scalar(ImGuiDataType_S32, "Music Volume", s.music_volume);
	input_scalar(ImGuiDataType_S32, "Effects Volume", s.effects_volume);
	input_scalar(ImGuiDataType_S32, "Voice Volume", s.voice_volume);
	input_scalar_multi(ImGuiDataType_S32, "Camera Elevation Dir", s.camera_elevation_dir, 3, 4);
	input_scalar_multi(ImGuiDataType_S32, "Camera Azimuth Dir", s.camera_azimuth_dir, 3, 4);
	input_scalar_multi(ImGuiDataType_S32, "Camera Rotate Speed", s.camera_rotate_speed, 3, 4);
	input_scalar_n(ImGuiDataType_U8, "First Person Mode On", s.first_person_mode_on);
	input_scalar(ImGuiDataType_U8, "Was NTSC Progessive", s.was_ntsc_progessive);
	input_scalar(ImGuiDataType_U8, "Wide", s.wide);
	input_scalar_n(ImGuiDataType_U8, "Controller Vibration On", s.controller_vibration_on);
	input_scalar(ImGuiDataType_U8, "Quick Select Pause On", s.quick_select_pause_on);
	input_scalar(ImGuiDataType_U8, "Language", s.language);
	input_scalar(ImGuiDataType_U8, "Aux Setting 2", s.aux_setting_2);
	input_scalar(ImGuiDataType_U8, "Aux Setting 3", s.aux_setting_3);
	input_scalar(ImGuiDataType_U8, "Aux Setting 4", s.aux_setting_4);
	input_scalar(ImGuiDataType_U8, "Auto Save On", s.auto_save_on);
	
	return true;
}

static bool statistics_page(bool draw_gui) {
	if(!save.player_statistics.has_value()) return false;
	if(!draw_gui) return true;
	
	if(ImGui::BeginTabBar("##player_statistics_tabs")) {
		for(s32 i = 0; i < 2; i++) {
			ImGui::PushID(i);
			std::string tab_name = stringf("Player %d", i + 1);
			if(ImGui::BeginTabItem(tab_name.c_str())) {
				memory_card::PlayerData& d = save.player_statistics->array[i];
				input_scalar(ImGuiDataType_U32, "Health Received", d.health_received);
				input_scalar(ImGuiDataType_U32, "Damage Received", d.damage_received);
				input_scalar(ImGuiDataType_U32, "Ammo Received", d.ammo_received);
				input_scalar(ImGuiDataType_U32, "Time Charge Booting", d.time_charge_booting);
				input_scalar(ImGuiDataType_U32, "Num Deaths", d.num_deaths);
				input_scalar_n(ImGuiDataType_U32, "Weapon Kills", d.weapon_kills);
				input_scalar_n(ImGuiDataType_Float, "Weapon Kill Percentage", d.weapon_kill_percentage);
				input_scalar_n(ImGuiDataType_U32, "Ammo Used", d.ammo_used);
				input_scalar_n(ImGuiDataType_U32, "Shots That Hit", d.shots_that_hit);
				input_scalar_n(ImGuiDataType_U32, "Shots That Miss", d.shots_that_miss);
				input_scalar_n(ImGuiDataType_Float, "Shot Accuracy", d.shot_accuracy);
				input_scalar_n(ImGuiDataType_U32, "Func Mod Kills", d.func_mod_kills);
				input_scalar_n(ImGuiDataType_U32, "Func Mod Used", d.func_mod_used);
				input_scalar_n(ImGuiDataType_U32, "Time Spent In Vehicles", d.time_spent_in_vehicles);
				input_scalar_n(ImGuiDataType_U32, "Kills With Vehicle Weaps", d.kills_with_vehicle_weaps);
				input_scalar_n(ImGuiDataType_U32, "Kills From Vehicle Squashing", d.kills_from_vehicle_squashing);
				input_scalar(ImGuiDataType_U32, "Kills While In Vehicle", d.kills_while_in_vehicle);
				input_scalar_n(ImGuiDataType_U32, "Vehicle Shots That Hit", d.vehicle_shots_that_hit);
				input_scalar_n(ImGuiDataType_U32, "Vehicle Shots That Miss", d.vehicle_shots_that_miss);
				input_scalar_n(ImGuiDataType_Float, "Vehicle Shot Accuracy", d.vehicle_shot_accuracy);
				ImGui::EndTabItem();
			}
			ImGui::PopID();
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static bool levels_page(bool draw_gui) {
	bool any_tabs = false;
	
	for(const memory_card::LevelSaveGame& level_save_game : save.levels) {
		if(level_save_game.level.has_value()) {
			any_tabs = true;
		}
	}
	if(!any_tabs) return false;
	if(!draw_gui) return true;
	
	ImGui::BeginTable("##levels", 3, ImGuiTableFlags_RowBg);
	ImGui::TableSetupColumn("Index");
	ImGui::TableSetupColumn("Status");
	ImGui::TableSetupColumn("Jackpot");
	ImGui::TableHeadersRow();
	for(s32 i = 0; i < (s32) save.levels.size(); i++) {
		if(save.levels[i].level.has_value()) {
			ImGui::PushID(i);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%d", i);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar(ImGuiDataType_U8, "##status", save.levels[i].level->status);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar(ImGuiDataType_U8, "##jackpot", save.levels[i].level->jackpot);
			ImGui::PopID();
		}
	}
	ImGui::EndTable();
	
	return true;
}
static bool missions_page(bool draw_gui) {
	bool any_tabs = false;
	for(const memory_card::LevelSaveGame& level_save_game : save.levels) {
		if(level_save_game.level.has_value()) {
			any_tabs = true;
		}
	}
	if(!any_tabs) return false;
	if(!draw_gui) return true;
	
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Level:");
	ImGui::SameLine();
	if(ImGui::BeginTabBar("##mission_tabs")) {
		for(s32 i = 0; i < (s32) save.levels.size(); i++) {
			memory_card::LevelSaveGame& level_save_game = save.levels[i];
			std::string tab_name = stringf("%d", i);
			if(level_save_game.level.has_value() && ImGui::BeginTabItem(tab_name.c_str())) {
				memory_card::LevelSave& level = *level_save_game.level;
				ImGui::BeginTable("##missions", 6, ImGuiTableFlags_RowBg);
				ImGui::TableSetupColumn("Index");
				ImGui::TableSetupColumn("XP");
				ImGui::TableSetupColumn("Bolts");
				ImGui::TableSetupColumn("Status");
				ImGui::TableSetupColumn("Completes");
				ImGui::TableSetupColumn("Difficulty");
				ImGui::TableHeadersRow();
				for(s32 j = 0; j < ARRAY_SIZE(level_save_game.level->mission); j++) {
					if(save.levels[i].level.has_value()) {
						ImGui::PushID(j);
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::AlignTextToFramePadding();
						ImGui::Text("%d", j);
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-1);
						input_scalar(ImGuiDataType_S32, "##xp", level.mission[j].xp);
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-1);
						input_scalar(ImGuiDataType_S32, "##bolts", level.mission[j].bolts);
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-1);
						input_scalar(ImGuiDataType_U8, "##status", level.mission[j].status);
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-1);
						input_scalar(ImGuiDataType_U8, "##completes", level.mission[j].completes);
						ImGui::TableNextColumn();
						ImGui::SetNextItemWidth(-1);
						input_scalar(ImGuiDataType_U8, "##difficulty", level.mission[j].difficulty);
						ImGui::PopID();
					}
				}
				ImGui::EndTable();
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static u8 from_bcd(u8 value) {
	return (value & 0xf) + ((value & 0xf0) >> 4) * 10;
}

static u8 to_bcd(u8 value) {
	return (value % 10) | (value / 10) << 4;
}
