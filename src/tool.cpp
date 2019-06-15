#include "tool.h"

tool::tool() {
	static int s_id = 0;
	_id = ++s_id;
}

int tool::id() {
	return _id;
}

void tool::close(app& a) {
	auto iter = std::find_if(a.tools.begin(), a.tools.end(),
		[=](auto& ptr) { return ptr.get() == this; });
	a.tools.erase(iter);
}
