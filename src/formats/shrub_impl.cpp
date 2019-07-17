#include "shrub_impl.h"

shrub_impl::shrub_impl(stream* backing, uint32_t offset)
	: _backing(backing, offset, -1) {}

std::string shrub_impl::label() const {
	return "s";
}

glm::vec3 shrub_impl::position() const {
	auto data = _backing.read_c<fmt::shrub>(0);
	return data.position.glm();
}

void shrub_impl::set_position(glm::vec3 rotation_) {
	auto data = _backing.read<fmt::shrub>(0);
	data.position = vec3f(rotation_);
	_backing.write<fmt::shrub>(0, data);
}

glm::vec3 shrub_impl::rotation() const {
	return glm::vec3(0, 0, 0); // Stub
}

void shrub_impl::set_rotation(glm::vec3 rotation_) {
	// Stub
}
