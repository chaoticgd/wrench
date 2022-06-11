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

enum License {
	LICENSE_WRENCH = 0,
	LICENSE_BARLOW = 1,
	LICENSE_CXXOPTS = 2,
	LICENSE_GLAD = 3,
	LICENSE_GLFW = 4,
	LICENSE_GLM = 5,
	LICENSE_IMGUI = 6,
	LICENSE_LIBPNG = 7,
	LICENSE_NATIVEFILEDIALOG = 8,
	LICENSE_NLOHMANJSON = 9,
	LICENSE_RAPIDXML = 10,
	LICENSE_TOML11 = 11,
	LICENSE_ZLIB = 12,
	MAX_LICENSE = 13
};

packed_struct(BuildWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s16 version_major;
	/* 0x0a */ s16 version_minor;
	/* 0x0c */ u8 commit[0x14];
)

packed_struct(GuiWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ SectorRange contributors;
	/* 0x010 */ SectorRange license_text[32];
	/* 0x110 */ SectorRange fonts[10];
)

packed_struct(LauncherWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange unused;
	/* 0x10 */ SectorRange placeholder_images[2];
	/* 0x20 */ SectorRange oobe;
)

packed_struct(ToolWadInfo,
	BuildWadHeader build;
	GuiWadHeader gui;
	LauncherWadHeader launcher;
)

packed_struct(OobeWadHeader,
	/* 0x0 */ ByteRange greeting;
)

extern "C" {
	extern ToolWadInfo wadinfo;
}

#endif
