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

#include <gui/gui.h>
#include "gui/imgui_includes.h"

struct app;
struct Level;

class Tool {
public:
	virtual ~Tool() {}

	GlTexture icon;
	
	virtual void draw(app& a, glm::mat4 world_to_clip) = 0;
};

std::vector<std::unique_ptr<Tool>> enumerate_tools();

class PickerTool : public Tool {
public:
	PickerTool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	// Allows the user to select an object by clicking on it. See:
	// https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
	void pick_object(app& a, glm::mat4 world_to_clip, ImVec2 position);
};

class SelectionTool : public Tool {
public:
	SelectionTool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	bool _selecting = false;
	ImVec2 _selection_begin { 0, 0 };
};

class TranslateTool : public Tool {
public:
	TranslateTool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	glm::vec3 _displacement;
};

#endif
