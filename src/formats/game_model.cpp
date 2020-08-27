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
#include "model_utils.h"

moby_model::moby_model(
	stream* backing,
	std::size_t base_offset,
	std::size_t size,
	moby_model_header_type type)
	: _backing(backing, base_offset, size),
	  _type(type) {
	_backing.name = "Moby Model";
}

void moby_model::read() {
	std::size_t submodel_count;
	std::size_t submodel_table_offset;
	switch(_type) {
		case moby_model_header_type::LEVEL: {
			auto header = _backing.read<moby_model_level_header>(0);
			submodel_count = header.submodel_count;
			submodel_table_offset = header.rel_offset;
			break;
		}
		case moby_model_header_type::ARMOR: {
			auto header = _backing.read<moby_model_armor_header>(0);
			submodel_count = header.submodel_count_1;
			submodel_table_offset = header.submodel_table_offset;
			break;
		}
	}
	
	std::vector<moby_submodel_entry> submodel_entries;
	submodel_entries.resize(submodel_count);
	_backing.seek(submodel_table_offset);
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
		if(entry.transfer_vertex_count != vertex_header.transfer_vertex_count) {
			warn_current_submodel("conflicting vertex counts");
			continue;
		}
		if(entry.unknown_e != (3 + entry.transfer_vertex_count) / 4) {
			warn_current_submodel("weird value in submodel table entry at +0xe");
			continue;
		}
		if(entry.unknown_d != (0xf + entry.transfer_vertex_count * 6) / 0x10) {
			warn_current_submodel("weird value in submodel table entry at +0xd");
			continue;
		}
		submodel.vertices.resize(vertex_header.vertex_count_2 + vertex_header.vertex_count_4 + vertex_header.main_vertex_count);
		_backing.seek(entry.vertex_offset + vertex_header.vertex_table_offset);
		_backing.read_v(submodel.vertices);
		
		if(vertex_header.main_vertex_count == vertex_header.transfer_vertex_count) {
			warn_current_submodel("woooooo");
		}
		
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

void moby_model::import_ply(std::string path) {
	std::vector<ply_vertex> vertices = read_ply_model(path);
	
	submodels.clear();
	
	const auto emit_submodel = [&](std::size_t begin, std::size_t end) {
		moby_submodel& submodel = submodels.emplace_back();
		
		for(std::size_t i = begin; i < end; i++) {
			ply_vertex& in_vertex = vertices[i];
			
			moby_model_vertex& out_vertex = submodel.vertices.emplace_back();
			out_vertex.unknown_0 = 0xff;
			out_vertex.unknown_1 = 0;
			out_vertex.unknown_2 = 0;
			out_vertex.unknown_3 = 0xf4;
			out_vertex.unknown_4 = 0;
			out_vertex.unknown_5 = 0;
			out_vertex.unknown_6 = 0;
			out_vertex.unknown_7 = 0;
			out_vertex.unknown_8 = 0;
			out_vertex.unknown_9 = 0;
			out_vertex.x = in_vertex.x * INT16_MAX / 8.f;
			out_vertex.y = in_vertex.y * INT16_MAX / 8.f;
			out_vertex.z = in_vertex.z * INT16_MAX / 8.f;
			
			moby_model_st& st = submodel.st_coords.emplace_back();
			st.s = in_vertex.s * INT16_MAX;
			st.t = in_vertex.t * INT16_MAX;
		}
		
		moby_subsubmodel& subsubmodel = submodel.subsubmodels.emplace_back();
		if(begin == 0) {
			moby_model_texture_data tex;
			tex.texture_index = 0;
			subsubmodel.texture = tex;
		}
		for(std::size_t i = 0; i < end - begin; i++) {
			subsubmodel.indices.push_back(i);
		}
		
		submodel.vif_list = regenerate_submodel_vif_list(submodel);
	};
	
	// I'm not sure what the limits are on the size of the index buffer per
	// submodel, so we're going to be quite conservative for now.
	std::size_t i;
	for(i = 0; i < vertices.size() / 0x40; i++) {
		emit_submodel(i * 0x40, (i + 1) * 0x40);
	}
	emit_submodel(i * 0x40, vertices.size());
	
	write();
	read();
}

std::vector<vif_packet> moby_model::regenerate_submodel_vif_list(moby_submodel& submodel) {
	std::vector<vif_packet> result;
	
	static const std::size_t ST_UNPACK_ADDR_QUADWORDS = 0xc2;
	static const std::size_t INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	vif_packet& st_unpack = result.emplace_back();
	
	st_unpack.data.resize(submodel.st_coords.size() * sizeof(moby_model_st));
	std::memcpy(st_unpack.data.data(), submodel.st_coords.data(), st_unpack.data.size());
	
	st_unpack.address = 0; // Fake address.
	st_unpack.code.interrupt = 0;
	st_unpack.code.cmd = (vif_cmd) 0b1100000; // UNPACK
	st_unpack.code.num = submodel.st_coords.size();
	st_unpack.code.unpack.vnvl = vif_vnvl::V2_16;
	st_unpack.code.unpack.flg = vif_flg::USE_VIF1_TOPS;
	st_unpack.code.unpack.usn = vif_usn::SIGNED;
	st_unpack.code.unpack.addr = ST_UNPACK_ADDR_QUADWORDS;
	
	vif_packet& index_unpack = result.emplace_back();
	
	moby_model_index_header index_header;
	index_header.unknown_0 = 0xfe;
	index_header.texture_unpack_offset_quadwords = 0;
	index_header.unknown_2 = 0;
	index_header.unknown_3 = 0;
	index_unpack.data.resize(sizeof(index_header));
	std::memcpy(index_unpack.data.data(), &index_header, sizeof(index_header));
	
	for(moby_subsubmodel& subsubmodel : submodel.subsubmodels) {
		if(subsubmodel.texture) {
			index_unpack.data.push_back(0); // Push new texture.
		}
		for(uint8_t index : subsubmodel.indices) {
			index_unpack.data.push_back(index + 1); // Garbage but it'll give some kind of result.
		}
		index_unpack.data.push_back(1);
		index_unpack.data.push_back(1);
		index_unpack.data.push_back(1);
		index_unpack.data.push_back(0);
		while(index_unpack.data.size() % 4 != 0) {
			index_unpack.data.push_back(0);
		}
	}
	
	index_unpack.address = 1; // Fake address.
	index_unpack.code.interrupt = 0;
	index_unpack.code.cmd = (vif_cmd) 0b1100000; // UNPACK
	index_unpack.code.num = index_unpack.data.size() / 4;
	index_unpack.code.unpack.vnvl = vif_vnvl::V4_8;
	index_unpack.code.unpack.flg = vif_flg::USE_VIF1_TOPS;
	index_unpack.code.unpack.usn = vif_usn::SIGNED;
	index_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS;
	
	bool has_texture_unpack = false;
	for(moby_subsubmodel& subsubmodel : submodel.subsubmodels) {
		if(subsubmodel.texture) {
			has_texture_unpack = true;
		}
	}
	
	// TODO: Alignment?
	if(has_texture_unpack) {
		vif_packet& texture_unpack = result.emplace_back();
		
		for(moby_subsubmodel& subsubmodel : submodel.subsubmodels) {
			if(subsubmodel.texture) {
				// GIF A+D data. See EE User's Manual 7.3.2.
				uint8_t ad_data[0x40] = {
					0x27, 0xff, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0xa0, 0x41, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
				};
				*(uint32_t*) &ad_data[0x20] = subsubmodel.texture->texture_index;
				for(uint8_t byte : ad_data) {
					texture_unpack.data.push_back(byte);
				}
			}
		}
		
		texture_unpack.address = 2; // Fake address.
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (vif_cmd) 0b1100000; // UNPACK
		texture_unpack.code.num = texture_unpack.data.size() / 0x10;
		texture_unpack.code.unpack.vnvl = vif_vnvl::V4_32;
		texture_unpack.code.unpack.flg = vif_flg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = vif_usn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + result[1].data.size() / 4;
		
		// Write texture UNPACK offset in quadwords.
		result[1].data[1] = result[1].data.size() / 4;
	}
	
	return result;
}

void moby_model::write() {
	if(_type == moby_model_header_type::LEVEL) {
		throw std::runtime_error("moby_model::write not yet implemented for level models!");
	}
	
	// Skip past the submodel table.
	_backing.seek((submodels.size() + 1) * 0x10);
	
	uint32_t texture_unpack_offset_all = 0;
	
	std::vector<moby_submodel_entry> submodel_table;
	for(moby_submodel& submodel : submodels) {
		moby_submodel_entry& entry = submodel_table.emplace_back();
		
		_backing.pad(0x10, 0x0);
		entry.vif_list_offset = _backing.tell();
		
		uint32_t texture_unpack_offset = 0;
		
		for(std::size_t i = 0; i < submodel.vif_list.size(); i++) {
			vif_packet& packet = submodel.vif_list[i];
			if(packet.code.is_unpack()) {
				if(i == 2) { // Texture unpack.
					while(_backing.tell() % 0x10 != 0xc) {
						_backing.write<uint8_t>(0);
					}
					texture_unpack_offset = _backing.tell() - 0xc;
					texture_unpack_offset_all = texture_unpack_offset;
				} else {
					_backing.pad(0x4, 0);
				}
				_backing.write<uint32_t>(packet.code.encode_unpack());
			} else if(packet.code.cmd == vif_cmd::NOP) {
				_backing.pad(0x4, 0);
				_backing.write<uint32_t>(0);
			} else {
				throw std::runtime_error("vif_code has bad cmd (must be NOP or UNPACK).");
			}
			_backing.write_v(packet.data);
		}
		uint32_t end_of_vif_list_offset = _backing.tell();
		entry.vif_list_quadword_count = std::ceil((end_of_vif_list_offset - entry.vif_list_offset) / 16.f);
		if(texture_unpack_offset != 0) {
			entry.vif_list_texture_unpack_offset = std::ceil((end_of_vif_list_offset - texture_unpack_offset) / 16.f) - 1;
		} else {
			entry.vif_list_texture_unpack_offset = 0;
		}
		
		_backing.pad(0x10, 0x0);
		entry.vertex_offset = _backing.tell();
		
		moby_model_vertex_table_header vertex_header;
		vertex_header.unknown_0 = 0;
		vertex_header.vertex_count_2 = 0;
		vertex_header.vertex_count_4 = 0;
		vertex_header.main_vertex_count = submodel.vertices.size();
		vertex_header.vertex_count_8 = 0;
		vertex_header.transfer_vertex_count =
			vertex_header.vertex_count_2 +
			vertex_header.vertex_count_4 +
			vertex_header.main_vertex_count +
			vertex_header.vertex_count_8;
		vertex_header.vertex_table_offset = 0x10;
		vertex_header.unknown_e = 0;
		_backing.write(vertex_header);
		
		_backing.write_v(submodel.vertices);
		
		entry.vertex_data_quadword_count = std::ceil((_backing.tell() - entry.vertex_offset) / 16.f);
		
		// Not sure what these are for but these expressions seem to match
		// all the existing models.
		entry.unknown_d = (0xf + vertex_header.transfer_vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.transfer_vertex_count) / 4;
		
		entry.transfer_vertex_count = vertex_header.transfer_vertex_count;
	}
	
	uint32_t tex_application_offset = _backing.tell();
	
	// Write out bogus texture application table.
	uint8_t tex_application_table[12] = {
		0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	_backing.write_n((const char*) tex_application_table, 12);
	_backing.write<uint32_t>(texture_unpack_offset_all | 0x80000000); // pointer + terminator
	
	_backing.seek(0x10);
	_backing.write_v(submodel_table);
	
	moby_model_armor_header header;
	header.submodel_count_1 = submodels.size();
	header.submodel_count_2 = 0;
	header.submodel_count_3 = 0;
	header.submodel_count_1_plus_2 = header.submodel_count_1 + header.submodel_count_2;
	header.submodel_table_offset = 0x10;
	header.texture_applications_offset = tex_application_offset;
	_backing.seek(0x0);
	_backing.write(header);
}

std::string moby_model::resource_path() const {
	return _backing.resource_path();
}
