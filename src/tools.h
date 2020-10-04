/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef TOOLS_H
#define TOOLS_H

#include "gl_includes.h"
#include "imgui_includes.h"
#include "formats/level_impl.h"

struct app;
struct level;

class tool {
public:
	virtual ~tool() {}

	gl_texture icon;
	
	virtual void draw(app& a, glm::mat4 world_to_clip) = 0;
};

std::vector<std::unique_ptr<tool>> enumerate_tools();

class picker_tool : public tool {
public:
	picker_tool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	// Allows the user to select an object by clicking on it. See:
	// https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
	void pick_object(app& a, glm::mat4 world_to_clip, ImVec2 position);
};

class selection_tool : public tool {
public:
	selection_tool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	bool _selecting = false;
	ImVec2 _selection_begin { 0, 0 };
};

class translate_tool : public tool {
public:
	translate_tool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	glm::vec3 _displacement;
};

class spline_tool : public tool {
public:
	spline_tool();
	
	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	entity_id _selected_spline = NULL_ENTITY_ID;
	size_t _selected_vertex = 0;
	glm::vec3 _plane_normal { 0.f, 0.f, 1.f };
};

#endif
