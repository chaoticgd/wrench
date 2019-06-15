#ifndef TOOL_H
#define TOOL_H

#include "imgui/imgui.h"

#include "app.h"

class tool {
public:
	tool();

	virtual const char* title_text() const = 0;
	virtual ImVec2 initial_size() const = 0;
	virtual void render(app& a) = 0;

	int id();
	void close(app& a);

private:
	int _id;
};

#endif
