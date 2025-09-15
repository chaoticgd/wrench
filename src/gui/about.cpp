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

#include "about.h"

#include <core/stream.h>
#include <engine/compression.h>
#include <toolwads/wads.h>
#include <gui/book.h>

static void about_wrench();
static void about_credits();
static void about_libraries();

static const gui::Page ABOUT_PAGES[] = {
	{"Wrench", about_wrench},
	{"Credits", about_credits},
	{"Libraries", about_libraries}
};

typedef void (*VoidFuncPtr)();

static License loaded_license = MAX_LICENSE;
static std::vector<u8> license_text;

template <License license>
static constexpr VoidFuncPtr gen_license_page()
{
	return []() {
		if (license != loaded_license) {
			license_text.clear();
			SectorRange range = wadinfo.gui.license_text[(s32) license];
			std::vector<u8> compressed = g_guiwad.read_multiple<u8>(range.offset.bytes(), range.size.bytes());
			decompress_wad(license_text, compressed);
			license_text.push_back(0);
			loaded_license = license;
		}
		
		ImGui::SetNextItemWidth(-1);
		ImVec2 size(0, ImGui::GetWindowHeight() - 128);
		ImGui::InputTextMultiline("license", (char*) license_text.data(), license_text.size(), size,
			ImGuiInputTextFlags_Multiline | ImGuiInputTextFlags_ReadOnly);
	};
}

static const gui::Page LICENSE_PAGES[] = {
	{"Wrench", gen_license_page<LICENSE_WRENCH>()},
	{"Catch2", gen_license_page<LICENSE_CATCH2>()},
	{"Barlow", gen_license_page<LICENSE_BARLOW>()},
	{"Dear ImGui", gen_license_page<LICENSE_IMGUI>()},
	{"Dear ImGui Club", gen_license_page<LICENSE_IMGUI_CLUB>()},
	{"GLAD", gen_license_page<LICENSE_GLAD>()},
	{"GLFW", gen_license_page<LICENSE_GLFW>()},
	{"GLM", gen_license_page<LICENSE_GLM>()},
	{"ImGuizmo", gen_license_page<LICENSE_IMGUIZMO>()},
	{"libpng", gen_license_page<LICENSE_LIBPNG>()},
	{"libzip", gen_license_page<LICENSE_LIBZIP>()},
	{"nativefiledialog", gen_license_page<LICENSE_NATIVEFILEDIALOG>()},
	{"nlohmanjson", gen_license_page<LICENSE_NLOHMANJSON>()},
	{"PINE", gen_license_page<LICENSE_PINE>()},
	{"RapidXML", gen_license_page<LICENSE_RAPIDXML>()},
	{"zlib", gen_license_page<LICENSE_ZLIB>()}
};

static const gui::Chapter ABOUT_SCREEN[] = {
	{"About", ARRAY_PAIR(ABOUT_PAGES)},
	{"Licenses", ARRAY_PAIR(LICENSE_PAGES)}
};

static std::vector<u8> credits_text;

void gui::about_screen()
{
	static const Page* page = nullptr;
	gui::book(&page, "About##the_popup", ARRAY_PAIR(ABOUT_SCREEN), BookButtons::CLOSE);
}

static void about_wrench()
{
	ImGui::TextWrapped("Wrench is a set of modding tools for the Ratchet & Clank PS2 games.");
	ImGui::NewLine();
	// These numbers are extracted from the git tag at build time. See 'src/toolwads/'.
	if (strlen(wadinfo.build.version_string) != 0) {
		ImGui::TextWrapped("Release Version %s", wadinfo.build.version_string);
	} else {
		ImGui::TextWrapped("Development Version");
	}
	ImGui::NewLine();
	char* c = wadinfo.build.commit_string;
	ImGui::AlignTextToFramePadding();
	ImGui::TextWrapped("Built from git commit %c%c%c%c%c%c%c%c",
		c[0],c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
	ImGui::SameLine();
	if (ImGui::Button("Copy Full Hash")) {
		ImGui::SetClipboardText(c);
	}
}

static void about_credits()
{
	if (credits_text.empty()) {
		SectorRange range = wadinfo.gui.credits;
		std::vector<u8> compressed_bytes = g_guiwad.read_multiple<u8>(range.offset.bytes(), range.size.bytes());
		
		std::vector<u8> string;
		decompress_wad(credits_text, compressed_bytes);
		credits_text.push_back(0);
	}
	
	ImGui::TextWrapped("%s", (const char*) credits_text.data());
}

static void about_libraries()
{
	const char* libraries =
		"Catch2: https://github.com/catchorg/Catch2\n"
		"dear imgui: https://github.com/ocornut/imgui\n"
		"glad: https://github.com/Dav1dde/glad\n"
		"glfw: https://www.glfw.org/\n"
		"glm: https://github.com/g-truc/glm\n"
		"ImGuizmo: https://github.com/CedricGuillemet/ImGuizmo\n"
		"libpng: http://www.libpng.org/pub/png/libpng.html\n"
		"libzip: https://libzip.org/\n"
		"nativefiledialog: https://github.com/mlabbe/nativefiledialog\n"
		"nlohmann json: https://github.com/nlohmann/json\n"
		"rapidxml: http://rapidxml.sourceforge.net/\n"
		"toml11: https://github.com/ToruNiina/toml11\n"
		"MD5 implementation by Colin Plumb\n"
		"zlib: https://zlib.net/\n";
	ImGui::TextWrapped("%s", libraries);
}
