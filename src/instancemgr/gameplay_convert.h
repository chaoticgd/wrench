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

#ifndef INSTANCEMGR_GAMEPLAY_CONVERT_H
#define INSTANCEMGR_GAMEPLAY_CONVERT_H

#include <instancemgr/gameplay.h>

void move_gameplay_to_instances(
	Instances& dest,
	HelpMessages* help_dest,
	std::vector<u8>* occl_dest,
	std::vector<CppType>* types_dest,
	Gameplay& src,
	Game game);
void move_instances_to_gameplay(
	Gameplay& dest,
	Instances& src,
	HelpMessages* help_src,
	std::vector<u8>* occlusion_src,
	const std::map<std::string, CppType>& types_src);
s32 rewrite_link(s32 id, const char* link_type_name, const Instances& instances, const char* context);
s32 rewrite_link(s32 id, InstanceType type, const Instances& instances, const char* context);

#endif
