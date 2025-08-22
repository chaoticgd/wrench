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

#ifndef CORE_MATERIAL_H
#define CORE_MATERIAL_H

#include <core/util.h>
#include <core/mesh.h>

enum class MaterialSurfaceType
{
	NONE, COLOUR, TEXTURE
};

struct MaterialSurface
{
	MaterialSurface() : type(MaterialSurfaceType::NONE) {}
	MaterialSurface(const glm::vec4& c) : type(MaterialSurfaceType::COLOUR), colour(c) {}
	MaterialSurface(s32 t) : type(MaterialSurfaceType::TEXTURE), texture(t) {}
	bool operator==(const MaterialSurface& rhs) const {
		bool equal = true;
		equal &= type == rhs.type;
		if(type == MaterialSurfaceType::COLOUR) {
			equal &= colour == rhs.colour;
		}
		if(type == MaterialSurfaceType::TEXTURE) {
			equal &= texture == rhs.texture;
		}
		return equal;
	}
	MaterialSurfaceType type;
	union {
		glm::vec4 colour;
		s32 texture;
	};
};

enum class WrapMode
{
	REPEAT, CLAMP
};

enum class MetalEffectMode
{
	OFF, CHROME, GLASS
};

struct Material
{
	std::string name;
	MaterialSurface surface;
	WrapMode wrap_mode_s = WrapMode::REPEAT;
	WrapMode wrap_mode_t = WrapMode::REPEAT;
	MetalEffectMode metal_mode = MetalEffectMode::OFF;
};

enum MaterialAttribute
{
	MATERIAL_ATTRIB_SURFACE = 1 << 1,
	MATERIAL_ATTRIB_WRAP_MODE = 1 << 2,
	MATERIAL_ATTRIB_METAL_MODE = 1 << 3
};

// An effective material is a set of materials that for some subset of
// attributes are equal. For example, an effective material over the texture
// attribute means that for each texture, there will be an effective material
// referencing all materials with that texture. This is used e.g. for generating
// AD GIF data where only the texture index and wrapping mode is relevant i.e.
// materials that vary only by other attributes should be merged.
struct EffectiveMaterial
{
	std::vector<s32> materials;
};

struct EffectiveMaterialsOutput
{
	std::vector<EffectiveMaterial> effectives;
	std::vector<s32> material_to_effective;
};
EffectiveMaterialsOutput effective_materials(const std::vector<Material>& materials, u32 attributes);

#endif
