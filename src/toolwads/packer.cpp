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

#include <fstream>
#include <core/png.h>
#include <engine/compression.h>
#include <instancemgr/instance.h>
#include <toolwads/wads.h>

static void pack_build_wad();
static void pack_gui_wad();
static void pack_launcher_wad();
static SectorRange pack_oobe_wad(OutputStream& dest);
static void pack_editor_wad();
static void pack_memcard_wad();
static void pack_trainer_wad();
static SectorRange pack_ascii_icon(OutputStream& dest, const char* src_path);
static SectorRange pack_file(OutputStream& dest, const char* src_path);
static SectorRange pack_compressed_image(OutputStream& dest, const char* src_path);
static ByteRange pack_image(OutputStream& dest, const char* src_path);
static s32 parse_positive_embedded_int(const char* str);

std::string build_dir;

int main(int argc, char** argv)
{
	if (argc != 2) {
		if (argc != 0) {
			fprintf(stderr, "usage: %s <build dir>\n", argv[0]);
		}
		exit(1);
	}
	build_dir = argv[1];
	
	printf("Packing build wad...\n");
	fflush(stdout);
	pack_build_wad();
	
	printf("Packing gui wad...\n");
	fflush(stdout);
	pack_gui_wad();
	
	printf("Packing launcher wad...\n");
	fflush(stdout);
	pack_launcher_wad();
	
	printf("Packing editor wad...\n");
	fflush(stdout);
	pack_editor_wad();
	
	printf("Packing memcard wad...\n");
	fflush(stdout);
	pack_memcard_wad();
	
	printf("Packing trainer wad...\n");
	fflush(stdout);
	pack_trainer_wad();
	
	return 0;
}

extern const char* git_commit;
extern const char* git_tag;

static void pack_build_wad()
{
	FileOutputStream wad;
	verify_fatal(wad.open(build_dir + "/build.wad"));
	
	BuildWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<BuildWadHeader>();
	
	// Default values for if the tag isn't valid.
	header.version_major = -1;
	header.version_minor = -1;
	header.version_patch = -1;
	
	if (git_tag && git_tag[0] == 'v') {
		// Parse the version number from the git tag.
		const char* major_pos = git_tag + 1;
		header.version_major = parse_positive_embedded_int(major_pos);
		
		const char* minor_pos = major_pos;
		while (*minor_pos != '.' && *minor_pos != '\0') {
			minor_pos++;
		}
		if (*minor_pos != '\0') {
			minor_pos++;
			header.version_minor = parse_positive_embedded_int(minor_pos);
		}
		
		const char* patch_pos = minor_pos;
		while (*patch_pos != '.' && *patch_pos != '\0') {
			patch_pos++;
		}
		if (*patch_pos != '\0') {
			patch_pos++;
			header.version_patch = parse_positive_embedded_int(patch_pos);
		}
		
		// Store the version string.
		verify_fatal(strlen(git_tag) < 20);
		strncpy(header.version_string, git_tag, sizeof(header.version_string));
	}
	
	// Store the git commit.
	if (git_commit && strlen(git_commit) != 0) {
		verify_fatal(strlen(git_commit) == 40);
		strncpy(header.commit_string, git_commit, sizeof(header.commit_string));
	}
	
	wad.write<BuildWadHeader>(0, header);
}

static void pack_gui_wad()
{
	FileOutputStream wad;
	verify_fatal(wad.open(build_dir + "/gui.wad"));
	
	GuiWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<GuiWadHeader>();
	
	header.credits = pack_file(wad, "CREDITS");
	
	header.license_text[LICENSE_WRENCH] = pack_file(wad, "data/licenses/wrench.txt");
	header.license_text[LICENSE_BARLOW] = pack_file(wad, "data/licenses/barlowfont.txt");
	header.license_text[LICENSE_CATCH2] = pack_file(wad, "data/licenses/catch2.txt");
	header.license_text[LICENSE_GLAD] = pack_file(wad, "data/licenses/glad.txt");
	header.license_text[LICENSE_GLFW] = pack_file(wad, "data/licenses/glfw.txt");
	header.license_text[LICENSE_GLM] = pack_file(wad, "data/licenses/glm.txt");
	header.license_text[LICENSE_IMGUI] = pack_file(wad, "data/licenses/imgui.txt");
	header.license_text[LICENSE_LIBPNG] = pack_file(wad, "data/licenses/libpng.txt");
	header.license_text[LICENSE_NATIVEFILEDIALOG] = pack_file(wad, "data/licenses/nativefiledialog.txt");
	header.license_text[LICENSE_NLOHMANJSON] = pack_file(wad, "data/licenses/nlohmanjson.txt");
	header.license_text[LICENSE_RAPIDXML] = pack_file(wad, "data/licenses/rapidxml.txt");
	header.license_text[LICENSE_ZLIB] = pack_file(wad, "data/licenses/zlib.txt");
	header.license_text[LICENSE_LIBZIP] = pack_file(wad, "data/licenses/libzip.txt");
	header.license_text[LICENSE_IMGUIZMO] = pack_file(wad, "data/licenses/imguizmo.txt");
	header.license_text[LICENSE_PINE] = pack_file(wad, "data/licenses/pine.txt");
	
	header.fonts[0] = pack_file(wad, "data/gui/Barlow-Regular.ttf");
	header.fonts[1] = pack_file(wad, "data/gui/Barlow-Italic.ttf");
	
	wad.pad(SECTOR_SIZE, 0);
	wad.write<GuiWadHeader>(0, header);
}

static void pack_launcher_wad()
{
	FileOutputStream wad;
	verify_fatal(wad.open(build_dir + "/launcher.wad"));
	
	LauncherWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<LauncherWadHeader>();
	
	header.logo = pack_compressed_image(wad, "data/launcher/logo.png");
	header.oobe = pack_oobe_wad(wad);
	
	wad.pad(SECTOR_SIZE, 0);
	wad.write<LauncherWadHeader>(0, header);
}

static SectorRange pack_oobe_wad(OutputStream& dest)
{
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
	compress_wad(compressed_bytes, bytes, "", 1);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static void pack_editor_wad()
{
	FileOutputStream wad;
	verify_fatal(wad.open(build_dir + "/editor.wad"));
	
	EditorWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<EditorWadHeader>();
	
	header.tool_icons[0] = pack_ascii_icon(wad, "data/editor/icons/picker_tool.txt");
	header.tool_icons[1] = pack_ascii_icon(wad, "data/editor/icons/selection_tool.txt");
	header.tool_icons[2] = pack_ascii_icon(wad, "data/editor/icons/translate_tool.txt");
	header.tool_icons[3] = pack_ascii_icon(wad, "data/editor/icons/spline_tool.txt");
	
	header.instance_3d_view_icons[INST_MOBY] = pack_compressed_image(wad, "data/editor/icons/moby.png");
	header.instance_3d_view_icons[INST_MOBYGROUP] = pack_compressed_image(wad, "data/editor/icons/moby_group.png");
	header.instance_3d_view_icons[INST_TIE] = pack_compressed_image(wad, "data/editor/icons/tie.png");
	header.instance_3d_view_icons[INST_TIEGROUP] = pack_compressed_image(wad, "data/editor/icons/tie_group.png");
	header.instance_3d_view_icons[INST_SHRUB] = pack_compressed_image(wad, "data/editor/icons/shrub.png");
	header.instance_3d_view_icons[INST_SHRUBGROUP] = pack_compressed_image(wad, "data/editor/icons/shrub_group.png");
	header.instance_3d_view_icons[INST_POINTLIGHT] = pack_compressed_image(wad, "data/editor/icons/point_light.png");
	header.instance_3d_view_icons[INST_ENVSAMPLEPOINT] = pack_compressed_image(wad, "data/editor/icons/env_sample_point.png");
	header.instance_3d_view_icons[INST_ENVTRANSITION] = pack_compressed_image(wad, "data/editor/icons/env_transition.png");
	header.instance_3d_view_icons[INST_CUBOID] = pack_compressed_image(wad, "data/editor/icons/cuboid.png");
	header.instance_3d_view_icons[INST_SPHERE] = pack_compressed_image(wad, "data/editor/icons/sphere.png");
	header.instance_3d_view_icons[INST_CYLINDER] = pack_compressed_image(wad, "data/editor/icons/cylinder.png");
	header.instance_3d_view_icons[INST_PILL] = pack_compressed_image(wad, "data/editor/icons/pill.png");
	header.instance_3d_view_icons[INST_CAMERA] = pack_compressed_image(wad, "data/editor/icons/camera.png");
	header.instance_3d_view_icons[INST_SOUND] = pack_compressed_image(wad, "data/editor/icons/sound.png");
	header.instance_3d_view_icons[INST_AREA] = pack_compressed_image(wad, "data/editor/icons/area.png");
	
	wad.pad(SECTOR_SIZE, 0);
	wad.write<EditorWadHeader>(0, header);
}

static SectorRange pack_ascii_icon(OutputStream& dest, const char* src_path)
{
	dest.pad(SECTOR_SIZE, 0);
	s64 offset = dest.tell();
	
	std::ifstream image_file((std::string(src_path)));
	
	u8 buffer[32][16];
	for (s32 y = 0; y < 32; y++) {
		std::string line;
		std::getline(image_file, line);
		if (line.size() > 32) {
			line = line.substr(0, 32);
		}
		for (size_t x = 0; x < line.size(); x++) {
			u8 nibble = (line[x] == '#') ? 0xf : 0x0;
			if (x % 2 == 0) {
				buffer[y][x / 2] = nibble << 4;
			} else {
				buffer[y][x / 2] |= nibble;
			}
		}
		for (size_t x = line.size(); x < 32; x++) {
			if (x % 2 == 0) {
				buffer[y][x / 2] = 0;
			} else {
				buffer[y][x / 2] &= 0xf0;
			}
		}
	}
	dest.write_n((u8*) buffer, sizeof(buffer));
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static void pack_memcard_wad()
{
	FileOutputStream wad;
	verify_fatal(wad.open(build_dir + "/memcard.wad"));
	
	MemcardWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<MemcardWadHeader>();
	
	header.savegame = pack_file(wad, "data/memcard/savegame.wtf");
	header.types[0] = pack_file(wad, "data/memcard/types_rac.h");
	header.types[1] = pack_file(wad, "data/memcard/types_gc.h");
	header.types[2] = pack_file(wad, "data/memcard/types_uya.h");
	header.types[3] = pack_file(wad, "data/memcard/types_dl.h");
	
	wad.pad(SECTOR_SIZE, 0);
	wad.write<MemcardWadHeader>(0, header);
}

static void pack_trainer_wad()
{
	FileOutputStream wad;
	verify_fatal(wad.open(build_dir + "/trainer.wad"));
	
	TrainerWadHeader header = {};
	header.header_size = sizeof(header);
	wad.alloc<TrainerWadHeader>();
	
	header.types[0] = pack_file(wad, "data/trainer/trainer_types_rac.h");
	header.types[1] = pack_file(wad, "data/trainer/trainer_types_gc.h");
	header.types[2] = pack_file(wad, "data/trainer/trainer_types_uya.h");
	header.types[3] = pack_file(wad, "data/trainer/trainer_types_dl.h");
	
	wad.pad(SECTOR_SIZE, 0);
	wad.write<TrainerWadHeader>(0, header);
}

// *****************************************************************************

static SectorRange pack_file(OutputStream& dest, const char* src_path)
{
	dest.pad(SECTOR_SIZE, 0);
	s64 offset = dest.tell();
	
	FileInputStream src;
	verify_fatal(src.open(src_path));
	
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, "", 1);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static SectorRange pack_compressed_image(OutputStream& dest, const char* src_path)
{
	dest.pad(SECTOR_SIZE, 0);
	s32 offset = dest.tell();
	
	std::vector<u8> bytes;
	MemoryOutputStream stream(bytes);
	
	pack_image(stream, src_path);
	
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, "", 1);
	
	dest.write_v(compressed_bytes);
	
	SectorRange range;
	range.offset = Sector32::size_from_bytes(offset);
	range.size = Sector32::size_from_bytes(dest.tell() - offset);
	return range;
}

static ByteRange pack_image(OutputStream& dest, const char* src_path)
{
	s32 offset = (s32) dest.tell();
	
	FileInputStream src;
	verify_fatal(src.open(src_path));
	
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


static s32 parse_positive_embedded_int(const char* str)
{
	char copy[16] = {};
	s32 size = strlen(str);
	for (s32 i = 0; i < std::min(16, size); i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			copy[i] = str[i];
		} else if (i == 0) {
			return -1;
		} else {
			break;
		}
	}
	return atoi(copy);
}
