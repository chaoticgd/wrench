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

#include "tfrag.h"
#include "../util.h"

tfrag::tfrag(stream *backing, std::size_t base_offset, tfrag_entry & entry)
		: _backing(backing, base_offset, 0), _base_offset(base_offset) {
	_backing.name = "TFrag";


	_tfrag_points.resize(entry.vertex_count);
	for (std::size_t i = 0; i < entry.vertex_count; i++) {
		auto vertex = _backing.peek<fmt::vertex>(entry.vertex_offset + i * 0x10);
		_tfrag_points[i].x = vertex.x;
		_tfrag_points[i].y = vertex.y;
		_tfrag_points[i].z = vertex.z;
	}

	_vif_list = parse_vif_chain(
		&_backing, 0, entry.color_offset / 0x10);


	auto interpreted_vif_list = interpret_vif_list(_vif_list);

	std::vector<vec3f> vertices;

	// generate vertices
	vertices.resize(entry.color_count);
	for (int i = 0; i < entry.color_count; ++i)
	{
		// vertices are based around the position
		// each vertex has a displacement, that is +-x, +-y, +-z in int16
		// these values must be divided by 1024 to get their actual coordinates
		auto displace_data = interpreted_vif_list.displace_data[i];
		vertices[i].x = (float)(interpreted_vif_list.position[0] + displace_data.x) / 1024.0;
		vertices[i].y = (float)(interpreted_vif_list.position[1] + displace_data.y) / 1024.0;
		vertices[i].z = (float)(interpreted_vif_list.position[2] + displace_data.z) / 1024.0;
	}

	// generate triangles
	// not sure how they generate triangles since the indices are just a stream of indices
	// but its possible that its based on the vu program
	// or perhaps another value in the tfrag
	// this just builds the triangles by connecting each vertex to its previous vertex and the first vertex
	for (size_t i = 2; i < interpreted_vif_list.indices.size(); ++i)
	{
		// The indices map to the st_data array where the actual vertex id is held
		// The st_data object also contains the st/uv whatever values for the vertex
		auto a_st_index = interpreted_vif_list.st_data[interpreted_vif_list.indices[0]];
		auto b_st_index = interpreted_vif_list.st_data[interpreted_vif_list.indices[i-1]];
		auto c_st_index = interpreted_vif_list.st_data[interpreted_vif_list.indices[i]];

		auto vertex = vertices[a_st_index.vid / 2];
		_tfrag_triangles.push_back(vertex.x);
		_tfrag_triangles.push_back(vertex.y);
		_tfrag_triangles.push_back(vertex.z);
		vertex = vertices[b_st_index.vid / 2];
		_tfrag_triangles.push_back(vertex.x);
		_tfrag_triangles.push_back(vertex.y);
		_tfrag_triangles.push_back(vertex.z);
		vertex = vertices[c_st_index.vid / 2];
		_tfrag_triangles.push_back(vertex.x);
		_tfrag_triangles.push_back(vertex.y);
		_tfrag_triangles.push_back(vertex.z);
	}
}

std::vector<float> tfrag::triangles() const {
	return _tfrag_triangles;
}


std::vector<float> tfrag::colors() const {
	std::vector<float> result;
	return result;
}

tfrag::interpreted_tfrag_vif_list tfrag::interpret_vif_list(
	const std::vector<vif_packet>& vif_list) {
	interpreted_tfrag_vif_list result;

	std::size_t unpack_index = 0;
	for (const vif_packet& packet : vif_list) {
		// Skip no-ops.
		if (!packet.code.is_unpack()) {

			// the position is usually right after the st_index part
			// there are tfrags that have multiple positions but the ones I've seen
			// have all contained the same values for each position.
			// This could produce some errors but it works for now
			if (unpack_index == 5 && packet.code.is_strow())
			{
				std::memcpy(result.position.data(), packet.data.data(), packet.data.size());
			}

			continue;
		}

		switch (unpack_index) {
		case 0: { // indices

			if (packet.code.unpack.vnvl != vif_vnvl::V4_8) {
				warn_current_tfrag("malformed first UNPACK (wrong format)");
				return {};
			}
			
			// Reads in each index as a byte
			result.indices.resize(packet.data.size());
			std::memcpy(result.indices.data(), packet.data.data(), packet.data.size());
			break;
		}
		case 1: { // unknown
			/*
				8A 00 00 00 00 FF FF FF
			*/
			break;
		}
		case 2: { // unknown
			/*
				0A 00 00 00 00 00 00 00 00 00 00 00 0E 00 26 00
				30 00 30 00 30 00 30 00 30 00 30 00 30 00 30 00
				30 00 30 00 33 00 09 00
			*/
			break;
		}
		case 3: { // texture
			/*
				01 00 00 00 00 00 00 00 06 00 00 00 00 00 00 00
				7B FF 00 00 04 00 00 00 14 00 00 00 00 00 00 45
				00 00 00 00 01 00 00 00 08 00 00 00 00 00 00 00
				00 00 00 00 00 00 00 00 34 00 00 00 00 00 00 00
				00 00 00 00 00 00 00 00 36 00 00 00 00 00 00 00
			*/

			if (packet.data.size() % sizeof(tfrag_texture_data) != 0) {
				warn_current_tfrag("malformed third UNPACK (wrong size)");
				return {};
			}
			if (packet.code.unpack.vnvl != vif_vnvl::V4_32) {
				warn_current_tfrag("malformed third UNPACK (wrong format)");
				return {};
			}
			result.textures.resize(packet.data.size() / sizeof(tfrag_texture_data));
			std::memcpy(result.textures.data(), packet.data.data(), packet.data.size());
			break;
		}
		case 4: { // ST and indices
			/*
				00 10 00 10 00 10 00 00 00 10 00 00 00 10 02 00
				00 00 00 10 00 10 04 00 00 00 00 00 00 10 06 00
				00 E0 00 10 00 10 08 00 00 E0 00 00 00 10 0A 00
				00 C0 00 10 00 10 0C 00 00 C0 00 00 00 10 0E 00
				00 A0 00 10 00 10 10 00 00 A0 00 00 00 10 12 00
			*/

			if (packet.data.size() % sizeof(tfrag_st_index) != 0) {
				warn_current_tfrag("malformed fourth UNPACK (wrong size)");
				return {};
			}
			if (packet.code.unpack.vnvl != vif_vnvl::V4_16) {
				warn_current_tfrag("malformed fourth UNPACK (wrong format)");
				return {};
			}

			result.st_data.resize(packet.data.size() / sizeof(tfrag_st_index));
			std::memcpy(result.st_data.data(), packet.data.data(), packet.data.size());
			break;
		}
		default: {


			// Check for displacement
			/*
				B3 35 C2 12 1F F9 0E 30 BA 15 6F 05 C7 15 D2 09
				2B F9 C5 0E 6C 0F BE 05 6C FB 5B FF 2E F9 9D E9
				E2 04 FF 06 E4 E0 2E F0 01 F9 08 DB 6C F2 03 03
				24 CC 46 EA 20 F9 4D CA 9B EA 58 05
			*/

			if (packet.code.unpack.vnvl != vif_vnvl::V3_16) {
				continue;
			}

			int num = packet.data.size() / sizeof(tfrag_displace);
			size_t size = num * sizeof(tfrag_displace);
			size_t initialSize = result.displace_data.size();
			result.displace_data.resize(initialSize + num);
			std::memcpy(result.displace_data.data() + initialSize, packet.data.data(), size);
			break;
		}
		}

		unpack_index++;
	}

	if (unpack_index < 5) {
		warn_current_tfrag("VIF list with not enough UNPACK packets");
		return {};
	}

	return result;
}


void tfrag::warn_current_tfrag(const char* message) {
	fprintf(stderr, "warning: Model %s (at %s), tfrag at %ld has %s.\n",
		_backing.name.c_str(), _backing.resource_path().c_str(), _base_offset, message);
}
