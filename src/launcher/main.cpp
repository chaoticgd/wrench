/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include <core/png.h>
#include <engine/compression.h>
#include <gui/gui.h>
#include <launcher/global_state.h>

static void update_gui(f32 delta_time);
static void update_mod_list_window();
static void update_information_window();
static void update_buttons_window(f32 buttons_window_height);
static void create_dock_layout();
void begin_docking(f32 buttons_window_height);

static ImFont* load_font(SectorRange range);
static GlTexture load_image(SectorRange range);

int main(int argc, char** argv) {
	g_launcher.mode = LauncherMode::DRAWING_GUI;
	verify(g_launcher.wad.open("data/launcher.wad"), "Failed to open 'launcher.wad'.");
	g_launcher.header = &((ToolWadInfo*) WAD_INFO)->launcher;
	
	for(;;) {
		switch(g_launcher.mode) {
			case LauncherMode::DRAWING_GUI: {
				g_launcher.window = gui::startup("Wrench Launcher", 960, 600);
				
				load_font(g_launcher.header->font);
				g_launcher.placeholder_image = load_image(g_launcher.header->placeholder_images[0]);
				
				while(g_launcher.mode == LauncherMode::DRAWING_GUI) {
					gui::run_frame(g_launcher.window, update_gui);
					
					if(glfwWindowShouldClose(g_launcher.window)) {
						g_launcher.mode = LauncherMode::EXIT;
					}
				}
				
				gui::shutdown(g_launcher.window);
				break;
			}
			case LauncherMode::RUNNING_EMULATOR: {
				// TODO: Fix command injection.
				system(g_launcher.emulator_command.c_str());
				g_launcher.mode = LauncherMode::DRAWING_GUI;
				break;
			}
			case LauncherMode::EXIT: {
				return 0;
			}
		}
	}
}

void update_gui(f32 delta_time) {
	f32 button_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
	f32 buttons_window_height = button_height + GImGui->Style.WindowPadding.y * 2.f;
	
	begin_docking(buttons_window_height);
	
	static bool first_frame = true;
	if(first_frame) {
		create_dock_layout();
		first_frame = false;
	}
	
	update_mod_list_window();
	update_information_window();
	
	ImGui::End(); // docking
	
	update_buttons_window(buttons_window_height);
}

static void update_mod_list_window() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Mod List");
	
	const char* greeting = "Wrench Modding Toolset v0.0";
	ImGui::NewLine();
	f32 greeting_width = ImGui::CalcTextSize(greeting).x;
	ImGui::SetCursorPosX((ImGui::GetWindowSize().x - greeting_width) / 2);
	ImGui::Text("%s", greeting);
	ImGui::NewLine();
	
	ImGui::AlignTextToFramePadding();
	ImGui::Text("Game");
	ImGui::SameLine();
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
	if(ImGui::BeginCombo("##game", "Ratchet: Gladiator (EU, sces_532.58)")) {
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
	
	ImGui::BeginChild("table");
	if(ImGui::BeginTable("mods", 4)) {
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		
		bool check = false;
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Checkbox("##enabled", &check);
		ImGui::TableNextColumn();
		ImGui::Text("my mod");
		ImGui::TableNextColumn();
		ImGui::Text("45k");
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Checkbox("##enabled", &check);
		ImGui::TableNextColumn();
		ImGui::Text("more bolts mod");
		ImGui::TableNextColumn();
		ImGui::Text("14k");
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Checkbox("##enabled", &check);
		ImGui::TableNextColumn();
		ImGui::Text("nude clank mod");
		ImGui::TableNextColumn();
		ImGui::Text("1k");
		
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Checkbox("##enabled", &check);
		ImGui::TableNextColumn();
		ImGui::Text("less bolts mod");
		ImGui::TableNextColumn();
		ImGui::Text("10k");
		
		ImGui::EndTable();
	}
	ImGui::EndChild();
	
	ImGui::End();
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
}

static void update_information_window() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Information");
	
	ImGui::Image((void*) (intptr_t) g_launcher.placeholder_image.id, ImVec2(512, 320));
	
	ImGui::TextWrapped("This is a mod that enhances the gaming experience by introducing rich new ideas to create a new paradigm of synergistic gameplay.");
	
	ImGui::End();
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
}

static void update_buttons_window(f32 buttons_window_height) {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
	
	ImVec2 viewport_size = ImGui::GetMainViewport()->Size;
	ImGui::SetNextWindowPos(ImVec2(-1, viewport_size.y - buttons_window_height));
	ImGui::SetNextWindowSize(ImVec2(viewport_size.x + 2, buttons_window_height + 1));
	
	bool p_open;
	ImGui::Begin("Buttons", &p_open, flags);
	
	if(ImGui::Button("Import ISO")) {
		
	}
	
	ImGui::SameLine();
	if(ImGui::Button("Import Mod")) {
		
	}
	
	ImGui::SameLine();
	if(ImGui::Button("New Mod")) {
		
	}
	
	ImGui::SameLine();
	if(ImGui::Button("Open in Editor")) {
		
	}
	
	ImGui::SameLine();
	f32 build_run_text_width = ImGui::CalcTextSize("Build & Run").x;
	ImGuiStyle& s = ImGui::GetStyle();
	f32 build_run_button_width = s.FramePadding.x + build_run_text_width + s.FramePadding.x;
	f32 build_area_width = 192 + s.ItemSpacing.x + build_run_button_width + s.WindowPadding.x;
	ImGui::SetCursorPosX(viewport_size.x - build_area_width);
	
	ImGui::PushItemWidth(192);
	if(ImGui::BeginCombo("##builds", "dl")) {
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
	
	ImGui::SameLine();
	if(ImGui::Button("Build & Run")) {
		
	}
	
	ImGui::End();
}

static void create_dock_layout() {
	ImGuiID dockspace_id = ImGui::GetID("dock_space");
	
	ImGui::DockBuilderRemoveNode(dockspace_id);
	ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspace_id, ImVec2(1.f, 1.f));

	ImGuiID mod_list, information;
	ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 1.f / 2.f, &mod_list, &information);
	
	ImGui::DockBuilderDockWindow("Mod List", mod_list);
	ImGui::DockBuilderDockWindow("Information", information);

	ImGui::DockBuilderFinish(dockspace_id);
}

void begin_docking(f32 buttons_window_height) {
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 size = viewport->Size;
	size.y -= buttons_window_height;
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowViewport(viewport->ID);
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
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_NoTabBar);
}

static ImFont* load_font(SectorRange range) {
	std::vector<u8> compressed_font = g_launcher.wad.read_multiple<u8>(range.offset.bytes(), range.size.bytes());
	
	decompress_wad(g_launcher.font, compressed_font);
	
	ImFontConfig font_cfg;
	font_cfg.FontDataOwnedByAtlas = false;
	
	ImGuiIO& io = ImGui::GetIO();
	return io.Fonts->AddFontFromMemoryTTF(g_launcher.font.data(), g_launcher.font.size(), 22, &font_cfg);
}

static GlTexture load_image(SectorRange range) {
	std::vector<u8> compressed_image = g_launcher.wad.read_multiple<u8>(range.offset.bytes(), range.size.bytes());
	
	std::vector<u8> image;
	decompress_wad(image, compressed_image);
	
	GlTexture texture;
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, *(s32*) &image[0], *(s32*) &image[4], 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[16]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	return texture;
}
