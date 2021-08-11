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

#include "tcol.h"
#include "../util.h"

tcol::tcol(stream *backing, std::size_t base_offset)
		: _backing(backing, base_offset, 0), _base_offset(base_offset) {
	_backing.name = "TCol";

	int triangle_count = 0;
	glm::vec3 position_offset;

	// read header
	auto header = _backing.peek<tcol_header>(0);

	// get start of collision data
	_backing.seek(header.collision_offset);
	auto coord_z = _backing.read<int16_t>() * 4;
	auto zlist_size = _backing.read<uint16_t>();

	// update data
	data.coordinate_value = coord_z;
	data.list.resize(zlist_size);

	// 
	for (int z = 0; z < zlist_size; ++z)
	{
		// reseek
		_backing.seek(header.collision_offset + 4 + (z * 2));
		
		// grab offset
		auto ylist_offset = header.collision_offset + (_backing.read<uint16_t>() * 4);
		
		if (ylist_offset == header.collision_offset) {

		}
		else {
			// parse
			_backing.seek(ylist_offset);
			auto coord_y = _backing.read<int16_t>() * 4;
			auto ylist_size = _backing.read<uint16_t>();

			// update data
			data.list[z].coordinate_value = coord_y;
			data.list[z].list.resize(ylist_size);

			// 
			for (int y = 0; y < ylist_size; ++y)
			{
				// reseek
				_backing.seek(ylist_offset + 4 + (y * 4));

				// grab offset
				auto xlist_offset = header.collision_offset + _backing.read<uint32_t>();
				if (xlist_offset == header.collision_offset) {

				}
				else {
					// parse
					_backing.seek(xlist_offset);
					auto coord_x = _backing.read<int16_t>() * 4;
					auto xlist_size = _backing.read<uint16_t>();

					// update data
					data.list[z].list[y].coordinate_value = coord_x;
					data.list[z].list[y].list.resize(xlist_size);

					for (int x = 0; x < xlist_size; ++x)
					{
						// reseek
						_backing.seek(xlist_offset + 4 + (x * 4));

						auto xlist_entry = _backing.read<uint32_t>();
						auto data_offset = header.collision_offset + (xlist_entry >> 8);

						if (data_offset == header.collision_offset) {

						}
						else {
							// parse
							_backing.seek(data_offset);
							auto face_count = _backing.read<uint16_t>();
							auto vertex_count = _backing.read<uint8_t>();
							auto r_count = _backing.read<uint8_t>(); // quads

							// update data
							auto coldata = data.list[z].list[y].list[x];
							coldata.faces.resize(face_count);
							coldata.vertices.resize(vertex_count);

							// 
							for (int v = 0; v < vertex_count; ++v)
							{
								auto vertex_data = _backing.read<uint32_t>();
								coldata.vertices[v] = unpack_vertex(vertex_data);printf("v %f\n", coldata.vertices[v].x);static int k = 0; if(k++ > 20) exit(1);
							}

							for (int f = 0; f < face_count; ++f)
							{
								coldata.faces[f].v0 = _backing.read<uint8_t>();
								coldata.faces[f].v1 = _backing.read<uint8_t>();
								coldata.faces[f].v2 = _backing.read<uint8_t>();
								coldata.faces[f].collision_id = _backing.read<uint8_t>();
							}

							for (int r = 0; r < r_count; ++r)
							{
								coldata.faces[r].v3 = _backing.read<uint8_t>();
								coldata.faces[r].is_quad = true;
							}

							data.list[z].list[y].list[x] = coldata;
							triangle_count += face_count + r_count;
						}
					}
				}
			}
		}
	}

	// 
	position_offset.z = data.coordinate_value + 2;
	for (auto ystrip : data.list)
	{
		position_offset.y = ystrip.coordinate_value + 2;

		for (auto xstrip : ystrip.list)
		{
			position_offset.x = xstrip.coordinate_value + 2;

			for (auto coldata : xstrip.list)
			{
				for (auto face : coldata.faces)
				{
					push_face(position_offset, face, coldata);
				}

				position_offset.x += 4;
			}

			position_offset.y += 4;
		}

		position_offset.z += 4;
	}
}

void tcol::push_face(glm::vec3 offset, tcol::tcol_face face, tcol::tcol_data data) {
	auto v0 = data.vertices[face.v0] + offset;
	auto v1 = data.vertices[face.v1] + offset;
	auto v2 = data.vertices[face.v2] + offset;

	_tcol_triangles.push_back(v0.x);
	_tcol_triangles.push_back(v0.y);
	_tcol_triangles.push_back(v0.z);
	_tcol_triangles.push_back(v1.x);
	_tcol_triangles.push_back(v1.y);
	_tcol_triangles.push_back(v1.z);
	_tcol_triangles.push_back(v2.x);
	_tcol_triangles.push_back(v2.y);
	_tcol_triangles.push_back(v2.z);

	if (face.is_quad) {
		auto v3 = data.vertices[face.v3] + offset;

		_tcol_triangles.push_back(v0.x);
		_tcol_triangles.push_back(v0.y);
		_tcol_triangles.push_back(v0.z);
		_tcol_triangles.push_back(v2.x);
		_tcol_triangles.push_back(v2.y);
		_tcol_triangles.push_back(v2.z);
		_tcol_triangles.push_back(v3.x);
		_tcol_triangles.push_back(v3.y);
		_tcol_triangles.push_back(v3.z);
	}

	auto color = get_collision_color(face.collision_id);
	glm::vec3 normal = glm::normalize(glm::cross(v2 - v0, v1 - v0));
	color -= fabs((normal.x + normal.y + normal.z) / 10.f);
	int v_count = 3 + (face.is_quad ? 3 : 0);
	for (int i = 0; i < v_count; ++i) {
		_tcol_vertex_colors.push_back(color.x);
		_tcol_vertex_colors.push_back(color.y);
		_tcol_vertex_colors.push_back(color.z);
	}
}

glm::vec3 tcol::get_collision_color(uint8_t colId) {
	glm::vec3 v;

	// from https://github.com/RatchetModding/replanetizer/blob/ada7ca73418d7b01cc70eec58a41238986b84112/LibReplanetizer/Models/Collision.cs#L26
	// Colorize different types of collision without knowing what they are
	v.x = (uint8_t)((uint64_t)(colId & 0x3) << 6) / 255.0;
	v.y = (uint8_t)((uint64_t)(colId & 0xC) << 4) / 255.0;
	v.z = (uint8_t)(colId & 0xF0) / 255.0;

	return v;
}

glm::vec3 tcol::unpack_vertex(uint32_t vertex) {
	glm::vec3 v;

	auto x = (int32_t)(vertex << 22) >> 22;
	auto y = (int32_t)(vertex << 12) >> 22;
	auto z = (int32_t)(vertex << 0) >> 20;

	v.x = x / 16.0;
	v.y = y / 16.0;
	v.z = z / 64.0;

	return v;
}

std::vector<float> tcol::triangles() const {
	return _tcol_triangles;
}

std::vector<float> tcol::colors() const {
	return _tcol_vertex_colors;
}
