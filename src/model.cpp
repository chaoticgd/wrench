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

model::model()
	: _vertex_buffer(0),
	  _vertex_buffer_size(0) {}

model::~model() {
	glDeleteBuffers(1, &_vertex_buffer);
}

GLuint model::vertex_buffer() const {
	
	// The vertex buffer cache is not considered a part of this object's logical
	// state. Only functions that modify the vertices should be non-const.
	model* this_ = const_cast<model*>(this);
	
	if(_vertex_buffer == 0) {
		std::vector<float> vertex_data = this_->triangles();

		this_->_vertex_buffer_size = vertex_data.size();
		
		glDeleteBuffers(1, &this_->_vertex_buffer);
		glGenBuffers(1, &this_->_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, this_->_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER,
			_vertex_buffer_size * sizeof(float),
			vertex_data.data(), GL_STATIC_DRAW);
	}
	
	return _vertex_buffer;
}

std::size_t model::vertex_buffer_size() const {
	return _vertex_buffer_size;
}
