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
			
		auto interpreted_vif_list = interpret_vif_list(submodel.vif_list);
		submodel.index_header = interpreted_vif_list.index_header;
		submodel.st_coords = std::move(interpreted_vif_list.st_data);
		submodel.subsubmodels = read_subsubmodels(interpreted_vif_list);
		
		auto vertex_header = _backing.read<moby_model_vertex_table_header>(entry.vertex_offset);
		if(vertex_header.vertex_table_offset / 0x10 > entry.vertex_data_quadword_count) {
			warn_current_submodel("bad vertex table offset or size");
			continue;
		}
		submodel.vertices.resize(vertex_header.vertex_count_2 + vertex_header.vertex_count_4 + vertex_header.main_vertex_count);
		_backing.seek(entry.vertex_offset + vertex_header.vertex_table_offset);
		_backing.read_v(submodel.vertices);
		
		// This is almost certainly wrong, but makes the models look better for the time being.
		if(submodel.vertices.size() > 0) {
			for(std::size_t i = 0; i < vertex_header.vertex_count_8; i++) {
				submodel.vertices.push_back(submodel.vertices.back());
			}
		}
		
		if(!validate_indices(submodel)) {
			warn_current_submodel("indices that overrun the vertex table");
		}
		
		submodels.emplace_back(std::move(submodel));
	}
}

moby_model::interpreted_moby_vif_list moby_model::interpret_vif_list(
		const std::vector<vif_packet>& vif_list) {
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
					warn_current_submodel("malformed first UNPACK (wrong format)");
					return {};
				}
				result.st_data.resize(packet.data.size() / sizeof(moby_model_st));
				std::memcpy(result.st_data.data(), packet.data.data(), packet.data.size());
				break;
			}
			case 1: { // Index buffer unpack.
				if(packet.data.size() < 4) {
					warn_current_submodel("malformed second UNPACK (too small)");
					return {};
				}
				result.index_header = *(moby_model_index_header*) &packet.data.front();
				result.indices.resize(packet.data.size() - 4);
				std::memcpy(result.indices.data(), packet.data.data() + 4, packet.data.size() - 4);
				break;
			}
			case 2: { // Texture unpack (optional).
				if(packet.data.size() % sizeof(moby_model_texture_data) != 0) {
					warn_current_submodel("malformed third UNPACK (wrong size)");
					return {};
				}
				if(packet.code.unpack.vnvl != +vif_vnvl::V4_32) {
					warn_current_submodel("malformed third UNPACK (wrong format)");
					return {};
				}
				result.textures.resize(packet.data.size() / sizeof(moby_model_texture_data));
				std::memcpy(result.textures.data(), packet.data.data(), packet.data.size());
				break;
			}
			case 3: {
					warn_current_submodel("too many UNPACK packets");
				return {};
			}
		}
		
		unpack_index++;
	}
	
	if(unpack_index < 2) {
		warn_current_submodel("VIF list with not enough UNPACK packets");
		return {};
	}
	
	return result;
}

std::vector<moby_subsubmodel> moby_model::read_subsubmodels(
		interpreted_moby_vif_list submodel_data) {
	std::vector<moby_subsubmodel> result;
	
	std::optional<moby_model_texture_data> texture;
	std::size_t next_texture_index = 0;
	std::size_t start_index = 0;
	
	std::vector<int8_t> index_queue; // Like the GS vertex queue.
	
	for(std::size_t i = 0; i < submodel_data.indices.size(); i++) {
		if(submodel_data.indices[i] == 0) {
			// Not sure if this is correct. We should try to figure out what
			// loop condition the game uses for processing indices.
			if(i < submodel_data.indices.size() - 4) {
				// At this point the game would push a command to update the
				// GS texture registers.
				if(next_texture_index >= submodel_data.textures.size()) {
					warn_current_submodel("too few textures for its index buffer");
					return {};
				}
				texture = submodel_data.textures[next_texture_index++];
			}
			// If there were no previous subsubmodels in this submodel, we
			// don't need to try and create one now. This happens when the
			// first index in a submodel updates the texture.
			if(start_index != i) {
				moby_subsubmodel subsubmodel;
				// Iterate over one maximal contiguous list of non-zero indices.
				//                  .-----^-----.
				// indices = { 0x0, 0x1, 0x2, 0x3, 0x0, 0x4, ... }
				for(std::size_t j = start_index + 1; j < i; j++) {
					// Unravel the tristrip into a regular GL_TRIANGLES index
					// buffer, but don't draw a triangle if the sign bit is set.
					int8_t index = submodel_data.indices[j];
					if(index_queue.size() < 3) {
						index_queue.push_back(index);
					} else {
						index_queue[0] = index_queue[1];
						index_queue[1] = index_queue[2];
						index_queue[2] = index;
						if(index > 0) { // If drawing kick.
							for(int k = 0; k < 3; k++) {
								int8_t gl_index = index_queue[k] - 1; // 1-indexed.
								if(gl_index < 0) {
									gl_index += 128; // Zero sign bit.
								}
								subsubmodel.indices.push_back(gl_index);
							}
						}
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

void moby_model::warn_current_submodel(const char* message) {
	fprintf(stderr, "warning: Model %s (at %s), submodel %ld has %s.\n",
		_backing.name.c_str(), _backing.resource_path().c_str(), submodels.size(), message);
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
