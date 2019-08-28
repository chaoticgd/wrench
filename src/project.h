/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef PROJECT_H
#define PROJECT_H

#include <ZipLib/ZipArchive.h>

#include "iso_stream.h"
#include "worker_logger.h"
#include "formats/racpak.h"
#include "formats/level_impl.h"
#include "formats/texture_impl.h"

# /*
#	A project is a mod that patches the game's ISO file. Additional metadata
#	such as the game ID is also stored. It is serialised to disk as a zip file.
# */

class app;

struct iso_views {
	iso_views(stream* iso_file, worker_logger& log);

	std::vector<std::unique_ptr<racpak>> racpaks;
	std::map<int, std::unique_ptr<level_impl>> levels;
	fip_scanner space_wad;
	fip_scanner armor_wad;
	racpak_fip_scanner hud_wad;
};

class wrench_project {
public:
	wrench_project(std::string iso_path, worker_logger& log, std::string game_id); // New
	wrench_project(std::string iso_path, std::string project_path, worker_logger& log); // Open

	std::string cached_iso_path() const;

	void save(app* a);
	void save_as(app* a);

private:
	void save_to(std::string path);

	std::string read_game_id();

	std::string _project_path;
	ZipArchive::Ptr _wratch_archive;
	const std::string _game_id;
	iso_stream _iso;

public:
	iso_views views;
};

#endif
