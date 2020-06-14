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

#include "command.h"
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

class wrench_project {
public:
	wrench_project(
		std::map<std::string, std::string>& game_paths,
		worker_logger& log,
		std::string game_id_); // New
	wrench_project(
		std::map<std::string, std::string>& game_paths,
		std::string project_path,
		worker_logger& log); // Open

	std::string project_path() const;
	std::string cached_iso_path() const;

	void save(app* a, std::function<void()> on_done);
	void save_as(app* a, std::function<void()> on_done);
	
	level* selected_level();
	std::size_t selected_level_index();
	std::vector<level*> levels();
	level* level_from_index(std::size_t index);
	std::map<std::string, std::vector<texture>*> texture_lists();
	std::map<std::string, std::vector<game_model>*> model_lists();
	
	template <typename T, typename... T_constructor_args>
	void emplace_command(T_constructor_args... args);
	void undo();
	void redo();
	
	void open_level(std::size_t index);
	
	int id();
	
	void save_to(std::string path);

private:
	void load_tables();

	std::string read_game_id();

	std::string _project_path;
	ZipArchive::Ptr _wrench_archive;

public: // Initialisation order matters.
	const std::string game_id; // e.g. "SCES_516.07"

private:
	std::size_t _history_index;
	std::vector<std::unique_ptr<command>> _history_stack;
	
	std::map<std::size_t, std::unique_ptr<racpak>> _archives;
	std::map<std::size_t, std::vector<texture>> _texture_wads;
	std::map<std::size_t, std::unique_ptr<level>> _levels;
	std::vector<armor_archive> _armor;
	level* _selected_level;
	
	int _id;
	static int _next_id;
	
public:
	iso_stream iso;
	table_of_contents toc;
};

template <typename T, typename... T_constructor_args>
void wrench_project::emplace_command(T_constructor_args... args) {
	_history_stack.resize(_history_index++);
	_history_stack.emplace_back(std::make_unique<T>(args...));
	auto& cmd = _history_stack[_history_index - 1];
	cmd->apply(this);
}

#endif
