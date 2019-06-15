#ifndef GUI_H
#define GUI_H

#include <functional>

#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "imgui/examples/imgui_impl_glfw.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include "app.h"
#include "tool.h"

namespace gui {
	void render(app& a);

	void file_new(app& a);
	void file_import_rc2_level(app& a);

	class string_input : public tool {
	public:
		string_input(const char* title);

		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;

		void on_okay(std::function<void(app&, std::string)> callback);

	private:
		const char* _title_text;
		std::vector<char> _buffer;
		std::function<void(app&, std::string)> _callback;
	};

	class moby_list : public tool {
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};

	class inspector : public tool {
		const char* title_text() const override;
		ImVec2 initial_size() const override;
		void render(app& a) override;
	};
}

#endif
