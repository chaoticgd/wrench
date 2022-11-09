#include "core/util/error_util.h"
#include "wads.h"

#include <stdlib.h>
#include <core/filesystem.h>

static std::string find_wad(const fs::path& directory, const char* file_name);

WadPaths find_wads(const char* bin_path) {
	fs::path directory = fs::path(bin_path).remove_filename();
	WadPaths wads;
	wads.gui = find_wad(directory, "gui.wad");
	wads.launcher = find_wad(directory, "launcher.wad");
	wads.editor = find_wad(directory, "editor.wad");
	wads.underlays = find_wad(directory, "underlays.zip");
	return wads;
}

static std::string find_wad(const fs::path& directory, const char* file_name) {
	if(fs::exists(directory/file_name)) {
		return (directory/file_name).string();
	}
	if(fs::exists(directory/".."/"share"/"wrench"/file_name)) {
		return (directory/".."/"share"/"wrench"/file_name).string();
	}
	verify_not_reached("Failed to find '%s'.", file_name);
}
