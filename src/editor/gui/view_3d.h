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

#ifndef EDITOR_GUI_VIEW_3D_H
#define EDITOR_GUI_VIEW_3D_H


#define GLM_ENABLE_EXPERIMENTAL

#include "../app.h"
#include "window.h"

class view_3d : public window {
public:
	view_3d() {}
	~view_3d();
	const char* title_text() const;
	ImVec2 initial_size() const;
	void render(app& a);
	
	bool has_padding() const override;
	
private:
	GLuint _frame_buffer_texture = 0;
};

#endif
