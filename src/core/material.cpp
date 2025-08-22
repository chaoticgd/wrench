/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "material.h"

EffectiveMaterialsOutput effective_materials(const std::vector<Material>& materials, u32 attributes)
{
	EffectiveMaterialsOutput output;
	output.material_to_effective.resize(materials.size(), -1);
	for (size_t i = 0; i < materials.size(); i++) {
		if (output.material_to_effective[i] == -1) {
			s32 effective_index = (s32) output.effectives.size();
			EffectiveMaterial& effective = output.effectives.emplace_back();
			for (size_t j = 0; j < materials.size(); j++) {
				if (output.material_to_effective[j] == -1) {
					bool equal = true;
					if (attributes & MATERIAL_ATTRIB_SURFACE) {
						equal &= materials[i].surface == materials[j].surface;
					}
					if (attributes & MATERIAL_ATTRIB_WRAP_MODE) {
						equal &= materials[i].wrap_mode_s == materials[j].wrap_mode_s;
						equal &= materials[i].wrap_mode_t == materials[j].wrap_mode_t;
					}
					if (attributes & MATERIAL_ATTRIB_METAL_MODE) {
						equal &= materials[i].metal_mode == materials[j].metal_mode;
					}
					if (equal) {
						effective.materials.emplace_back((s32) j);
						output.material_to_effective[j] = effective_index;
					}
				}
			}
		}
	}
	return output;
}
