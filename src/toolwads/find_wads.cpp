#include "core/util/error_util.h"
#include "wads.h"

#include <stdlib.h>
#include <core/filesystem.h>

static std::string find_wad(const fs::path& directory, const char* file_name);
static std::string find_bank(const fs::path& directory, const char* file_name);

WadPaths find_wads(const char* bin_path)
{
	fs::path directory = fs::path(bin_path).remove_filename();
	WadPaths wads;
	wads.gui = find_wad(directory, "gui.wad");
	wads.launcher = find_wad(directory, "launcher.wad");
	wads.editor = find_wad(directory, "editor.wad");
	wads.memcard = find_wad(directory, "memcard.wad");
	wads.trainer = find_wad(directory, "trainer.wad");
	wads.underlay = find_bank(directory, "underlay");
	wads.overlay = find_bank(directory, "overlay");
	return wads;
}

static std::string find_wad(const fs::path& directory, const char* file_name)
{
	if (fs::exists(directory/file_name)) {
		return (directory/file_name).string();
	}
	if (fs::exists(directory/".."/file_name)) {
		return (directory/".."/file_name).string();
	}
	if (fs::exists(directory/".."/"share"/"wrench"/file_name)) {
		return (directory/".."/"share"/"wrench"/file_name).string();
	}
	verify_not_reached_fatal("Failed to load WAD.");
}

static std::string find_bank(const fs::path& directory, const char* file_name)
{
	if (fs::exists(directory/file_name)) {
		return (directory/file_name).string();
	}
	if (fs::exists(directory/".."/"data"/file_name)) {
		return (directory/".."/"data"/file_name).string();
	}
	if (fs::exists(directory/".."/".."/"data"/file_name)) {
		return (directory/".."/".."/"data"/file_name).string();
	}
	if (fs::exists(directory/".."/"share"/"wrench"/file_name)) {
		return (directory/".."/"share"/"wrench"/file_name).string();
	}
	verify_not_reached_fatal("Failed to load WAD.");
}

const char* get_versioned_application_name(const char* name)
{
	const char* application_version;
	if (strlen(wadinfo.build.version_string) > 0) {
		application_version = wadinfo.build.version_string;
	} else {
		application_version = wadinfo.build.commit_string;
	}
	static char versioned_name[256];
	snprintf(versioned_name, sizeof(versioned_name), "%s %s", name, application_version);
	return versioned_name;
}
