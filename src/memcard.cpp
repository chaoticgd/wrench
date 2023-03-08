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
static void common_page();
static void gadgets_page();
static void hero_page();
static void settings_page();
static void statistics_page();

struct Page {
	const char* name;
	void (*func)();
};

static const Page PAGES[] = {
	{"Common", &common_page},
	{"Gadgets", &gadgets_page},
	{"Hero", &hero_page},
	{"Settings", &settings_page},
	{"Statistics", &statistics_page}
};

static std::string directory;
static std::vector<fs::path> file_paths;
static size_t selected_file_index = 0;
static Opt<memory_card::File> file;
static std::string load_error_message;
static Opt<memory_card::SaveGame> save;

int main(int argc, char** argv) {
	WadPaths wads = find_wads(argv[0]);
	g_guiwad.open(wads.gui);
	
	GLFWwindow* window = gui::startup("Wrench Memory Card Editor", 960, 720);
	gui::load_font(wadinfo.gui.fonts[0], 22);
	while(!glfwWindowShouldClose(window)) {
		gui::run_frame(window, update_gui);
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
	if(gui::input_folder_path(&directory, "##directory", nullptr)) {
		file_paths.clear();
		try {
			for(auto entry : fs::directory_iterator(directory)) {
				if(entry.is_regular_file()) {
					file_paths.emplace_back(entry.path());
				}
			}
		} catch(std::filesystem::filesystem_error&) {}
		std::sort(BEGIN_END(file_paths));
	}
	
	ImGui::BeginChild("##files");
	for(size_t i = 0; i < file_paths.size(); i++) {
		if(ImGui::Selectable(file_paths[i].filename().string().c_str(), i == selected_file_index)) {
			try {
				std::vector<u8> buffer = read_file(file_paths[i]);
				file = memory_card::read_save(buffer);
				load_error_message.clear();
				save = memory_card::parse_save(*file);
			} catch(RuntimeError& error) {
				load_error_message = error.message;
			}
			selected_file_index = i;
		}
	}
	ImGui::EndChild();
}

static void sections() {
	
}

static void editor() {
	if(!load_error_message.empty()) {
		ImGui::Text("%s", load_error_message.c_str());
		return;
	}
	
	if(!file.has_value() || !save.has_value()) {
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
		for(const Page& page : PAGES) {
			if(ImGui::BeginTabItem(page.name)) {
				page.func();
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

// I'm using macros instead of functions so that the compiler doesn't complain
// about misaligned pointers (even if in practice they'll always be aligned).
#define input_scalar(data_type, label, value) \
	{ auto temp = value; ImGui::InputScalar(label, data_type, &value); value = temp; }
#define input_scalar_n(data_type, label, value, dim) \
	{ \
		decltype(value) temp; \
		memcpy(temp, value, sizeof(value)); \
		ImGui::InputScalarN(label, data_type, temp, dim, nullptr, nullptr, nullptr, ImGuiInputTextFlags_None); \
		memcpy(value, temp, sizeof(value)); \
	}

static void common_page() {
	ImGui::InputInt("Level", &(*save->level));
	
	u8 clock[sizeof(memory_card::Clock)];
	memcpy(clock, &(*save->last_save_time), sizeof(memory_card::Clock));
	ImGui::InputScalarN("Last Save Time", ImGuiDataType_U8, clock, sizeof(memory_card::Clock), nullptr, nullptr, nullptr, ImGuiInputTextFlags_None);
	memcpy(&(*save->last_save_time), clock, sizeof(memory_card::Clock));
}

static void gadgets_page() {
	input_scalar(ImGuiDataType_S32, "ptr", save->hero_gadget_box->p_next_gadget_event);
}

static void hero_page() {
	memory_card::HeroSave& h = *save->hero_save;
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
	input_scalar_n(ImGuiDataType_U8, "Last Equipped Gadget", h.last_equipped_gadget, 2);
	input_scalar_n(ImGuiDataType_U8, "Temp Weapons", h.temp_weapons, 4);
	input_scalar(ImGuiDataType_S32, "Current Max Limit Break", h.current_max_limit_break);
	input_scalar(ImGuiDataType_S16, "Armor Level 2", h.armor_level_2);
	input_scalar(ImGuiDataType_S16, "Progression Armor Level", h.progression_armor_level);
	input_scalar(ImGuiDataType_S32, "Start Limit Break Diff", h.start_limit_break_diff);
}

static void settings_page() {
	if(!save->settings.has_value()) {
		return;
	}
	
	memory_card::GameSettings& s = *save->settings;
	//ImGui::InputInt("PAL Mode", &s.pal_mode);
	//input_u8("help_voice_on", &s.help_voice_on);
	//input_u8("help_text_on", &s.help_text_on);
	//input_u8("subtitles_active", &s.subtitles_active);
	//ImGui::InputInt("stereo", &s.stereo);
	//ImGui::InputInt("music_volume", &s.music_volume);
	//ImGui::InputInt("effects_volume", &s.effects_volume);
	//ImGui::InputInt("voice_volume", &s.voice_volume);
	//input_u8("was_ntsc_progessive", &s.was_ntsc_progessive);
	//input_u8("wide", &s.wide);
	//input_u8("quick_select_pause_on", &s.quick_select_pause_on);
	//input_u8("language", &s.language);
	//input_u8("aux_setting_2", &s.aux_setting_2);
	//input_u8("aux_setting_3", &s.aux_setting_3);
	//input_u8("aux_setting_4", &s.aux_setting_4);
	//input_u8("auto_save_on", &s.auto_save_on);
}

static void statistics_page() {
	
}
