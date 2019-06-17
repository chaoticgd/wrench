#ifndef APP_H
#define APP_H

#include <vector>
#include <memory>
#include <set>

#include "window.h"
#include "level.h"

class tool;

struct app {
	window main_window;
	std::vector<std::unique_ptr<tool>> tools;
	std::vector<uint32_t> selection;

	glm::vec2 mouse_last;
	glm::vec2 mouse_diff;
	std::set<int> keys_down;

	bool has_level() const;
	level& get_level();
	const level_impl& read_level() const;
	void set_level(std::unique_ptr<level_impl> level);

private:
	std::unique_ptr<level_impl> _level;
};

#endif
