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

#include "shrub.h"

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../app.h"
#include "../util.h"
#include "model_utils.h"

shrub_model::shrub_model(
	stream* backing,
	std::size_t base_offset,
	std::size_t size)
	: _backing(backing, base_offset, size)
{
	_backing.name = "Shrub Model";
}

void shrub_model::read() {
	std::size_t submodel_count;
	std::size_t submodel_table_offset = 0;
	std::size_t p1 = 0, p2 = 0;

	auto header = _backing.read<shrub_model_header>(0);
	submodel_count = header.submodel_count;
	submodel_table_offset = sizeof(shrub_model_header);

	scale = header.scale;

	std::vector<shrub_submodel_entry> submodel_entries;
	submodel_entries.resize(submodel_count);
	_backing.seek(submodel_table_offset);
	_backing.read_v(submodel_entries);

	submodels.clear();
	for (shrub_submodel_entry& entry : submodel_entries) {
		shrub_submodel submodel;

		submodel.vif_list = parse_vif_chain(
			&_backing, entry.vif_list_offset, entry.vif_list_size / 0x10);

		auto interpreted_vif_list = interpret_vif_list(submodel.vif_list);
		submodel.st_coords = std::move(interpreted_vif_list.st_data);
		submodel.subsubmodels = read_subsubmodels(interpreted_vif_list);

		submodel.vertices.resize(interpreted_vif_list.vertices.size());
		memcpy(submodel.vertices.data(), interpreted_vif_list.vertices.data(), interpreted_vif_list.vertices.size() * sizeof(shrub_model_vertex));

		/*
		// Basically shrubs only contain a collection of vertices
		// These vertices contain some kind of id/index that is used to group vertices together
		// into a chain
		// 
		// Given a chain of vertices 0,1,2,3,4 the following faces will be generated:
		//	0,1,2
		//  1,2,3
		//  2,3,4
		//  
		// The id for vertex n will match the id for vertex n-1 iff the id for vertex n is equal to the id for n-1 plus 3.
		// To mark the end of a chain, the id delta will be 4 instead of 3.

		// The following code uses p1 and p2 to find the start and end of a chain
		for (p2 = 1; p2 < submodel.vertices.size(); ++p2) {
			auto vertex = submodel.vertices[p2];
			auto last_vertex = submodel.vertices[p2 - 1];

			// if this vertex id is not equal to the last vertex id plus 3, then we've found a vertex chain
			if (vertex.id != (last_vertex.id + 3)) {
				add_vertex_chain(submodel, p1, p2 - 1);
				p1 = p2;
			}
		}

		// add the final vertex chain
		if (p1 < p2-1)
			add_vertex_chain(submodel, p1, p2 - 1);
		*/

		if (!validate_indices(submodel)) {
			warn_current_submodel("indices that overrun the vertex table");
		}

		submodels.emplace_back(std::move(submodel));
	}
}

void shrub_model::add_vertex_chain(
	const shrub_submodel& submodel,
	size_t start,
	size_t end) {

	for (int i = start+2; i < end; ++i) {
		for (int j = i - 2; j <= i; ++j) {
			_triangles.push_back(submodel.vertices[j].x / 1024.0);
			_triangles.push_back(submodel.vertices[j].y / 1024.0);
			_triangles.push_back(submodel.vertices[j].z / 1024.0);
		}
	}
}

shrub_model::interpreted_shrub_vif_list shrub_model::interpret_vif_list(
	const std::vector<vif_packet>& vif_list) {
	interpreted_shrub_vif_list result;

	std::size_t unpack_index = 0;
	for (const vif_packet& packet : vif_list) {
		// Skip no-ops.
		if (!packet.code.is_unpack()) {
			continue;
		}

		switch (unpack_index) {
		case 0: { // Texture def/vertex chain def data unpack
			if (packet.data.size() % 0x10 != 0) {
				warn_current_submodel("malformed first UNPACK (wrong size)");
				return {};
			}
			if (packet.code.unpack.vnvl != vif_vnvl::V4_32) {
				warn_current_submodel("malformed first UNPACK (wrong format)");
				return {};
			}

			std::memcpy(&result.header, packet.data.data(), sizeof(shrub_submodel_header));

			result.vertex_chain_defs.resize(result.header.vertex_chain_count);
			std::memcpy(result.vertex_chain_defs.data(),
				packet.data.data() + sizeof(shrub_submodel_header),
				result.header.vertex_chain_count * sizeof(shrub_vertex_chain_entry));

			result.texture_defs.resize(result.header.texture_def_count);
			std::memcpy(result.texture_defs.data(),
				packet.data.data() + sizeof(shrub_submodel_header) + sizeof(shrub_vertex_chain_entry)* result.header.vertex_chain_count,
				result.header.texture_def_count * sizeof(shrub_texture_entry));

			break;
		}
		case 1: { // Vertex buffer unpack
			if (packet.data.size() % sizeof(shrub_model_vertex) != 0) {
				warn_current_submodel("malformed second UNPACK (wrong size)");
				return {};
			}
			if (packet.code.unpack.vnvl != vif_vnvl::V4_16) {
				warn_current_submodel("malformed second UNPACK (wrong format)");
				return {};
			}
			result.vertices.resize(packet.data.size() / sizeof(shrub_model_vertex));
			std::memcpy(result.vertices.data(), packet.data.data(), packet.data.size());
			break;
		}
		case 2: { // ST unpack
			if (packet.data.size() % sizeof(shrub_model_st) != 0) {
				warn_current_submodel("malformed third UNPACK (wrong size)");
				return {};
			}
			if (packet.code.unpack.vnvl != vif_vnvl::V4_16) {
				warn_current_submodel("malformed third UNPACK (wrong format)");
				return {};
			}
			result.st_data.resize(packet.data.size() / sizeof(shrub_model_st));
			std::memcpy(result.st_data.data(), packet.data.data(), packet.data.size());
			break;
		}
		case 3: {
			warn_current_submodel("too many UNPACK packets");
			return {};
		}
		}

		unpack_index++;
	}

	if (unpack_index < 2) {
		warn_current_submodel("VIF list with not enough UNPACK packets");
		return {};
	}

	return result;
}

std::vector<float> shrub_model::triangles() const {
	return _triangles;
}

std::vector<float> shrub_model::colors() const {
	return _vertex_colors;
}


std::vector<shrub_subsubmodel> shrub_model::read_subsubmodels(
	interpreted_shrub_vif_list submodel_data) {
	std::vector<shrub_subsubmodel> result;

	std::optional<shrub_texture_entry> texture;
	std::size_t next_texture_index = 0;
	std::size_t start_index = 0;

	std::size_t vertex_index = 0;
	std::size_t tex_def_index = 0;

	std::vector<int8_t> index_queue; // Like the GS vertex queue.


	for (std::size_t i = 0; i < submodel_data.header.vertex_chain_count; ++i)
	{
		auto chain = submodel_data.vertex_chain_defs[i];

		// find texture
		while (tex_def_index < submodel_data.header.texture_def_count && submodel_data.texture_defs[tex_def_index].id_start > chain.id_start)
			++tex_def_index;

		// if doesn't exist use last texture
		if (tex_def_index >= submodel_data.header.texture_def_count)
			tex_def_index = submodel_data.header.texture_def_count - 1;

		// 
		shrub_subsubmodel subsubmodel;
		subsubmodel.texture = submodel_data.texture_defs[tex_def_index];
		vertex_index += 2;

		// find end
		if (i < (submodel_data.header.vertex_chain_count - 1))
		{
			auto end_chain = submodel_data.vertex_chain_defs[i + 1];
			while (submodel_data.vertices[vertex_index].id < end_chain.id_start)
			{
				subsubmodel.indices.push_back(vertex_index - 2);
				subsubmodel.indices.push_back(vertex_index - 1);
				subsubmodel.indices.push_back(vertex_index - 0);
				++vertex_index;
			}
		}
		else
		{
			while (vertex_index < submodel_data.header.vertex_count)
			{
				subsubmodel.indices.push_back(vertex_index - 2);
				subsubmodel.indices.push_back(vertex_index - 1);
				subsubmodel.indices.push_back(vertex_index - 0);
				++vertex_index;
			}
		}

		// 
		result.emplace_back(std::move(subsubmodel));
	}

	return result;
}


bool shrub_model::validate_indices(const shrub_submodel& submodel) {
	for (const shrub_subsubmodel& subsubmodel : submodel.subsubmodels) {
		for (uint8_t index : subsubmodel.indices) {
			if (index >= submodel.vertices.size()) {
				return false;
			}
		}
	}
	return true;
}

void shrub_model::warn_current_submodel(const char* message) {
	fprintf(stderr, "warning: Model %s (at %s), submodel %ld has %s.\n",
		_backing.name.c_str(), _backing.resource_path().c_str(), submodels.size(), message);
}

std::string shrub_model::resource_path() const {
	return _backing.resource_path();
}
