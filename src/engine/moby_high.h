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

// Convert between the low-level representation of moby meshes and glTF meshes.
//
// For unpacking: read_class -> recover_packets -> merge_packets
// For packing: split_packets -> build_packets -> write_class

namespace MOBY {

std::vector<GLTF::Mesh> recover_packets(const std::vector<MobyPacket>& packets, s32 o_class, f32 scale, bool animated);

// Expects the input meshes to have restart bit tristrips instead of zero area
// tristrips (see comment below). Also, it expects the input meshes to have
// effective material indices instead of regular indices.
std::vector<MobyPacket> build_packets(const std::vector<GLTF::Mesh> input, const std::vector<EffectiveMaterial>& effectives, const std::vector<Material>& materials, f32 scale);

GLTF::Mesh merge_packets(const std::vector<GLTF::Mesh>& packets, Opt<std::string> name);

// Split up a mesh into moby packet-sized submeshes with the restart bit
// representation instead of the zero area tri representation.
// Hence, to export the result as a glTF mesh you'll have to convert it back
// (see src/core/tristrip.h). Also, the "material" indices are actually
// effective material indices.
std::vector<GLTF::Mesh> split_packets(const GLTF::Mesh& mesh, const std::vector<s32>& material_to_effective, bool output_broken_indices);

}

#endif
