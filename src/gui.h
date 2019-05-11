#ifndef GUI_H
#define GUI_H

#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include "app.h"

namespace gui {
	void render(app& a);

	void file_new(app& a);

	void new_frame();
}

#endif
