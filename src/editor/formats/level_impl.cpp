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

#include "level_impl.h"
#include "../../lz/compression.h"

#include <editor/app.h>

void level::open(fs::path json_path, Json& json) {
	Opt<Game> game_opt = game_from_string(json["game"]);
	if(!game_opt.has_value()) {
		return;
	}
	game = *game_opt;
	fs::path level_dir = json_path.parent_path();
	path = json_path;
	Json level_json = Json::parse(read_file(json_path, "r"));
	LevelWad wad = read_level_wad_json(level_json, level_dir, game);
	_gameplay = std::move(wad.gameplay);
	for(Mesh& collision_mesh : wad.collision.meshes) {
		collision.emplace_back(upload_mesh(collision_mesh, true));
	}
	collision_materials = upload_materials(wad.collision.materials, {});
	for(MobyClass& cls : wad.moby_classes) {
		if(cls.high_model.has_value() && cls.high_model->meshes.size() >= 1) {
			EditorMobyClass ec;
			ec.mesh = cls.high_model->meshes[0];
			ec.high_lod = upload_mesh(cls.high_model->meshes[0], true);
			ec.materials = upload_materials(cls.high_model->materials, cls.textures);
			mobies.emplace(cls.o_class, std::move(ec));
		}
	}
}

void level::save() {
	Json data_json = to_json(_gameplay);
	
	Json gameplay_json;
	gameplay_json["metadata"] = get_file_metadata("gameplay", "Wrench Level Editor");
	for(auto& item : data_json.items()) {
		gameplay_json[item.key()] = std::move(item.value());
	}
	fs::path dest_dir = path.parent_path();
	Json level_json = Json::parse(read_file(path, "r"));
	std::string gameplay_path = level_json["gameplay"];
	write_file(dest_dir, fs::path(gameplay_path), gameplay_json.dump(1, '\t'));
}

void level::push_command(std::function<void(level&)> apply, std::function<void(level&)> undo) {
	_history_stack.resize(_history_index++);
	_history_stack.emplace_back(UndoRedoCommand {apply, undo});
	apply(*this);
}

void level::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[_history_index - 1].undo(*this);
	_history_index--;
}

void level::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index].apply(*this);
	_history_index++;
}
