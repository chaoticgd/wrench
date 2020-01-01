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

#include "command.h"
#include "iso_stream.h"
#include "worker_logger.h"
#include "formats/racpak.h"
#include "formats/game_model.h"
#include "formats/level_impl.h"
#include "formats/texture_impl.h"
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
		std::string game_id); // New
	wrench_project(
		std::map<std::string, std::string>& game_paths,
		std::string project_path,
		worker_logger& log); // Open

	std::string project_path() const;
	std::string cached_iso_path() const;

	void save(app* a, std::function<void()> on_done);
	void save_as(app* a, std::function<void()> on_done);
	
	level* selected_level();
	std::vector<level*> levels();
	level* level_at(std::size_t offset);
	std::vector<texture_provider*> texture_providers();
	std::map<std::string, std::vector<game_model>*> model_lists();
	
	template <typename T, typename... T_constructor_args>
	void emplace_command(T_constructor_args... args);
	void undo();
	void redo();
	
	// Views are objects that access the game's data structures and expose an
	// abstracted interface to view and modify them. Available views are a list
	// of all supported views, not only those that are currently loaded.
	std::vector<std::string> available_view_types();
	std::vector<std::string> available_views(std::string group);
	void select_view(std::string group, std::string view);
	
	racpak* open_archive(std::size_t offset, std::size_t size);
	void open_texture_archive(std::size_t offset, std::size_t size);
	void open_level(std::size_t offset, std::size_t size);
	
	int id();
	
private:
	void save_to(std::string path);

	std::string read_game_id();

	std::string _project_path;
	ZipArchive::Ptr _wrench_archive;
	const std::string _game_id;
	
	std::size_t _history_index;
	std::vector<std::unique_ptr<command>> _history_stack;
	
	using view_group = std::map<std::string, std::function<void(wrench_project*)>>;
	using game_view = std::map<std::string, view_group>; // Views for a given game.
	static const std::map<std::string, game_view> _views;
	std::string _next_view_name;
	
	std::map<std::size_t, std::unique_ptr<racpak>> _archives;
	std::map<std::size_t, std::unique_ptr<texture_provider>> _texture_wads;
	std::map<std::size_t, std::unique_ptr<level>> _levels;
	std::optional<armor_archive> _armor;
	level* _selected_level;
	
	int _id;
	static int _next_id;
	
public:
	iso_stream iso;
};

template <typename T, typename... T_constructor_args>
void wrench_project::emplace_command(T_constructor_args... args) {
	_history_stack.resize(_history_index++);
	_history_stack.emplace_back(std::make_unique<T>(args...));
	auto& cmd = _history_stack[_history_index - 1];
	cmd->apply(this);
}

#endif
