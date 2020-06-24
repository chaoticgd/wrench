/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "model.h"

#include "stream.h"

model::model()
	: _vertex_buffer(0),
	  _vertex_buffer_size(0) {}

model::model(model&& rhs)
	: _vertex_buffer(rhs._vertex_buffer),
	  _vertex_buffer_size(rhs._vertex_buffer_size) {
	rhs._vertex_buffer = 0;
	rhs._vertex_buffer_size = 0;
}

model::~model() {
	if(_vertex_buffer != 0) {
		glDeleteBuffers(1, &_vertex_buffer);
	}
}

void model::update() {
	glDeleteBuffers(1, &_vertex_buffer);
	
	std::vector<float> vertex_data;
	try {
		vertex_data = triangles();
	} catch(stream_error& e) {
		// We've failed to read the model data.
		_vertex_buffer = 0;
		_vertex_buffer_size = 0;
		return;
	}
	
	_vertex_buffer_size = vertex_data.size();
	glGenBuffers(1, &_vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		_vertex_buffer_size * sizeof(float),
		vertex_data.data(), GL_STATIC_DRAW);
}

GLuint model::vertex_buffer() const {
	return _vertex_buffer;
}

std::size_t model::vertex_buffer_size() const {
	return _vertex_buffer_size;
}
