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

#include "occlusion_tool.h"

#include <editor/app.h>

#define GET_BIT(val_dest, mask_src, index, size) \
	{ \
		s32 array_index = (index) >> 3; \
		s32 bit_index = (index) & 7; \
		verify(array_index > -1 || array_index < size, "Tried to get a bit out of range."); \
		val_dest = ((mask_src)[array_index] >> bit_index) & 1; \
	}

OcclusionTool::OcclusionTool() {
	icon = load_icon(3);
}

void OcclusionTool::draw(app& a, glm::mat4 world_to_clip) {
	Level* lvl = a.get_level();
	if(!lvl) return;
	
	Gameplay& loaded_gameplay = lvl->gameplay();
	
	ImGui::Begin("Occlusion Debugger");
	ImGui::InputText("Gameplay Path", &gameplay_path);
	ImGui::InputText("Occlusion Path", &occlusion_path);
	if(ImGui::Button("Load Octants")) {
		std::vector<u8> gameplay_buffer = read_file(gameplay_path);
		std::vector<u8> occlusion_buffer = read_file(occlusion_path);
		PvarTypes types;
		read_gameplay(gameplay, types, gameplay_buffer, a.game, *gameplay_block_descriptions_from_game(a.game));
		octants = read_occlusion_grid(occlusion_buffer);
	}
	if(!octants.empty() && ImGui::Button("Select Potentially Visible Set")) {
		s32 octant_x = a.render_settings.camera_position.x / 4;
		s32 octant_y = a.render_settings.camera_position.y / 4;
		s32 octant_z = a.render_settings.camera_position.z / 4;
		
		// Find the current octant the camera is in.
		OcclusionOctant* octant = nullptr;
		for(OcclusionOctant& oct : octants) {
			if(oct.x == octant_x && oct.y == octant_y && oct.z == octant_z) {
				octant = &oct;
			}
		}
		
		// Select all tie instances visible from that octant.
		if(octant) {
			verify_fatal(opt_size(gameplay.tie_instances) == opt_size(loaded_gameplay.tie_instances));
			verify_fatal(gameplay.occlusion.has_value());
			if(gameplay.tie_instances.has_value() && loaded_gameplay.tie_instances.has_value()) {
				for(size_t i = 0; i < gameplay.tie_instances->size(); i++) {
					(*loaded_gameplay.tie_instances)[i].selected = false;
				}
				for(size_t i = 0; i < gameplay.tie_instances->size(); i++) {
					for(OcclusionMapping& map : gameplay.occlusion->tie_mappings) {
						if(map.occlusion_id == (*gameplay.tie_instances)[i].occlusion_index) {
							u8 bit;
							GET_BIT(bit, octant->visibility, map.bit_index, 128);
							if(bit) {
								(*loaded_gameplay.tie_instances)[i].selected = true;
							}
						}
					}
				}
			}
			error.clear();
		} else {
			error = "No visibility data for current octant!";
		}
	}
	ImGui::Text("%s", error.c_str());
	ImGui::End();
}
