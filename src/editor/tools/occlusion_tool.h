/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef EDITOR_TOOLS_OCCLUSION_TOOL_H
#define EDITOR_TOOLS_OCCLUSION_TOOL_H

// Really hacky occlusion debugger. Disabled by default because it's of such
// poor quality, but still kept in source control since it's still useful.

#include <engine/occlusion.h>
#include <editor/tools.h>
#include <instancemgr/gameplay.h>

class OcclusionTool : public Tool {
public:
	OcclusionTool();

	void draw(app& a, glm::mat4 world_to_clip) override;

private:
	std::string gameplay_path;
	std::string occlusion_path;
	Gameplay gameplay;
	std::vector<OcclusionOctant> octants;
	s32 index = -1;
	std::string error;
};

#endif
