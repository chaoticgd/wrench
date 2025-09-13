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

#ifndef TOOLWADS_WADS_H
#define TOOLWADS_WADS_H

#include <core/util.h>

enum License
{
	LICENSE_WRENCH = 0,
	LICENSE_BARLOW = 1,
	LICENSE_CATCH2 = 2,
	LICENSE_GLAD = 3,
	LICENSE_GLFW = 4,
	LICENSE_GLM = 5,
	LICENSE_IMGUI = 6,
	LICENSE_LIBPNG = 7,
	LICENSE_NATIVEFILEDIALOG = 8,
	LICENSE_NLOHMANJSON = 9,
	LICENSE_RAPIDXML = 10,
	LICENSE_IMGUIZMO = 11,
	LICENSE_ZLIB = 12,
	LICENSE_LIBZIP = 13,
	LICENSE_IMGUI_CLUB = 14,
	LICENSE_PINE = 15,
	MAX_LICENSE = 16
};

packed_struct(BuildWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s32 version_major;
	/* 0x0c */ s32 version_minor;
	/* 0x10 */ s32 version_patch;
	/* 0x14 */ char version_string[20];
	/* 0x28 */ char commit_string[41];
	/* 0x51 */ char pad[3];
)

packed_struct(GuiWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange credits;
	/* 0x010 */ SectorRange license_text[32];
	/* 0x110 */ SectorRange fonts[10];
)

packed_struct(LauncherWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange logo;
	/* 0x10 */ SectorRange placeholder_images[2];
	/* 0x20 */ SectorRange oobe;
)

packed_struct(EditorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange tool_icons[32];
	/* 0x108 */ SectorRange instance_3d_view_icons[32];
)

packed_struct(MemcardWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorRange savegame;
	/* 0xc */ SectorRange types[4];
)

packed_struct(TrainerWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ SectorRange types[4];
)

packed_struct(ToolWadInfo,
	BuildWadHeader build;
	GuiWadHeader gui;
	LauncherWadHeader launcher;
	EditorWadHeader editor;
	MemcardWadHeader memcard;
)

packed_struct(OobeWadHeader,
	/* 0x0 */ ByteRange greeting;
)

extern "C" {
	extern ToolWadInfo wadinfo;
}

struct WadPaths
{
	std::string gui;
	std::string launcher;
	std::string editor;
	std::string underlay;
	std::string overlay;
	std::string memcard;
	std::string trainer;
};

WadPaths find_wads(const char* bin_path);
const char* get_versioned_application_name(const char* name);

#endif
