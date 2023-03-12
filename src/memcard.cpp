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
static bool profiles_page(bool draw_gui);
static bool profile_stats_page(bool draw_gui);
static bool game_modes_page(bool draw_gui);
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
static bool sections_page(bool draw_gui);
static void sections_subpage(const std::vector<memory_card::Section>& sections);
static u8 from_bcd(u8 value);
static u8 to_bcd(u8 value);

struct Page {
	const char* name;
	bool (*func)(bool draw_gui);
	bool visible = false;
};

static Page PAGES[] = {
	// net
	{"Profiles", &profiles_page},
	{"Profile Statistics", &profile_stats_page},
	{"Game Modes", &game_modes_page},
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
	{"Missions", &missions_page},
	// sections
	{"Sections", &sections_page}
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
static bool raw_mode = false;

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
	} else {
		ImGui::Text("%s", listing_error.c_str());
	}
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
	
	ImGui::SameLine();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
	ImGui::Checkbox("Raw Mode", &raw_mode);
	ImGui::PopStyleVar();
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
	assert_not_reached("Bad field type.");
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

static bool profiles_page(bool draw_gui) {
	if(!save.mp_profiles.has_value()) return false;
	if(!draw_gui) return true;
	
	if(ImGui::BeginTabBar("##profiles")) {
		for(s32 i = 0; i < ARRAY_SIZE(save.mp_profiles->array); i++) {
			memory_card::ProfileStruct& p = save.mp_profiles->array[i];
			if(ImGui::BeginTabItem(std::to_string(i).c_str())) {
				input_scalar("Skin", p.skin);
				input_scalar("Camera 0 Normal Left/Right Mode", p.camera_options[0].normal_left_right_mode, CHECKBOX);
				input_scalar("Camera 0 Normal Up/Down Mode", p.camera_options[0].normal_up_down_mode, CHECKBOX);
				input_scalar("Camera 0 Speed", p.camera_options[0].camera_speed);
				input_scalar("Camera 1 Normal Left/Right Mode", p.camera_options[1].normal_left_right_mode, CHECKBOX);
				input_scalar("Camera 1 Normal Up/Down Mode", p.camera_options[1].normal_up_down_mode, CHECKBOX);
				input_scalar("Camera 1 Speed", p.camera_options[1].camera_speed);
				input_scalar("Camera 2 Normal Left/Right Mode", p.camera_options[2].normal_left_right_mode, CHECKBOX);
				input_scalar("Camera 2 Normal Up/Down Mode", p.camera_options[2].normal_up_down_mode, CHECKBOX);
				input_scalar("Camera 2 Speed", p.camera_options[2].camera_speed);
				input_scalar("First Person Mode On", p.first_person_mode_on, CHECKBOX);
				input_text("Name", p.name);
				input_text("Password", p.password);
				input_scalar("Map Access", p.map_access);
				input_scalar("PAL Server", p.pal_server);
				input_scalar("Help Msg off", p.help_msg_off);
				input_scalar("Save Password", p.save_password);
				input_scalar("Location index", p.location_idx);
				input_scalar("Active", p.active);
				input_array("Help Data", p.help_data);
				input_scalar("Net Enabled", p.net_enabled, CHECKBOX);
				input_scalar("Vibration", p.vibration, CHECKBOX);
				input_scalar("Music Volume", p.music_volume);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static bool profile_stats_page(bool draw_gui) {
	if(!save.mp_profiles.has_value()) return false;
	if(!draw_gui) return true;
	
	if(ImGui::BeginTabBar("##profile_stats")) {
		for(s32 i = 0; i < ARRAY_SIZE(save.mp_profiles->array); i++) {
			memory_card::ProfileStruct& p = save.mp_profiles->array[i];
			if(ImGui::BeginTabItem(stringf("%d", i).c_str())) {
				memory_card::GeneralStatStruct& g = p.general_stats;
				input_scalar("Games Played", g.no_of_games_played);
				input_scalar("Games Won", g.no_of_games_won);
				input_scalar("Games Lost", g.no_of_games_lost);
				input_scalar("Kills", g.no_of_kills);
				input_scalar("Deaths", g.no_of_deaths);
				
				memory_card::SiegeMatchStatStruct& s = p.siege_match_stats;
				input_scalar("Siege Match Games Won", s.no_of_wins);
				input_scalar("Siege Match Games Lost", s.no_of_losses);
				input_array("Siege Match Wins Per Level", s.wins_per_level);
				input_array("Siege Match Losses Per Level", s.losses_per_level);
				input_scalar("Siege Match Base Captures", s.no_of_base_captures);
				input_scalar("Siege Match Kills", g.no_of_kills);
				input_scalar("Siege Match Deaths", g.no_of_deaths);
				
				memory_card::DeadMatchStatStruct& d = p.dead_match_stats;
				input_scalar("Death Match Wins", d.no_of_wins);
				input_scalar("Death Match Losses", d.no_of_losses);
				input_array("Death Match Wins Per Level", d.wins_per_level);
				input_array("Death Match Losses Per Level", d.losses_per_level);
				input_scalar("Siege Match Kills", d.no_of_kills);
				input_scalar("Siege Match Deaths", d.no_of_deaths);
				
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static bool game_modes_page(bool draw_gui) {
	if(!save.game_mode_options.has_value()) return false;
	if(!draw_gui) return true;
	
	memory_card::GameModeStruct& o = *save.game_mode_options;
	input_scalar("Mode Chosen", o.mode_chosen);
	
	input_scalar("Siege Match Time Limit", o.siege_options.time_limit);
	input_scalar("Siege Match Nodes", o.siege_options.nodes_on, CHECKBOX);
	input_scalar("Siege Match Aids", o.siege_options.ais_on, CHECKBOX);
	input_scalar("Siege Match Vehicles", o.siege_options.vehicles_on, CHECKBOX);
	input_scalar("Siege Match Friendly Fire", o.siege_options.friendlyfire_on, CHECKBOX);
	
	input_scalar("Time Death Match Time Limit", o.time_death_match_options.time_limit);
	input_scalar("Time Death Match Vehicles", o.time_death_match_options.vehicles_on, CHECKBOX);
	input_scalar("Time Death Match Friendly Fire", o.time_death_match_options.friendly_fire_on, CHECKBOX);
	input_scalar("Time Death Match Suicide", o.time_death_match_options.suicide_on, CHECKBOX);
	
	input_scalar("Frag Death Match Frag Limit", o.frag_death_match_options.frag_limit);
	input_scalar("Frag Death Match Vehicles", o.frag_death_match_options.vechicles_on, CHECKBOX);
	input_scalar("Frag Death Match Suicide", o.frag_death_match_options.suicide_on, CHECKBOX);
	input_scalar("Frag Death Match Friendly Fire", o.frag_death_match_options.friendly_fire_on, CHECKBOX);
	
	return true;
}

static bool slot_page(bool draw_gui) {
	if(save.type != memory_card::FileType::SLOT) return false;
	if(!draw_gui) return true;
	
	if(save.level.has_value()) input_scalar("Level", *save.level);
	if(save.elapsed_time.has_value()) input_scalar("Elapsed Time", *save.elapsed_time);
	if(save.last_save_time.has_value()) input_clock("Last Save Time (smhdmy)", *save.last_save_time)
	if(save.global_flags.has_value()) input_array("Global Flags", save.global_flags->array);
	if(save.global_flags.has_value()) input_array("Global Flags", save.global_flags->array);
	if(save.cheats_activated.has_value()) input_array("Cheats Activated", save.cheats_activated->array);
	if(save.skill_points.has_value()) input_array("Skill Points", save.skill_points->array);
	if(save.cheats_ever_activated.has_value()) input_array("Cheats Ever Activated", save.cheats_ever_activated->array);
	if(save.movies_played_record.has_value()) input_array("Movies Played Record", save.movies_played_record->array);
	if(save.total_play_time.has_value()) input_scalar("Total Play Time", *save.total_play_time);
	if(save.total_deaths.has_value()) input_scalar("Total Deaths", *save.total_deaths);
	if(save.purchaseable_gadgets.has_value()) input_array("Purchaseable Gadgets", save.purchaseable_gadgets->array);
	if(save.purchaseable_bot_upgrades.has_value()) input_array("Purchaseable Bot Upgrades", save.purchaseable_bot_upgrades->array);
	if(save.purchaseable_wrench_level.has_value()) input_scalar("Purchaseable Wrench Level", *save.purchaseable_wrench_level);
	if(save.purchaseable_post_fx_mods.has_value()) input_array("Purchaseable Post FX Mods", save.purchaseable_post_fx_mods->array);
	if(save.first_person_desired_mode.has_value()) input_array("First Person Desired Mode", save.first_person_desired_mode->array);
	if(save.saved_difficulty_level.has_value()) input_scalar("Saved Difficulty Level", *save.saved_difficulty_level);
	if(save.battledome_wins_and_losses.has_value()) input_array("Battledome Wins and Losses", save.battledome_wins_and_losses->array);
	if(save.quick_switch_gadgets.has_value()) input_array_2d("Quick Select Gadgets", save.quick_switch_gadgets->array);
	
	return true;
}

static bool bots_page(bool draw_gui) {
	if(!save.bot_save.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::BotSave& s = *save.bot_save;
	
	input_array("Bot Upgrades", s.bot_upgrades);
	input_array("Bot Paintjobs", s.bot_upgrades);
	input_array("Bot Heads", s.bot_upgrades);
	input_array("Current Bot Paint Job", s.bot_upgrades);
	input_array("Current Bot Head", s.bot_upgrades);
	
	return true;
}

static bool enemy_kills_page(bool draw_gui) {
	if(!save.enemy_kills.has_value()) return false;
	if(!draw_gui) return true;
	
	if(ImGui::BeginTable("##enemy_kills", 3, ImGuiTableFlags_RowBg)) {
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
			input_scalar("##o_class", save.enemy_kills->array[i].o_class);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##kills", save.enemy_kills->array[i].kills);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	
	return true;
}

static bool gadget_page(bool draw_gui) {
	if(!save.hero_gadget_box.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	if(ImGui::BeginTabBar("##gadget_tabs")) {
		if(ImGui::BeginTabItem("General")) {
			ImGui::BeginChild("##general");
			gadget_general_subpage();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Entries")) {
			ImGui::BeginChild("##entries");
			gadget_entries_subpage();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Events")) {
			ImGui::BeginChild("##events");
			gadget_events_subpage();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Event Messages")) {
			ImGui::BeginChild("##messages");
			gadget_messages_subpage();
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static void gadget_general_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	input_scalar("Initialized", g.initialized, CHECKBOX);
	input_scalar("Level", g.level);
	input_array("Button Down", g.button_down);
	input_array("Button Up Frames", g.button_up_frames);
	input_scalar("Num Gadget Events", g.num_gadget_events);
	input_array("Mod Basic", g.mod_basic);
	input_scalar("Mod Post FX", g.mod_post_fx);
	input_scalar("Gadget Event Pointer", g.p_next_gadget_event);
}

static void gadget_entries_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	if(ImGui::BeginTable("##gadget_entries", 9, ImGuiTableFlags_RowBg)) {
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
			input_scalar("##level", save.hero_gadget_box->gadgets[i].level);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##ammo", save.hero_gadget_box->gadgets[i].ammo);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##xp", save.hero_gadget_box->gadgets[i].xp);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##action_frame", save.hero_gadget_box->gadgets[i].action_frame);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##mod_active_post_fx", save.hero_gadget_box->gadgets[i].mod_active_post_fx);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##mod_active_weapon", save.hero_gadget_box->gadgets[i].mod_active_weapon);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_array("##mod_active_basic", save.hero_gadget_box->gadgets[i].mod_active_basic);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_array("##mod_weapon", save.hero_gadget_box->gadgets[i].mod_weapon);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

static void gadget_events_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	if(ImGui::BeginTable("##gadget_entries", 9, ImGuiTableFlags_RowBg)) {
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
			input_scalar("##gadget_id", save.hero_gadget_box->gadget_event_slots[i].gadget_id);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##player_index", save.hero_gadget_box->gadget_event_slots[i].player_index);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##gadget_type", save.hero_gadget_box->gadget_event_slots[i].gadget_type);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##gadget_event_type", save.hero_gadget_box->gadget_event_slots[i].gadget_event_type);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##active_time", save.hero_gadget_box->gadget_event_slots[i].active_time);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##target_uid", save.hero_gadget_box->gadget_event_slots[i].target_uid);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_array("##target_offset_quat", save.hero_gadget_box->gadget_event_slots[i].target_offset_quat);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##p_next_gadget_event", save.hero_gadget_box->gadget_event_slots[i].p_next_gadget_event);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

static void gadget_messages_subpage() {
	memory_card::GadgetBox& g = *save.hero_gadget_box;
	
	if(ImGui::BeginTable("##gadget_messages", 9, ImGuiTableFlags_RowBg)) {
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
			input_scalar("##gadget_id", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.gadget_id);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##player_index", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.player_index);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##gadget_event_type", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.gadget_event_type);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##extra_data", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.extra_data);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##active_time", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.active_time);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##target_uid", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.target_uid);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_array("##firing_loc", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.firing_loc);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_array("##target_dir", save.hero_gadget_box->gadget_event_slots[i].gadget_event_msg.target_dir);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
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
	if(ImGui::BeginTable(label, 9, ImGuiTableFlags_RowBg)) {
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
			input_scalar("##times_used", help[i].times_used);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##counter", help[i].counter);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##last_time", help[i].last_time);
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1);
			input_scalar("##level_die", help[i].level_die);
			
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

static bool hero_page(bool draw_gui) {
	if(!save.hero_save.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::HeroSave& h = *save.hero_save;
	
	input_scalar("Bolts", h.bolts);
	input_scalar("Bolts Deficit", h.bolt_deficit);
	input_scalar("XP", h.xp);
	input_scalar("Points", h.points);
	input_scalar("Hero Max HP", h.hero_max_hp);
	input_scalar("Armor Level", h.armor_level);
	input_scalar("Limit Break", h.limit_break);
	input_scalar("Purchased Skins", h.purchased_skins);
	input_scalar("Spent Diff Stars", h.spent_diff_stars);
	input_scalar("Bolt Mult Level", h.bolt_mult_level);
	input_scalar("Bolt Mult Sub Level", h.bolt_mult_sub_level);
	input_scalar("Old Game Save Data", h.old_game_save_data);
	input_scalar("Blue Badges", h.blue_badges);
	input_scalar("Red Badges", h.red_badges);
	input_scalar("Green Badges", h.green_badges);
	input_scalar("Gold Badges", h.gold_badges);
	input_scalar("Black Badges", h.black_badges);
	input_scalar("Completes", h.completes);
	input_array("Last Equipped Gadget", h.last_equipped_gadget);
	input_array("Temp Weapons", h.temp_weapons);
	input_scalar("Current Max Limit Break", h.current_max_limit_break);
	input_scalar("Armor Level 2", h.armor_level_2);
	input_scalar("Progression Armor Level", h.progression_armor_level);
	input_scalar("Start Limit Break Diff", h.start_limit_break_diff);
	
	return true;
}

static bool settings_page(bool draw_gui) {
	if(!save.settings.has_value()) return false;
	if(!draw_gui) return true;
	memory_card::GameSettings& s = *save.settings;
	
	input_scalar("PAL Mode", s.pal_mode, CHECKBOX);
	input_scalar("Help Voice On", s.help_voice_on, CHECKBOX);
	input_scalar("Help Text On", s.help_text_on, CHECKBOX);
	input_scalar("Subtitles Active", s.subtitles_active, CHECKBOX);
	input_scalar("Stereo", s.stereo, CHECKBOX);
	input_scalar("Music Volume", s.music_volume);
	input_scalar("Effects Volume", s.effects_volume);
	input_scalar("Voice Volume", s.voice_volume);
	input_array_2d("Camera Elevation Dir", s.camera_elevation_dir);
	input_array_2d("Camera Azimuth Dir", s.camera_azimuth_dir);
	input_array_2d("Camera Rotate Speed", s.camera_rotate_speed);
	input_array("First Person Mode", s.first_person_mode_on);
	input_scalar("Was NTSC Progessive", s.was_ntsc_progessive);
	input_scalar("Wide", s.wide);
	input_array("Controller Vibration", s.controller_vibration_on);
	input_scalar("Quick Select Pause", s.quick_select_pause_on, CHECKBOX);
	input_scalar("Language", s.language);
	input_scalar("Aux Setting 2", s.aux_setting_2);
	input_scalar("Aux Setting 3", s.aux_setting_3);
	input_scalar("Aux Setting 4", s.aux_setting_4);
	input_scalar("Auto Save", s.auto_save_on, CHECKBOX);
	
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
				ImGui::BeginChild("##level");
				memory_card::PlayerData& d = save.player_statistics->array[i];
				input_scalar("Health Received", d.health_received);
				input_scalar("Damage Received", d.damage_received);
				input_scalar("Ammo Received", d.ammo_received);
				input_scalar("Time Charge Booting", d.time_charge_booting);
				input_scalar("Num Deaths", d.num_deaths);
				input_array("Weapon Kills", d.weapon_kills);
				input_array("Weapon Kill Percentage", d.weapon_kill_percentage);
				input_array("Ammo Used", d.ammo_used);
				input_array("Shots That Hit", d.shots_that_hit);
				input_array("Shots That Miss", d.shots_that_miss);
				input_array("Shot Accuracy", d.shot_accuracy);
				input_array("Func Mod Kills", d.func_mod_kills);
				input_array("Func Mod Used", d.func_mod_used);
				input_array("Time Spent In Vehicles", d.time_spent_in_vehicles);
				input_array("Kills With Vehicle Weaps", d.kills_with_vehicle_weaps);
				input_array("Kills From Vehicle Squashing", d.kills_from_vehicle_squashing);
				input_scalar("Kills While In Vehicle", d.kills_while_in_vehicle);
				input_array("Vehicle Shots That Hit", d.vehicle_shots_that_hit);
				input_array("Vehicle Shots That Miss", d.vehicle_shots_that_miss);
				input_array("Vehicle Shot Accuracy", d.vehicle_shot_accuracy);
				ImGui::EndChild();
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
	
	if(ImGui::BeginTable("##levels", 3, ImGuiTableFlags_RowBg)) {
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
				input_scalar("##status", save.levels[i].level->status);
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-1);
				input_scalar("##jackpot", save.levels[i].level->jackpot);
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
	
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
	
	if(ImGui::BeginTabBar("##mission_tabs")) {
		for(s32 i = 0; i < (s32) save.levels.size(); i++) {
			memory_card::LevelSaveGame& level_save_game = save.levels[i];
			if(level_save_game.level.has_value() && ImGui::BeginTabItem(std::to_string(i).c_str())) {
				ImGui::BeginChild("##level");
				memory_card::LevelSave& level = *level_save_game.level;
				if(ImGui::BeginTable("##missions", 6, ImGuiTableFlags_RowBg)) {
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
							input_scalar("##xp", level.mission[j].xp);
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1);
							input_scalar("##bolts", level.mission[j].bolts);
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1);
							input_scalar("##status", level.mission[j].status);
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1);
							input_scalar("##completes", level.mission[j].completes);
							ImGui::TableNextColumn();
							ImGui::SetNextItemWidth(-1);
							input_scalar("##difficulty", level.mission[j].difficulty);
							ImGui::PopID();
						}
					}
					ImGui::EndTable();
				}
				ImGui::EndChild();
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}
	
	return true;
}

static bool sections_page(bool draw_gui) {
	if(file->type == memory_card::FileType::MAIN) return false;
	if(file->type == memory_card::FileType::PATCH) return false;
	if(file->type == memory_card::FileType::SYS) return false;
	if(!draw_gui) return true;
	
	switch(file->type) {
		case memory_card::FileType::MAIN: {
			break;
		}
		case memory_card::FileType::NET: {
			sections_subpage(file->net.sections);
			break;
		}
		case memory_card::FileType::PATCH: {
			break;
		}
		case memory_card::FileType::SLOT: {
			if(ImGui::BeginTabBar("##section_tables")) {
				if(ImGui::BeginTabItem("Game")) {
					ImGui::BeginChild("##sections");
					sections_subpage(file->slot.sections);
					ImGui::EndChild();
					ImGui::EndTabItem();
				}
				for(s32 i = 0; i < (s32) file->slot.levels.size(); i++) {
					const std::vector<memory_card::Section>& sections = file->slot.levels[i];
					ImGui::PushID(i);
					if(ImGui::BeginTabItem(stringf("Lvl %d", i).c_str())) {
						ImGui::BeginChild("##sections");
						sections_subpage(sections);
						ImGui::EndChild();
						ImGui::EndTabItem();
					}
					ImGui::PopID();
				}
				ImGui::EndTabBar();
			}
			break;
		}
		case memory_card::FileType::SYS: {
			break;
		}
	}
	
	return true;
}

static void sections_subpage(const std::vector<memory_card::Section>& sections) {
	if(ImGui::BeginTable("##sections_table", 5, ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Index");
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("Data Offset (Bytes)");
		ImGui::TableSetupColumn("Data Size (Bytes)");
		ImGui::TableSetupColumn("Padded Data Size (Bytes)");
		ImGui::TableHeadersRow();
		for(s32 i = 0; i < (s32) sections.size(); i++) {
			const memory_card::Section& section = sections[i];
			ImGui::PushID(i);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%d", i);
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%04d (%s)", section.type, memory_card::section_type(section.type));
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("0x%x / %d", section.offset, section.offset);
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("0x%x / %d", section.unpadded_size, section.unpadded_size);
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("0x%x / %d", (s32) section.data.size(), (s32) section.data.size());
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

static u8 from_bcd(u8 value) {
	return (value & 0xf) + ((value & 0xf0) >> 4) * 10;
}

static u8 to_bcd(u8 value) {
	return (value % 10) | (value / 10) << 4;
}
