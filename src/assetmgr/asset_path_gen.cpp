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

#include "asset_path_gen.h"

template <typename ClassAsset>
static std::string generate_asset_path(const char* directory, const char* type, s32 tag, const Asset& parent)
{
	if (parent.has_child(tag)) {
		const ClassAsset* child = parent.get_child(tag).maybe_as<ClassAsset>();
		if (child && child->has_name() && !child->name().empty()) {
			std::string name = to_snake_case(child->name().c_str());
			if (child->has_category() && !child->category().empty()) {
				std::string category = to_snake_case(child->category().c_str());
				return stringf("%s/%s/%d_%s/%s_%s.asset", directory, category.c_str(), tag, name.c_str(), type, name.c_str());
			} else {
				return stringf("%s/%d_%s/%s_%s.asset", directory, tag, name.c_str(), type, name.c_str());
			}
		}
	}
	
	return stringf("%s/unsorted/%d/%s_%d.asset", directory, tag, type, tag);
}

std::string generate_level_asset_path(s32 tag, const Asset& parent)
{
	return generate_asset_path<LevelAsset>("levels", "level", tag, parent);
}

std::string generate_moby_class_asset_path(s32 tag, const Asset& parent)
{
	return generate_asset_path<MobyClassAsset>("moby_classes", "moby", tag, parent);
}

std::string generate_tie_class_asset_path(s32 tag, const Asset& parent)
{
	return generate_asset_path<TieClassAsset>("tie_classes", "tie", tag, parent);
}

std::string generate_shrub_class_asset_path(s32 tag, const Asset& parent)
{
	return generate_asset_path<ShrubClassAsset>("shrub_classes", "shrub", tag, parent);
}
