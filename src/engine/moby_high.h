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

#ifndef ENGINE_MOBY_HIGH_H
#define ENGINE_MOBY_HIGH_H

#include <core/gltf.h>
#include <engine/moby_packet.h>

namespace MOBY {

std::vector<GLTF::Mesh> recover_packets(const std::vector<MobyPacket>& packets, s32 o_class, f32 scale, bool animated);
std::vector<MobyPacket> build_packets(const Mesh& mesh, const std::vector<ColladaMaterial>& materials);

GLTF::Mesh merge_packets(const std::vector<GLTF::Mesh>& packets, Opt<std::string> name);
std::vector<GLTF::Mesh> split_packets(const GLTF::Mesh& mesh, bool output_broken_indices);

}

#endif
