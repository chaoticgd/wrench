#ifndef LEVEL_H
#define LEVEL_H

#include <map>
#include <memory>
#include <vector>

#include "command.h"
#include "moby.h"

class level_impl;

// Represents a level currently loaded into wrench. A mostly opaque type,
// allowing only for undo stack manipulation. Only command objects should access
// the (non const) level_impl type directly if we are to have any hope of
// implementing a working undo/redo system!
class level {
public:
	void apply_command(std::unique_ptr<command> action);

	bool undo();
	bool redo();

protected:
	level() {}

	std::vector<std::unique_ptr<command>> _history;
	std::size_t _history_index;
};

class level_impl : public level {
public:
	level_impl();

	void add_moby(uint32_t uid, std::unique_ptr<moby> m) { _mobies[uid].swap(m); }
	const std::map<uint32_t, std::unique_ptr<moby>>& mobies() const { return _mobies; }

private:
	std::map<uint32_t, std::unique_ptr<moby>> _mobies;
};

#endif
