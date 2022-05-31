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

#ifndef TOOLWADS_PACKER_H
#define TOOLWADS_PACKER_H

#include <core/png.h>
#include <engine/compression.h>
#include <toolwads/wads.h>

static void pack_launcher_wad();
static SectorRange pack_file(FileOutputStream& dest, const char* src_path);
static SectorRange pack_image(FileOutputStream& dest, const char* src_path);

int main() {
	pack_launcher_wad();
	return 0;
}

static void pack_launcher_wad() {
	FileOutputStream launcher_wad;
	assert(launcher_wad.open("data/launcher.wad"));
	
	LauncherWadHeader header = {0};
	header.header_size = sizeof(header);
	launcher_wad.alloc<LauncherWadHeader>();
	
	header.font = pack_file(launcher_wad, "data/Barlow-Regular.ttf");
	header.placeholder_images[0] = pack_image(launcher_wad, "data/my_mod.png");
	
	launcher_wad.write<LauncherWadHeader>(0, header);
}

static SectorRange pack_file(FileOutputStream& dest, const char* src_path) {
	dest.pad(SECTOR_SIZE, 0);
	s32 offset = dest.tell();
	
	FileInputStream src;
	assert(src.open(src_path));
	
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, "", 8);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static SectorRange pack_image(FileOutputStream& dest, const char* src_path) {
	dest.pad(SECTOR_SIZE, 0);
	s32 offset = dest.tell();
	
	FileInputStream src;
	assert(src.open(src_path));
	
	Opt<Texture> image = read_png(src);
	image->to_rgba();
	
	std::vector<u8> bytes;
	OutBuffer buffer(bytes);
	buffer.write<s32>(image->width);
	buffer.write<s32>(image->height);
	buffer.pad(0x10);
	buffer.write_multiple(image->data);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, "", 8);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

#endif
