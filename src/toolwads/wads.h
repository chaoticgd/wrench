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

packed_struct(BuildWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ s16 version_major;
	/* 0x0a */ s16 version_minor;
	/* 0x0c */ u8 commit[0x14];
	/* 0x20 */ SectorRange contributors;
)

packed_struct(LauncherWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ SectorRange font;
	/* 0x10 */ SectorRange placeholder_images[2];
	/* 0x20 */ SectorRange oobe;
)

packed_struct(ToolWadInfo,
	BuildWadHeader build;
	LauncherWadHeader launcher;
)

packed_struct(OobeWadHeader,
	/* 0x0 */ ByteRange greeting;
)

extern unsigned char WAD_INFO[];

#endif
