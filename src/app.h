#ifndef APP_H
#define APP_H

#include <vector>
#include <memory>

#include "window.h"
#include "level.h"

class tool;

struct app {
	window main_window;
	std::vector<std::unique_ptr<tool>> tools;
	std::vector<uint32_t> selection;

	bool has_level();
	const level_impl& level();
	void set_level(std::unique_ptr<level_impl> level);

private:
	std::unique_ptr<level_impl> _level;
};

#endif
