#include "level.h"

void level::apply_command(std::unique_ptr<command> action) {
	action->inject_level_pointer(static_cast<level_impl*>(this));
	action->apply();
	_history.resize(_history_index - 1);
	_history.push_back(std::move(action));
	_history_index++;
}
	
bool level::undo() {
	if(_history_index <= 0) {
		return false;
	}
	_history[--_history_index]->undo();
	return true;
}

bool level::redo() {
	if(_history_index >= _history.size()) {
		return false;
	}
	_history[_history_index++]->apply();
	return true;
}

level_impl::level_impl() {}
