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

#ifndef MODEL_H
#define MODEL_H

#include <array>
#include <vector>

#include "gl_includes.h"

# /*
#	Virtual base class representing a 3D model.
# */

class model {
public:
	model() {}
	model(const model& rhs) = delete;
	model(model&& rhs);
	virtual ~model();

	virtual std::vector<float> triangles() const = 0;
	virtual std::vector<float> colors() const = 0;
	
	// Read the model data, load it into a OpenGL buffer. Only call from main thread!
	void update();
	
	GLuint vertex_buffer() const;
	std::size_t vertex_buffer_size() const;
	GLuint vertex_color_buffer() const;
	std::size_t vertex_color_buffer_size() const;
	
private:
	GLuint _vertex_buffer = 0;
	std::size_t _vertex_buffer_size;
	GLuint _vertex_color_buffer = 0;
	std::size_t _vertex_color_buffer_size;
};

#endif
