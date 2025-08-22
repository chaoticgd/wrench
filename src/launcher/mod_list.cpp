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

#include "mod_list.h"

#include <set>
#include <zip.h>
#include <algorithm>

#include <core/png.h>
#include <core/stream.h>
#include <core/filesystem.h>
#include <gui/config.h>
#include <launcher/global_state.h>

static void update_mod_images();
static void update_mod_builds();

static std::vector<Mod> mods;
static size_t selected_mod = SIZE_MAX;

Mod* mod_list(const std::string& filter)
{
	verify_fatal(g_mod_images.size() >= 1);
	
	std::string filter_lower = filter;
	for (char& c : filter_lower) c = tolower(c);
	
	ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable;
	
	static bool path_column = false;
	static bool author_column = false;
	
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	if (ImGui::BeginTable("mods", 2 + path_column + author_column, flags)) {
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		if (path_column) {
			ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
		}
		if (author_column) {
			ImGui::TableSetupColumn("Author", ImGuiTableColumnFlags_WidthStretch);
		}
		ImGui::TableHeadersRow();
		
		for (size_t i = 0; i < mods.size(); i++) {
			Mod& mod = mods[i];
			
			std::string mod_name_lowercase = mod.info.name;
			for (char& c : mod_name_lowercase) c = tolower(c);
			
			if (mod_name_lowercase.find(filter_lower) != std::string::npos) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				ImGui::PushID(i);
				
				ImGui::SetCursorPosX(ImGui::GetStyle().FramePadding.x);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
				ImGui::Checkbox("##enabled", &mod.enabled);
				ImGui::PopStyleVar();
				
				ImGui::TableNextColumn();
				ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SpanAllColumns;
				bool selected;
				if (!mod.info.name.empty()) {
					selected = ImGui::Selectable(mod.info.name.c_str(), i == selected_mod, selectable_flags);
				} else {
					ImGui::PushFont(g_launcher.font_italic);
					selected = ImGui::Selectable(mod.path.c_str(), i == selected_mod, selectable_flags);
					ImGui::PopFont();
				}
				if (selected) {
					selected_mod = i;
					update_mod_images();
					update_mod_builds();
				}
				
				if (path_column) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", mod.path.c_str());
				}
				
				if (author_column) {
					ImGui::TableNextColumn();
					ImGui::Text("%s", mod.info.author.c_str());
				}
				
				ImGui::PopID();
			}
		}
		
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	
	if (ImGui::BeginPopupContextItem("mods")) {
		ImGui::MenuItem("Path", nullptr, &path_column);
		ImGui::MenuItem("Author", nullptr, &author_column);
		ImGui::EndPopup();
	}
	
	if (selected_mod >= mods.size()) {
		return nullptr;
	}
	
	return &mods[selected_mod];
}

void load_mod_list(const std::vector<std::string>& mods_folders)
{
	free_mod_list();
	
	for (std::string mods_dir : mods_folders) {
		if (!fs::is_directory(mods_dir)) {
			continue;
		}
		
		for (fs::directory_entry entry : fs::directory_iterator(mods_dir)) {
			std::vector<u8> game_info_txt;
			
			FileInputStream stream;
			int error;
			if (stream.open(entry.path()/"gameinfo.txt")) {
				game_info_txt = stream.read_multiple<u8>(stream.size());
			} else if (entry.path().extension() == ".zip") {
				zip_t* zip = zip_open(entry.path().string().c_str(), ZIP_RDONLY, &error);
				if (zip) {
					s64 count = zip_get_num_entries(zip, 0);
					for (s64 i = 0; i < count; i++) {
						fs::path path = zip_get_name(zip, i, 0);
						if (path.filename() != "gameinfo.txt") {
							continue;
						}
						
						zip_stat_t stat;
						if (zip_stat_index(zip, i, 0, &stat) != 0 || (stat.valid & ZIP_STAT_SIZE) == 0) {
							continue;
						}
						
						game_info_txt.resize(stat.size);
						
						zip_file_t* file = zip_fopen_index(zip, i, 0);
						zip_fread(file, (u8*) game_info_txt.data(), stat.size);
						zip_fclose(file);
					}
					zip_close(zip);
				}
			}
			
			if (!game_info_txt.empty()) {
				strip_carriage_returns(game_info_txt);
				game_info_txt.push_back(0);
				
				Mod& mod = mods.emplace_back();
				mod.path = entry.path().string();
				mod.info = read_game_info((char*) game_info_txt.data());
			}
		}
	}
	
	std::sort(BEGIN_END(mods), [](const Mod& lhs, const Mod& rhs)
		{ return lhs.info.name < rhs.info.name; });
	
	update_mod_images();
	update_mod_builds();
}

static void update_mod_images()
{
	g_mod_images.clear();
	
	if (selected_mod < mods.size() && mods[selected_mod].info.images.size() >= 1) {
		Mod& mod = mods[selected_mod];
		s32 successful_loads = 0;
		for (const std::string& path : mod.info.images) {
			FileInputStream stream;
			if (stream.open(fs::path(mod.path)/path)) {
				Opt<Texture> image = read_png(stream);
				if (image.has_value()) {
					image->to_rgba();
					GlTexture texture;
					texture.upload(image->data.data(), image->width, image->height);
					g_mod_images.emplace_back(std::move(texture), image->width, image->height, path);
					successful_loads++;
				}
			}
		}
		if (successful_loads < 1) {
			GlTexture texture;
			texture.upload(g_launcher.logo.data.data(), g_launcher.logo.width, g_launcher.logo.height);
			g_mod_images.emplace_back(std::move(texture), g_launcher.logo.width, g_launcher.logo.height, "Logo");
		}
	} else {
		const Texture& logo = g_launcher.logo;
		GlTexture texture;
		texture.upload(logo.data.data(), logo.width, logo.height);
		g_mod_images.emplace_back(std::move(texture), logo.width, logo.height, "Logo");
	}
}

static void update_mod_builds()
{
	g_mod_builds.clear();
	std::set<std::string> builds;
	for (const Mod& mod : mods) {
		if (mod.enabled) {
			for (const std::string& build : mod.info.builds) {
				builds.emplace(build);
			}
		}
	}
	for (const std::string& build : builds) {
		g_mod_builds.emplace_back(build);
	}
}

void free_mod_list()
{
	mods.clear();
	selected_mod = SIZE_MAX;
	g_mod_images.clear();
	g_mod_builds.clear();
}

std::vector<std::string> enabled_mods()
{
	std::vector<std::string> result;
	for (const Mod& mod : mods) {
		if (mod.enabled) {
			result.emplace_back(mod.path);
		}
	}
	return result;
}

bool any_mods_enabled()
{
	for (const Mod& mod : mods) {
		if (mod.enabled) {
			return true;
		}
	}
	return false;
}

std::vector<ModImage> g_mod_images;
std::vector<std::string> g_mod_builds;
