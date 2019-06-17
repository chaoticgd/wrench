#include "app.h"

bool app::has_level() const {
	return _level.get() != nullptr;
}

level& app::get_level() {
	return *_level.get();
}

const level_impl& app::read_level() const {
	return *_level.get();
}

void app::set_level(std::unique_ptr<level_impl> level) {
	_level.swap(level);
}
