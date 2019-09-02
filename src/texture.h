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

#ifndef TEXTURE_H
#define TEXTURE_H

#include <array>
#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>

#include "reflection/refolder.h"

# /*
#	Virtual base class that represents an indexed texture.
# */

struct colour {
	uint8_t r, g, b, a;
};

struct vec2i {
	int x, y;
};

#ifndef INSPECTABLE_DEF
#define INSPECTABLE_DEF
struct inspectable { virtual ~inspectable() = default; };
#endif

class texture : public inspectable {
public:
	virtual ~texture() = default;

	virtual vec2i size() const = 0;
	virtual void set_size(vec2i size_) = 0;

	virtual std::array<colour, 256> palette() const = 0;
	virtual void set_palette(std::array<colour, 256> palette_) = 0;

	virtual std::vector<uint8_t> pixel_data() const = 0;
	virtual void set_pixel_data(std::vector<uint8_t> pixel_data_) = 0;

	virtual std::string palette_path() const;
	virtual std::string pixel_data_path() const;

	template <typename... T>
	void reflect(T... callbacks) {
		rf::reflector r(this, callbacks...);
		r.visit_f("Width",           [=]() { return size().x; },          [](int) {});
		r.visit_f("Height",          [=]() { return size().y; },          [](int) {});
		r.visit_f("Palette Path",    [=]() { return palette_path(); },    [](std::string) {});
		r.visit_f("Pixel Data Path", [=]() { return pixel_data_path(); }, [](std::string) {});
	}
};

class texture_provider {
public:
	virtual ~texture_provider() = default;

	virtual std::string display_name() const = 0;
	virtual std::vector<texture*> textures() = 0;
	
	std::vector<const texture*> textures() const;
};

#endif
