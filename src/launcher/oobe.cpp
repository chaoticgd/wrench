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

#include "oobe.h"

#include <gui/gui.h>
#include <engine/compression.h>
#include <launcher/global_state.h>

static void oobe(f32 delta_time);

static bool done = false;
static std::vector<u8> wad;
static OobeWadHeader* header;
static GlTexture welcome;

bool run_oobe() {
	ByteRange64 oobe_range = g_launcher.header->oobe.bytes();
	std::vector<u8> compressed = g_launcher.wad.read_multiple<u8>(oobe_range.offset, oobe_range.size);
	decompress_wad(wad, compressed);
	header = (OobeWadHeader*) &wad[0];
	
	GLFWwindow* window = gui::startup("Wrench Setup", 640, 480, true);
	
	u8* image = &wad[header->greeting.offset];
	glGenTextures(1, &welcome.id);
	glBindTexture(GL_TEXTURE_2D, welcome.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, *(s32*) &image[0], *(s32*) &image[4], 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[16]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	while(!glfwWindowShouldClose(window) && !done) {
		gui::run_frame(window, oobe);
	}
	
	gui::shutdown(window);
	
	welcome.destroy();
	
	return done;
}

static void oobe(f32 delta_time) {
	ImDrawList& background = *ImGui::GetBackgroundDrawList();
	background.AddRectFilledMultiColor(ImVec2(0, 0), ImGui::GetMainViewport()->Size,
		0xffff0000, 0xffff0000, 0xff000000, 0xff000000);
	background.AddImage((void*) (intptr_t) welcome.id, ImVec2(0, 0), ImVec2(512, 128));
	
	ImVec2 centre = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(centre, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);
	ImGui::Begin("Wrench Setup", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	
	static bool first_frame = true;
		if(first_frame) {
		const char* home = getenv("HOME");
		if(home) {
			g_config.folders.base_folder = std::string(home) + "/wrench";
			g_config.folders.mods_folders = {g_config.folders.base_folder + "/mods"};
			g_config.folders.games_folder = g_config.folders.base_folder + "/games";
			g_config.folders.cache_folder = g_config.folders.base_folder + "/cache";
		} else {
			g_config.folders.mods_folders = {""};
		}
		first_frame = false;
	}
	
	ImGui::TextWrapped("Welcome to the setup utility for the Wrench Modding Toolset!");
	
	ImGui::Separator();
	ImGui::TextWrapped("The following config file will be created:");
	std::string config_label = gui::get_config_file_path();
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##path", &config_label, ImGuiInputTextFlags_ReadOnly);
	
	ImGui::Separator();
	ImGui::TextWrapped("The following folders will be created if they do not already exist:");
	ImGui::InputText("Base Folder", &g_config.folders.base_folder);
	ImGui::InputText("Mods Folder", &g_config.folders.mods_folders[0]);
	ImGui::InputText("Games Folder", &g_config.folders.games_folder);
	ImGui::InputText("Cache Folder", &g_config.folders.cache_folder);
	
	ImGui::Separator();
	if(ImGui::Button("Confirm")) {
		fs::create_directories(g_config.folders.base_folder);
		fs::create_directories(g_config.folders.mods_folders[0]);
		fs::create_directories(g_config.folders.games_folder);
		fs::create_directories(g_config.folders.cache_folder);
		
		done = true;
	}
	
	ImGui::End();
}
