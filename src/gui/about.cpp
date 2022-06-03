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
static void about_contributors();
static void about_libraries();

static const gui::Page ABOUT_PAGES[] = {
	{"Wrench", about_wrench},
	{"Contributors", about_contributors},
	{"Libraries", about_libraries}
};


static const gui::Chapter ABOUT_SCREEN[] = {
	{"About", ARRAY_PAIR(ABOUT_PAGES)}
};

static std::vector<u8> contributors_text;

void gui::about_screen() {
	static const Page* page = nullptr;
	gui::book(&page, "About##the_popup", ARRAY_PAIR(ABOUT_SCREEN), BookButtons::CLOSE);
}

static void about_wrench() {
	ImGui::TextWrapped("Wrench is a set of modding tools for the Ratchet & Clank PS2 games.");
}

static void about_contributors() {
	if(contributors_text.empty()) {
		SectorRange range = ((ToolWadInfo*) WAD_INFO)->build.contributors;
		
		FileInputStream wad;
		wad.open("data/build.wad");
		std::vector<u8> compressed_bytes = wad.read_multiple<u8>(range.offset.bytes(), range.size.bytes());
		
		std::vector<u8> string;
		decompress_wad(contributors_text, compressed_bytes);
		contributors_text.push_back(0);
	}
	
	ImGui::TextWrapped("%s", (const char*) contributors_text.data());
}

static void about_libraries() {
	const char* libraries =
		"cxxopts: https://github.com/jarro2783/cxxopts (MIT)\n"
		"glad: https://github.com/Dav1dde/glad (MIT)\n"
		"glfw: https://www.glfw.org/ (zlib)\n"
		"glm: https://github.com/g-truc/glm (Happy Bunny/MIT)\n"
		"imgui: https://github.com/ocornut/imgui (MIT)\n"
		"libpng: http://www.libpng.org/pub/png/libpng.html (libpng)\n"
		"nativefiledialog: https://github.com/mlabbe/nativefiledialog (zlib)\n"
		"nlohmann json: https://github.com/nlohmann/json (MIT)\n"
		"rapidxml: http://rapidxml.sourceforge.net/ (Boost)\n"
		"toml11: https://github.com/ToruNiina/toml11 (MIT)\n"
		"MD5 implementation by Colin Plumb\n"
		"zlib: https://zlib.net/ (zlib)\n";
	ImGui::TextWrapped("%s", libraries);
}
