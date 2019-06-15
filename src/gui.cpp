#include "gui.h"

#include <iostream>
#include <functional>

#include "menu.h"
#include "tool.h"
#include "formats/level_data.h"

void gui::render(app& a) {

	static const std::initializer_list<menu> menu_items = {
		{
			"File", {
				{ "New", file_new },
				{
					"Import", {
						{ "R&C2 Level (.wad)", file_import_rc2_level }
					}
				}
			}
		}
	};

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::BeginMainMenuBar();
	for(auto& menu : menu_items) {
		menu.render(a);
	}
	ImGui::EndMainMenuBar();

	for(auto& current_tool : a.tools) {
		ImGui::SetNextWindowSize(current_tool->initial_size(), ImGuiCond_Appearing);
		std::string title =
			std::string(current_tool->title_text()) +
			"##" + std::to_string(current_tool->id());
	 	ImGui::SetNextWindowSize(current_tool->initial_size(), ImGuiCond_FirstUseEver);
		if(ImGui::Begin(title.c_str())) {
			current_tool->render(a);
		}
		ImGui::End();
	}
}

void gui::file_new(app& a) {
	std::cout << "test\n";
}

void gui::file_import_rc2_level(app& a) {
	auto path_input = std::make_unique<string_input>("Enter File Path");
	path_input->on_okay([](app& a, std::string path) {
		file_stream level_stream(path);
		a.set_level(import_level(level_stream));
	});
	a.tools.emplace_back(std::move(path_input));
}

/*
	string_input
*/

gui::string_input::string_input(const char* title)
	: _title_text(title) {
	_buffer.resize(1024);
}

const char* gui::string_input::title_text() const {
	return _title_text;
}

ImVec2 gui::string_input::initial_size() const {
	return ImVec2(400, 100);
}

void gui::string_input::render(app& a) {
	ImGui::InputText("", _buffer.data(), 1024);
	bool pressed = ImGui::Button("Okay");
	if(pressed) {
		_callback(a, std::string(_buffer.begin(), _buffer.end()));
	}
	pressed |= ImGui::Button("Cancel");
	if(pressed) {
		close(a);
	}
}

void gui::string_input::on_okay(std::function<void(app&, std::string)> callback) {
	_callback = callback;
}

/*
	moby_list
*/

const char* gui::moby_list::title_text() const {
	return "Moby List";
}

ImVec2 gui::moby_list::initial_size() const {
	return ImVec2(200, 500);
}

void gui::moby_list::render(app& a) {
	if(!a.has_level()) {
		return;
	}

	ImGui::PushItemWidth(-1);
	ImGui::ListBoxHeader("##nolabel");
	for(const auto& moby : a.level().mobies()) {
		std::string name =
			moby.second->name + (moby.second->name.size() > 0 ? " " : "") +
			"[" + std::to_string(moby.first) + "]";
		if(ImGui::Selectable(name.c_str(), moby.second->selected)) {
			a.selection = { moby.first };
		}
	}
	ImGui::ListBoxFooter();
	ImGui::PopItemWidth();
}

/*
	inspector
*/

const char* gui::inspector::title_text() const {
	return "Inspector";
}

ImVec2 gui::inspector::initial_size() const {
	return ImVec2(200, 500);
}

void gui::inspector::render(app& a) {
	if(!a.has_level()) {
		ImGui::Text("<no level open>");
		return;
	}

	if(a.selection.size() < 1) {
		ImGui::Text("<no selection>");
		return;
	} else if(a.selection.size() > 1) {
		ImGui::Text("<multiple mobies selected>");
		return;
	}

	moby* selected = a.level().mobies().at(a.selection[0]).get();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 80);

	int i = 0;
	auto begin_property = [&i](const char* name) {
		ImGui::PushID(i++);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s", name);
		ImGui::NextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::PushItemWidth(-1);
	};

	auto end_property = []() {	
		ImGui::NextColumn();
		ImGui::PopID();
		ImGui::PopItemWidth();
	};

	selected->reflect(
		[=](const char* name, rf::property<uint32_t> p) {
			begin_property(name);
			int value = p.get();
			if(ImGui::InputInt("##nolabel", &value)) {
				p.set(value);
			}
			end_property();
		},
		[=](const char* name, rf::property<vec3f> p) {
			begin_property(name);
			vec3f value = p.get();
			if(ImGui::InputFloat3("##nolabel", value.components)) {
				p.set(value);
			}
			end_property();
		},
		[=](const char* name, rf::property<std::string> p) {
			begin_property(name);
			std::string value = p.get();
			if(ImGui::InputText("##nolabel", &value)) {
				p.set(value);
			}
			end_property();
		}
	);

	ImGui::PopStyleVar();
}
