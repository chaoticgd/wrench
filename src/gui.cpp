#include "gui.h"

#include <sstream>
#include <iostream>
#include <functional>
#include <boost/stacktrace.hpp>

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
		try {
			file_stream level_stream(path);
			a.set_level(import_level(level_stream));
		} catch(stream_error& e) {
			std::stringstream message;
			message << "stream_error: " << e.what() << "\n";
			message << boost::stacktrace::stacktrace();
			auto error_box = std::make_unique<message_box>
				("Level Import Failed", message.str());
			a.tools.emplace_back(std::move(error_box));
		}
	});
	a.tools.emplace_back(std::move(path_input));
}

/*
	moby_list
*/

const char* gui::moby_list::title_text() const {
	return "Moby List";
}

ImVec2 gui::moby_list::initial_size() const {
	return ImVec2(250, 500);
}

void gui::moby_list::render(app& a) {
	if(!a.has_level()) {
		return;
	}

	ImGui::PushItemWidth(-1);
	ImGui::ListBoxHeader("##nolabel");
	for(const auto& moby : a.read_level().mobies()) {
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
	return ImVec2(250, 500);
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

	moby* selected = a.read_level().mobies().at(a.selection[0]).get();

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
		[=](const char* name, rf::property<glm::vec3> p) {
			begin_property(name);
			float components[] = { p.get().x, p.get().y, p.get().z };
			if(ImGui::InputFloat3("##nolabel", components)) {
				p.set(glm::vec3(components[0], components[1], components[2]));
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

/*
	viewport_information
*/

const char* gui::viewport_information::title_text() const {
	return "Viewport Information";
}

ImVec2 gui::viewport_information::initial_size() const {
	return ImVec2(250, 150);
}

void gui::viewport_information::render(app& a) {
	glm::vec3 cam_pos = a.read_level().camera_position;
	ImGui::Text("Camera Position:\n\t%.3f, %.3f, %.3f",
		cam_pos.x, cam_pos.y, cam_pos.z);
	glm::vec3 cam_rot = a.read_level().camera_rotation;
	ImGui::Text("Camera Rotation:\n\t%.3f, %.3f, %.3f",
		cam_rot.x, cam_rot.y, cam_rot.z);
	ImGui::Text("Camera Control (Z to toggle):\n\t %s",
		a.read_level().camera_control ? "On" : "Off");
	if(ImGui::Button("Reset Camera")) {
		a.get_level().camera_position = glm::vec3(0, 0, 0);
		a.get_level().camera_rotation = glm::vec3(0, 0, 0);
	}
}

/*
	message_box
*/

gui::message_box::message_box(const char* title, std::string message)
	: _title(title), _message(message) {}

const char* gui::message_box::title_text() const {
	return _title;
}

ImVec2 gui::message_box::initial_size() const {
	return ImVec2(300, 200);
}

void gui::message_box::render(app& a) {
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 16;
	size.y -= 64;
	ImGui::PushItemWidth(-1);
	ImGui::InputTextMultiline("##nolabel", &_message, size, ImGuiInputTextFlags_ReadOnly);
	ImGui::PopItemWidth();
	if(ImGui::Button("Close")) {
		close(a);
	}
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
