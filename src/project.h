/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include "game_db.h"
#include "iso_stream.h"
#include "worker_logger.h"
#include "formats/toc.h"
#include "formats/racpak.h"
#include "formats/texture.h"
#include "formats/game_model.h"
#include "formats/level_impl.h"
#include "formats/armor_archive.h"

# /*
#	A project is a mod that patches the game's ISO file. Additional metadata
#	such as the game ID is also stored. It is serialised to disk as a zip file.
# */

class app;

struct game_iso {
	std::string path;
	std::string game_db_entry; // e.g. "R&C2" corresponding to the entry in the gamedb.txt file.
	std::string md5;
};

struct model_list {
	std::vector<moby_model>* models;
	std::vector<texture>* textures;
};

struct undo_redo_command {
	std::function<void()> apply;
	std::function<void()> undo;
};

class wrench_project {
public:
	wrench_project(
		game_iso game_,
		worker_logger& log); // New
	wrench_project(
		std::vector<game_iso> games,
		std::string project_path,
		worker_logger& log); // Open

	void post_load(); // Called from main thread, used for OpenGL things.

	std::string project_path() const;
	void set_project_path(std::string project_path);
	std::string cached_iso_path() const;
	
	level* selected_level();
	std::size_t selected_level_index();
	std::vector<level*> levels();
	level* level_from_index(std::size_t index);
	std::map<std::string, std::vector<texture>*> texture_lists(app* a);
	std::map<std::string, model_list> model_lists(app* a);
	
	void push_command(std::function<void()> apply, std::function<void()> undo);
	void undo();
	void redo();
	void clear_undo_history();
	
	void open_level(std::size_t index);
	
	int id();
	
	void save();

	armor_archive& armor() { return _armor.begin()->second; }

private:
	void load_tables();
	void load_gamedb_info(app* a);

	game_iso read_game_type(std::vector<game_iso> games);

	std::string table_index_to_name(std::size_t table_index);
	std::string level_index_to_name(std::size_t level_index);

	std::string _project_path;
	ZipArchive::Ptr _wrench_archive;

public: // Initialisation order matters.
	const game_iso game;

private:
	std::size_t _history_index;
	std::vector<undo_redo_command> _history_stack;
	
	std::map<std::size_t, std::unique_ptr<racpak>> _archives;
	std::map<std::size_t, std::vector<texture>> _texture_wads;
	std::map<std::size_t, std::unique_ptr<level>> _levels;
	std::map<std::size_t, armor_archive> _armor;
	level* _selected_level;
	
	std::optional<gamedb_game> _game_info;
	
	int _id;
	static int _next_id;
	
public:
	iso_stream iso;
	table_of_contents toc;
};

class command_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

// Store this in an undo/redo command to access a level object. This is required
// to check if the level has been unloaded or reallocated.
struct level_proxy {
	level_proxy(wrench_project* p) : proj(p), index(p->selected_level_index()) {}
	wrench_project* proj;
	std::size_t index;
	level& get() const {
		level* lvl = proj->level_from_index(index);
		if(lvl == nullptr) {
			throw command_error("The level this undo/redo operation applies to is not currently loaded.");
		}
		return *lvl;
	}
};

#endif
