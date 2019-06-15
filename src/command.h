#ifndef COMMAND_H
#define COMMAND_H

#include <stdexcept>

class level;
class level_impl;

class command {
	friend level;
public:
	virtual ~command() {}

protected:
	command() {}

	// Should only throw command_error.
	virtual void apply() = 0;

	// Should only throw command_error.
	virtual void undo() = 0;

	level_impl& lvl() { return *_lvl; }

private:
	void inject_level_pointer(level_impl* lvl) { _lvl = lvl; }

	level_impl* _lvl;
};

class command_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

#endif
