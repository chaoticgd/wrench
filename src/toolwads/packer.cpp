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
#include <toolwads/wads.h>

static void pack_build_wad();
static void pack_launcher_wad();
static SectorRange pack_oobe_wad(OutputStream& dest);
static SectorRange pack_file(OutputStream& dest, const char* src_path);
static SectorRange pack_compressed_image(OutputStream& dest, const char* src_path);
static ByteRange pack_image(OutputStream& dest, const char* src_path);
static s32 parse_positive_embedded_int(const char* str);

int main() {
	pack_build_wad();
	pack_launcher_wad();
	return 0;
}

static void pack_build_wad() {
	FileOutputStream wad;
	assert(wad.open("data/build.wad"));
	
	BuildWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<BuildWadHeader>();
	
	// Default values for if the tag isn't valid.
	header.version_major = -1;
	header.version_minor = -1;
	
	std::vector<u8> tag_str = read_file("data/git_tag.tmp");
	tag_str.push_back(0);
	const char* tag = (const char*) tag_str.data();
	
	// Parse the version number from the git tag.
	if(tag[0] == 'v') {
		const char* major_pos = tag + 1;
		header.version_major = parse_positive_embedded_int(major_pos);
		
		const char* minor_pos = major_pos;
		while(*minor_pos != '.' && *minor_pos != '\0') {
			minor_pos++;
		}
		if(*minor_pos != '\0') {
			minor_pos++;
			header.version_minor = parse_positive_embedded_int(minor_pos);
		}
	}
	
	std::vector<u8> commit_str = read_file("data/git_commit.tmp");
	commit_str.push_back(0);
	const char* commit = (const char*) commit_str.data();
	
	// Parse the git commit hash.
	for(size_t i = 0; i < std::min(strlen(commit), sizeof(header.commit) * 2); i++) {
		u8 nibble = (u8) commit[i];
		if(nibble >= '0' && nibble <= '9') {
			nibble -= '0';
		} else if(nibble >= 'a' && nibble <= 'f') {
			nibble += 0xa - 'a';
		} else {
			break;
		}
		if(i % 2 == 0) {
			header.commit[i / 2] = nibble << 4;
		} else {
			header.commit[i / 2] |= nibble;
		}
	}
	
	header.contributors = pack_file(wad, "CONTRIBUTORS");
	
	wad.write<BuildWadHeader>(0, header);
}

static void pack_launcher_wad() {
	FileOutputStream wad;
	assert(wad.open("data/launcher.wad"));
	
	LauncherWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<LauncherWadHeader>();
	
	header.font = pack_file(wad, "data/Barlow-Regular.ttf");
	header.placeholder_images[0] = pack_compressed_image(wad, "data/launcher/my_mod.png");
	header.oobe = pack_oobe_wad(wad);
	
	wad.write<LauncherWadHeader>(0, header);
}

static SectorRange pack_oobe_wad(OutputStream& dest) {
	dest.pad(SECTOR_SIZE, 0);
	s64 offset = dest.tell();
	
	std::vector<u8> bytes;
	MemoryOutputStream stream(bytes);
	
	OobeWadHeader header;
	stream.alloc<OobeWadHeader>();
	
	stream.pad(0x10, 0);
	header.greeting = pack_image(stream, "data/launcher/oobe/wrench_setup.png");
	
	stream.write(0, header);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, "", 8);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static SectorRange pack_file(OutputStream& dest, const char* src_path) {
	dest.pad(SECTOR_SIZE, 0);
	s64 offset = dest.tell();
	
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

static SectorRange pack_compressed_image(OutputStream& dest, const char* src_path) {
	dest.pad(SECTOR_SIZE, 0);
	s32 offset = dest.tell();
	
	std::vector<u8> bytes;
	MemoryOutputStream stream(bytes);
	
	pack_image(stream, src_path);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, "", 8);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static ByteRange pack_image(OutputStream& dest, const char* src_path) {
	s32 offset = (s32) dest.tell();
	
	FileInputStream src;
	assert(src.open(src_path));
	
	Opt<Texture> image = read_png(src);
	image->to_rgba();
	
	std::vector<u8> bytes;
	OutBuffer buffer(bytes);
	dest.write<s32>(image->width);
	dest.write<s32>(image->height);
	dest.pad(0x10, 0);
	dest.write_v(image->data);
	
	return {offset, (s32) (dest.tell() - offset)};
}


static s32 parse_positive_embedded_int(const char* str) {
	char copy[16] = {};
	s32 size = strlen(str);
	for(s32 i = 0; i < std::min(16, size); i++) {
		if(str[i] >= '0' && str[i] <= '9') {
			copy[i] = str[i];
		} else if(i == 0) {
			return -1;
		} else {
			break;
		}
	}
	return atoi(copy);
}
