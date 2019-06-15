#include "app.h"

bool app::has_level() {
	return _level.get() != nullptr;
}

const level_impl& app::level() {
	return *_level.get();
}

void app::set_level(std::unique_ptr<level_impl> level) {
	_level.swap(level);
}
