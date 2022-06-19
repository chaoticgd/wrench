/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#ifndef FORMATS_LEVEL_IMPL_H
#define FORMATS_LEVEL_IMPL_H

#include <memory>
#include <optional>
#include <stdint.h>

#include <core/level.h>
#include "../util.h"
#include "../fs_includes.h"
#include "../mesh.h"

struct EditorMobyClass {
	Mesh mesh;
	RenderMesh high_lod;
	std::vector<RenderMaterial> materials;
};

class level {
public:
	level() {}
	level(const level& rhs) = delete;
	level(level&& rhs) = default;
	
	void open(fs::path json_path, Json& json);
	void save();
	
	fs::path path;
	Game game;
	
	std::map<s32, EditorMobyClass> mobies;
	std::vector<RenderMesh> collision;
	std::vector<RenderMaterial> collision_materials;
	
	Gameplay& gameplay() { return _gameplay; }
	
private:
	Gameplay _gameplay;

public:
	void push_command(std::function<void(level&)> apply, std::function<void(level&)> undo);
	void undo();
	void redo();
private:
	struct UndoRedoCommand {
		std::function<void(level& lvl)> apply;
		std::function<void(level& lvl)> undo;
	};

	std::size_t _history_index = 0;
	std::vector<UndoRedoCommand> _history_stack;
};

class command_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

#endif
