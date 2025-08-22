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

#ifndef INSTANCEMGR_PVAR_H
#define INSTANCEMGR_PVAR_H

#include <map>

#include <core/build_config.h>
#include <cppparser/cpp_type.h>
#include <instancemgr/instance.h>

struct Gameplay;
struct Instances;

// By the time these functions are called the moby, camera and sound instances
// should have already been transferred to the destination object.

// Take the pvar sections from the gameplay file, distribute their contents
// amongst the instances, and try to recover type information.
void recover_pvars(
	Instances& dest, std::vector<CppType>& pvar_types_dest, const Gameplay& src, Game game);

// Bake the pvar data from the instances down into a number of data sections to
// be stored in the gameplay file.
void build_pvars(Gameplay& dest, const Instances& src, const std::map<std::string, CppType>& types_src);

// Determine the C++ type name for a pvar structure e.g. "update123" for moby 123.
std::string pvar_type_name_from_instance(const Instance& inst);

#endif
