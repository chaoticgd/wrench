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

#include "model_utils.h"

#include <fstream>

#include "../stream.h"

float read_float(const std::string& line, std::size_t& index) {
	std::size_t float_end = index + line.substr(index).find(" ");
	float result = std::stof(line.substr(index, float_end));
	index = float_end + 1;
	return result;
}

std::vector<ply_vertex> read_ply_model(std::string path) {
	std::ifstream file(path);
	if(!file) {
		throw stream_io_error("Failed to open .PLY file!");
	}
	
	std::vector<ply_vertex> vertices;
	
	std::string line;
	std::size_t vertex_count = 0;
	bool reading_vertices = false;
	while(std::getline(file, line)) {
		if(line.find("element vertex ") == 0) {
			try {
				vertex_count = std::stoi(line.substr(strlen("element vertex ")));
			} catch(std::logic_error&) {
				throw stream_format_error("Failed to read vertex count from .PLY file.");
			}
		}
		
		if(line.find("end_header") == 0) {
			reading_vertices = true;
		} else if(reading_vertices && vertex_count > 0) {
			ply_vertex vertex;
			std::size_t pos = 0;
			try {
				vertex.x = read_float(line, pos);
				vertex.y = read_float(line, pos);
				vertex.z = read_float(line, pos);
				vertex.nx = read_float(line, pos);
				vertex.ny = read_float(line, pos);
				vertex.nz = read_float(line, pos);
				vertex.s = read_float(line, pos);
				vertex.t = read_float(line, pos);
			} catch(std::logic_error&) {
				throw stream_format_error("Failed to read vertices from .PLY file.");
			}
			vertices.push_back(vertex);
			
			vertex_count--;
		}
	}
	
	return vertices;
}
