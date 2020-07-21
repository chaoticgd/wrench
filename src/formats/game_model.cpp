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

#include "game_model.h"

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../app.h"
#include "../util.h"

moby_model::moby_model(
	stream* backing,
	std::size_t base_offset,
	std::size_t size,
	std::size_t submodel_table_offset,
	std::vector<std::size_t> submodel_counts_)
	: submodel_counts(std::move(submodel_counts_)),
	  _backing(backing, base_offset, size),
	  _submodel_table_offset(submodel_table_offset) {
	_backing.name = "Moby Model";
}

void moby_model::read() {
	std::size_t total_submodel_count = submodel_counts[0];
	
	std::vector<moby_submodel_entry> submodel_entries;
	submodel_entries.resize(total_submodel_count);
	_backing.seek(_submodel_table_offset);
	_backing.read_v(submodel_entries);
	
	submodels.clear();
	for(moby_submodel_entry& entry : submodel_entries) {
		moby_submodel submodel;
		
		submodel.vif_list = parse_vif_chain(
			&_backing, entry.vif_list_offset, entry.vif_list_quadword_count);
			
		auto interpreted_vif_list = interpret_vif_list(
			submodel.vif_list, _backing.name.c_str(), submodels.size());
		submodel.st_coords = std::move(interpreted_vif_list.st_data);
		submodel.subsubmodels = read_subsubmodels(
			interpreted_vif_list, _backing.name.c_str(), submodels.size());
		
		auto vertex_header = _backing.read<moby_model_vertex_table_header>(entry.vertex_offset);
		if(vertex_header.vertex_table_offset / 0x10 > entry.vertex_data_quadword_count) {
			fprintf(stderr, "Error: Model %s submodel %ld has bad vertex table offset or size.\n",
				_backing.name.c_str(), submodels.size());
			continue;
		}
		submodel.vertices.resize(entry.vertex_data_quadword_count - vertex_header.vertex_table_offset / 0x10);
		_backing.seek(entry.vertex_offset + vertex_header.vertex_table_offset);
		_backing.read_v(submodel.vertices);
		
		if(!validate_indices(submodel)) {
			fprintf(stderr, "warning: Model %s submodel %ld has indices that overrun the vertex table.\n", _backing.name.c_str(), submodels.size());
		}
		
		submodels.emplace_back(std::move(submodel));
	}
}

moby_model::interpreted_moby_vif_list moby_model::interpret_vif_list(
		const std::vector<vif_packet>& vif_list,
		const char* model_name,
		std::size_t submodel_index) {
	interpreted_moby_vif_list result;
	
	std::size_t unpack_index = 0;
	for(const vif_packet& packet : vif_list) {
		// Skip no-ops.
		if(!packet.code.is_unpack()) {
			continue;
		}
		
		switch(unpack_index) {
			case 0: { // ST unpack.
				if(packet.code.unpack.vnvl != +vif_vnvl::V2_16) {
					fprintf(stderr, "warning: Model %s submodel %ld has malformed first UNPACK (wrong format).\n", model_name, submodel_index);
					return {};
				}
				result.st_data.resize(packet.data.size() / sizeof(moby_model_st));
				std::memcpy(result.st_data.data(), packet.data.data(), packet.data.size());
				break;
			}
			case 1: { // Index buffer unpack.
				if(packet.data.size() < 4) {
					fprintf(stderr, "warning: Model %s submodel %ld has malformed second UNPACK (too small).\n", model_name, submodel_index);
					return {};
				}
				result.index_header = *(uint32_t*) &packet.data.front();
				result.indices.resize(packet.data.size() - 4);
				std::memcpy(result.indices.data(), packet.data.data() + 4, packet.data.size() - 4);
				break;
			}
			case 2: { // Texture unpack (optional).
				if(packet.data.size() % sizeof(moby_model_texture_data) != 0) {
					fprintf(stderr, "warning: Model %s submodel %ld has malformed third UNPACK (wrong size).\n", model_name, submodel_index);
					return {};
				}
				if(packet.code.unpack.vnvl != +vif_vnvl::V4_32) {
					fprintf(stderr, "warning: Model %s submodel %ld has malformed third UNPACK (wrong format).\n", model_name, submodel_index);
					return {};
				}
				result.textures.resize(packet.data.size() / sizeof(moby_model_texture_data));
				std::memcpy(result.textures.data(), packet.data.data(), packet.data.size());
				break;
			}
			case 3: {
				fprintf(stderr, "warning: Too many UNPACK packets in model %s submodel %ld VIF list.\n", model_name, submodel_index);
				return {};
			}
		}
		
		unpack_index++;
	}
	
	if(unpack_index < 2) {
		fprintf(stderr, "warning: VIF list for model %s submodel %ld doesn't have enough UNPACK packets.\n", model_name, submodel_index);
		return {};
	}
	
	return result;
}

std::vector<moby_subsubmodel> moby_model::read_subsubmodels(
		interpreted_moby_vif_list submodel_data,
		const char* model_name,
		std::size_t submodel_index) {
	std::vector<moby_subsubmodel> result;
	
	std::optional<moby_model_texture_data> texture;
	std::size_t next_texture_index = 0;
	std::size_t start_index = 0;
	
	for(std::size_t i = 0; i < submodel_data.indices.size(); i++) {
		if(submodel_data.indices[i] == 0) {
			// Not sure if this is correct. We should try to figure out what
			// loop condition the game uses for processing indices.
			if(i < submodel_data.indices.size() - 4) {
				// At this point the game would push a command to update the
				// GS texture registers.
				if(next_texture_index >= submodel_data.textures.size()) {
					fprintf(stderr, "warning: Model %s submodel %ld has too few textures for its index buffer!\n", model_name, submodel_index);
					return {};
				}
				*texture = submodel_data.textures[next_texture_index++];
			}
			// If there were no previous subsubmodels in this submodel, we
			// don't need to try and create one now. This happens when the
			// first index in a submodel updates the texture.
			if(start_index != i) {
				moby_subsubmodel subsubmodel;
				subsubmodel.indices.resize(i - start_index - 1);
				subsubmodel.sign_bits.resize(i - start_index - 1);
				for(std::size_t i = 0; i < subsubmodel.indices.size(); i++) {
					int8_t index = submodel_data.indices[start_index + 1 + i];
					if(index > 0) {
						subsubmodel.indices[i] = index;
						subsubmodel.sign_bits[i] = 0;
					} else if(index < 0) {
						subsubmodel.indices[i] = index + 128;
						subsubmodel.sign_bits[i] = 1;
					} else {
						assert(false);
					}
				}
				subsubmodel.texture = texture;
				result.emplace_back(std::move(subsubmodel));
				
				// For the next subsubmodel.
				start_index = i;
			}
		}
	}
	
	return result;
}

bool moby_model::validate_indices(const moby_submodel& submodel) {
	for(const moby_subsubmodel& subsubmodel : submodel.subsubmodels) {
		for(uint8_t index : subsubmodel.indices) {
			if(index >= submodel.vertices.size()) {
				return false;
			}
		}
	}
	return true;
}

void moby_model::write() {
	// TODO
}

std::string moby_model::resource_path() const {
	return _backing.resource_path();
}

GLuint moby_model::texture(app& a, std::size_t index) {
	GLuint result = 0;
	if(auto project = a.get_project()) {
		auto& tex_list = project->armor().textures;
		if(index < tex_list.size()) {
			result = tex_list.at(texture_base_index + index).opengl_id();
		}
	}
	return result;
}
