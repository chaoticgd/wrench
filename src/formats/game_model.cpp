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

#include "../util.h"

moby_model::moby_model(
	stream* backing,
	std::size_t base_offset,
	std::size_t size,
	std::size_t submodel_table_offset,
	std::vector<std::size_t> submodel_counts_)
	: submodel_counts(std::move(submodel_counts_)),
	  thumbnail(0),
	  _vertex_buffer(0),
	  _vertex_count(0),
	  _backing(backing, base_offset, size),
	  _submodel_table_offset(submodel_table_offset) {
	_backing.name = "Moby Model";
	read();
}

moby_model::moby_model(moby_model&& rhs)
	: submodel_counts(std::move(rhs.submodel_counts)),
	  submodels(std::move(rhs.submodels)),
	  thumbnail(rhs.thumbnail),
	  _vertex_buffer(rhs._vertex_buffer),
	  _vertex_count(rhs._vertex_count),
	  _backing(std::move(rhs._backing)),
	  _submodel_table_offset(rhs._submodel_table_offset) {
	rhs.thumbnail = 0;
	rhs._vertex_buffer = 0;
	rhs._vertex_count = 0;
}

moby_model::~moby_model() {
	for(moby_model_submodel& submodel : submodels) {
		if(submodel.vertex_buffer != 0) {
			glDeleteBuffers(1, &submodel.vertex_buffer);
		}
	}
	if(thumbnail != 0) {
		glDeleteTextures(1, &thumbnail);
	}
	if(_vertex_buffer != 0) {
		glDeleteBuffers(1, &_vertex_buffer);
	}
}

void moby_model::read() {
	std::size_t total_submodel_count = 0;
	for(std::size_t submodel_count : submodel_counts) {
		total_submodel_count += submodel_count;
	}
	
	std::vector<moby_model_submodel_entry> submodel_entries;
	submodel_entries.resize(total_submodel_count);
	_backing.seek(_submodel_table_offset);
	_backing.read_v(submodel_entries);
	
	submodels = {};
	for(moby_model_submodel_entry& entry : submodel_entries) {
		moby_model_submodel submodel;
		
		submodel.vif_list = parse_vif_chain(&_backing, entry.vif_list_offset, entry.vif_list_qwc);
		
		std::size_t unpack_index = 0;
		for(vif_packet& packet : submodel.vif_list) {
			// Skip no-ops.
			if(!packet.code.is_unpack()) {
				continue;
			}
			
			switch(unpack_index) {
				case 0: { // ST unpack.
					if(packet.code.unpack.vnvl != +vif_vnvl::V2_16) {
						fprintf(stderr, "Error: Submodel %ld has malformed first UNPACK (wrong format: %s).\n",
							submodels.size(), packet.code.unpack.vnvl._to_string());
						continue;
					}
					submodel.st_data.resize((packet.data.size() * sizeof(uint32_t)) / sizeof(moby_model_st));
					std::memcpy(submodel.st_data.data(), packet.data.data(), packet.data.size());
					break;
				}
				case 1: { // Mystery unpack.
					break;
				}
				case 2: { // Texture unpack (optional).
					if(packet.data.size() * sizeof(uint32_t) != sizeof(moby_model_texture_data)) {
						fprintf(stderr, "Error: Submodel %ld has malformed third UNPACK (wrong size).\n", submodels.size());
						continue;
					}
					if(packet.code.unpack.vnvl != +vif_vnvl::V4_32) {
						fprintf(stderr, "Error: Submodel %ld has malformed third UNPACK (wrong format).\n", submodels.size());
						continue;
					}
					submodel.texture = *(moby_model_texture_data*) packet.data.data();
					break;
				}
				case 3: {
					fprintf(stderr, "Error: Too many UNPACK packets in submodel %ld VIF list.\n", submodels.size());
					continue;
				}
			}
			
			unpack_index++;
		}
		
		if(unpack_index < 2) {
			fprintf(stderr, "Error: VIF list for submodel %ld doesn't have enough UNPACK packets.\n", submodels.size());
			continue;
		}
		
		auto vertex_header = _backing.read<moby_model_vertex_table_header>(entry.vertex_offset);
		submodel.vertex_data.resize(vertex_header.vertex_count);
		_backing.seek(_submodel_table_offset + entry.vertex_offset + vertex_header.vertex_table_offset);
		_backing.read_v(submodel.vertex_data);
		
		submodel.visible_in_model_viewer = true;
		submodel.vertex_buffer = 0;
		submodel.st_buffer = 0;
		
		submodels.emplace_back(std::move(submodel));
	}
}

void moby_model::write() {
	// TODO
}

void moby_model::upload_vertex_buffer() {
	// Append submodels together, creating degenerate tris between them.
	std::vector<moby_model_vertex> vertex_data;
	for(moby_model_submodel& submodel : submodels) {
		if(submodel.vertex_data.size() < 3) {
			continue;
		}
		vertex_data.insert(vertex_data.end(), submodel.vertex_data.begin(), submodel.vertex_data.begin() + 1);
		vertex_data.insert(vertex_data.end(), submodel.vertex_data.begin(), submodel.vertex_data.end());
		vertex_data.insert(vertex_data.end(), submodel.vertex_data.end() - 1, submodel.vertex_data.end());
	}
	
	auto opengl_data = moby_vertex_data_to_opengl(vertex_data);
	
	_vertex_count = vertex_data.size();
	glDeleteBuffers(1, &_vertex_buffer);
	glGenBuffers(1, &_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		_vertex_count * sizeof(moby_model_opengl_vertex),
		opengl_data.data(), GL_STATIC_DRAW);
}

void moby_model::setup_vertex_attributes() const {
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) offsetof(moby_model_opengl_vertex, x));
}

GLuint moby_model::vertex_buffer() const {
	return _vertex_buffer;
}

std::size_t moby_model::vertex_count() const {
	return _vertex_count;
}

std::string moby_model::resource_path() const {
	return _backing.resource_path();
}

std::vector<moby_model_opengl_vertex> moby_vertex_data_to_opengl(const std::vector<moby_model_vertex>& vertex_data) {
	std::vector<moby_model_opengl_vertex> result(vertex_data.size());
	for(std::size_t i = 0; i < result.size(); i++) {
		const moby_model_vertex& in_vertex = vertex_data[i];
		result[i] = moby_model_opengl_vertex {
			in_vertex.x / (float) INT16_MAX,
			in_vertex.y / (float) INT16_MAX,
			in_vertex.z / (float) INT16_MAX
		};
	}
	return result;
}
