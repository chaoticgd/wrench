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

#include "commands.h"

#include <core/filesystem.h>
#include <gui/config.h>

struct BinPaths {
	std::string wrenchbuild;
	std::string wrencheditor;
};

static BinPaths bin_paths;

void gui::setup_bin_paths(const char* bin_path) {
	fs::path directory = fs::path(bin_path).remove_filename();
	bin_paths.wrenchbuild = (directory/"wrenchbuild").string();
	bin_paths.wrencheditor = (directory/"wrencheditor").string();
}

void gui::run_unpacker(const UnpackerParams& params, CommandStatus* status) {
	std::vector<std::string> args;
	args.emplace_back(bin_paths.wrenchbuild);
	args.emplace_back("unpack");
	args.emplace_back(params.iso_path);
	args.emplace_back("-o");
	args.emplace_back(g_config.paths.games_folder);
	args.emplace_back("-s"); // Unpack it into a subdirectory.
	spawn_command_thread(args, status);
}

std::string gui::run_packer(const PackerParams& params, CommandStatus* status) {
	std::string output_path;
	
	std::vector<std::string> args;
	args.emplace_back(bin_paths.wrenchbuild);
	args.emplace_back("pack");
	args.emplace_back(params.game_path);
	for(const std::string& mod_path : params.mod_paths) {
		args.emplace_back(mod_path);
	}
	args.emplace_back("-a");
	args.emplace_back(params.build);
	args.emplace_back("-o");
	if(fs::path(params.output_path).is_relative()) {
		output_path = (fs::path(g_config.paths.builds_folder)/params.output_path).string();
	} else {
		output_path = params.output_path;
	}
	args.emplace_back(output_path);
	args.emplace_back("-h");
	if(params.debug.single_level_enabled || params.debug.nompegs) {
		std::string single_level = params.debug.single_level_enabled ? params.debug.single_level_tag : "";
		std::string flags;
		if(params.debug.nompegs) {
			flags += "nompegs";
		}
		args.emplace_back("testlf," + single_level + "," + flags);
	} else {
		args.emplace_back("release");
	}
	spawn_command_thread(args, status);
	return output_path;
}

void gui::run_emulator(const EmulatorParams& params, CommandStatus* status) {
	std::vector<std::string> args;
	args.emplace_back(g_config.paths.emulator_path);
	args.emplace_back(params.iso_path);
	spawn_command_thread(args, status);
}
