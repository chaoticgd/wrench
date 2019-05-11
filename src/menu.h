#ifndef MENU_H
#define MENU_H

#include <vector>
#include <functional>
#include <initializer_list>

#include "app.h"

class menu {
public:
	menu(const char* name, std::function<void(app&)> callback);
	menu(const char* name, std::initializer_list<menu> children);

	void render(app& a) const;

private:
	const char* _name;
	std::function<void(app&)> _callback;
	std::vector<menu> _children;
};

#endif
