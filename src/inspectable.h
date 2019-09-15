/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef INSPECTABLE_H
#define INSPECTABLE_H

#include <string>
#include <glm/glm.hpp>
#include <functional>

#include "imgui_includes.h"

class app;
class wrench_project;
struct inspectable;
class inspector_callbacks;

using inspectable_getter = std::function<inspectable*(wrench_project&)>;

struct inspectable {
	virtual ~inspectable() = default;
	
	void _read_only(int) {}
	void _read_only(uint16_t) {}
	void _read_only(std::size_t) {}
	void _read_only(std::string) {}
	void _read_only(glm::vec3) {}
};

template <typename T_owner, typename T>
struct property {
	typedef T    (T_owner::*getter)() const;
	typedef void (T_owner::*setter)(T);
	getter get;
	setter set;
};

#define PROPERTY(type) \
	typename property<T, type>::getter const get, \
	typename property<T, type>::setter set

class inspector_callbacks {
public:
	inspector_callbacks(inspectable* subject);
	
	void category(const char* name);
	
	template <typename T> void input_integer(const char* name, PROPERTY(int        ) = &T::_read_only);
	template <typename T> void input_uint16 (const char* name, PROPERTY(uint16_t   ) = &T::_read_only);
	template <typename T> void input_size_t (const char* name, PROPERTY(std::size_t) = &T::_read_only);
	template <typename T> void input_string (const char* name, PROPERTY(std::string) = &T::_read_only);
	template <typename T> void input_vector3(const char* name, PROPERTY(glm::vec3  ) = &T::_read_only);	
		
	int i;
	
private:
	void begin_property(const char* name);
	void end_property();

	inspectable* _subject;
};

template <typename T>
void inspector_callbacks::input_integer(const char* name, PROPERTY(int)) {
	begin_property(name);
	int value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputInt("##input", &value, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		(static_cast<T*>(_subject)->*set)(value);
	}
	end_property();
}

template <typename T>
void inspector_callbacks::input_uint16(const char* name, PROPERTY(uint16_t)) {
	begin_property(name);
	int value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputInt("##input", &value, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		(static_cast<T*>(_subject)->*set)(value);
	}
	end_property();
}

template <typename T>
void inspector_callbacks::input_size_t(const char* name, PROPERTY(std::size_t)) {
	T* immed = static_cast<T*>(_subject);
	
	begin_property(name);
	int value = (immed->*get)();
	if(ImGui::InputInt("##input", &value, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
		(immed->*set)(value);
	}
	end_property();
}

template <typename T>
void inspector_callbacks::input_string(const char* name, PROPERTY(std::string)) {
	begin_property(name);
	std::string value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputText("##input", &value, ImGuiInputTextFlags_EnterReturnsTrue)) {
		(static_cast<T*>(_subject)->*set)(value);
	}
	end_property();
}

template <typename T>
void inspector_callbacks::input_vector3(const char* name, PROPERTY(glm::vec3)) {
	begin_property(name);
	glm::vec3 value = (static_cast<T*>(_subject)->*get)();
	if(ImGui::InputFloat3("##input", &value.x, 3, ImGuiInputTextFlags_EnterReturnsTrue)) {
		(static_cast<T*>(_subject)->*set)(value);
	}
	end_property();
}

#endif
