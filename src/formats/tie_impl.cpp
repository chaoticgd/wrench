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

#include "tie_impl.h"

#include "../shapes.h"

tie_impl::tie_impl(stream* backing, std::size_t offset)
	: _backing(backing, offset, -1), _base(offset) {}

std::size_t tie_impl::base() const {
	return _base;
}

std::string tie_impl::label() const {
	return "t";
}

glm::vec3 tie_impl::position() const {
	auto data = _backing.peek<fmt::tie>(0);
	return data.position.glm();
}

void tie_impl::set_position(glm::vec3 rotation_) {
	auto data = _backing.read<fmt::tie>(0);
	data.position = vec3f(rotation_);
	_backing.write<fmt::tie>(0, data);
}

glm::vec3 tie_impl::rotation() const {
	return glm::vec3(0, 0, 0); // Stub
}

void tie_impl::set_rotation(glm::vec3 rotation_) {
	// Stub
}

const model& tie_impl::object_model() const {
	static cube_model c;
	return c;
}
