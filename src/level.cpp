#include "level.h"

level::level()
	: _history_index(0) { }

const texture_provider* level::get_texture_provider() const {
	return const_cast<level*>(this)->get_texture_provider();
}

std::vector<const point_object*> level::point_objects() const {
	std::vector<const point_object*> result;
	for(auto shrub : shrubs()) {
		result.push_back(shrub);
	}
	for(auto moby : mobies()) {
		result.push_back(moby.second);
	}
	return result;
}

std::vector<const shrub*> level::shrubs() const {
	auto result = const_cast<level*>(this)->shrubs();
	return std::vector<const shrub*>(result.begin(), result.end());
}

std::map<uint32_t, const moby*> level::mobies() const {
	auto moby_map = const_cast<level*>(this)->mobies();
	std::map<uint32_t, const moby*> result;
	for(auto& [uid, moby] : moby_map) {
		result.emplace(uid, moby);
	}
	return result;
}

bool level::is_selected(const game_object* obj) const {
	return std::find(selection.begin(), selection.end(), obj) != selection.end();
}

void level::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[--_history_index]->undo();
}

void level::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index++]->apply();
}
