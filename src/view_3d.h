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

#ifndef VIEW_3D_H
#define VIEW_3D_H


#define GLM_ENABLE_EXPERIMENTAL

#include "app.h"
#include "model.h"
#include "window.h"

# /*
#	Draw 3D view and overlay text.
# */

class view_3d : public window {
public:
	view_3d(app* a);
	~view_3d();
	const char* title_text() const;
	ImVec2 initial_size() const;
	void render(app& a);
	
	[[nodiscard]] bool has_padding() const override;

	void draw_level(level& lvl) const;
	
	void draw_overlay_text(level& lvl) const;
	
	[[nodiscard]] glm::mat4 get_world_to_clip() const;
	[[nodiscard]] glm::mat4 get_local_to_clip(glm::mat4 world_to_clip, glm::vec3 position, glm::vec3 rotation) const;
	[[nodiscard]] glm::vec3 apply_local_to_screen(glm::mat4 world_to_clip, glm::mat4 local_to_world) const;
	
	// Allows the user to select an object by clicking on it. See:
	// https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
	void pick_object(level& lvl, ImVec2 position);
	void draw_pickframe(level& lvl) const;
	
	void select_rect(level& lvl, ImVec2 position);
private:
	GLuint _frame_buffer_texture;
	GLuint _zbuffer_texture;
	ImVec2 _viewport_size;
	bool _selecting;
	ImVec2 _selection_begin;
	ImVec2 _selection_end;
	
	gl_renderer* _renderer;
};

#endif
