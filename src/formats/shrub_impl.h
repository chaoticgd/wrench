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

#ifndef FORMATS_SHRUB_IMPL_H
#define FORMATS_SHRUB_IMPL_H

# /*
#	A shrub stored using a stream. The member function wrap read/write calls.
# */

#include "vec3f.h"
#include "../level.h"
#include "../stream.h"

class shrub : public base_shrub {
public:
	struct fmt {
		packed_struct(shrub,
			uint32_t unknown1;  // 0x0
			uint32_t unknown2;  // 0x4
			uint32_t unknown3;  // 0x8
			uint32_t unknown4;  // 0xc
			uint32_t unknown5;  // 0x10
			uint32_t unknown6;  // 0x14
			uint32_t unknown7;  // 0x18
			uint32_t unknown8;  // 0x1c
			uint32_t unknown9;  // 0x20
			uint32_t unknown10; // 0x24
			uint32_t unknown11; // 0x28
			uint32_t unknown12; // 0x2c
			uint32_t unknown13; // 0x30
			uint32_t unknown14; // 0x34
			uint32_t unknown15; // 0x38
			uint32_t unknown16; // 0x3c
			vec3f position;     // 0x40
			uint32_t unknown20; // 0x4c
			uint32_t unknown21; // 0x50
			uint32_t unknown22; // 0x54
			uint32_t unknown23; // 0x58
			uint32_t unknown24; // 0x5c
			uint32_t unknown25; // 0x60
			uint32_t unknown26; // 0x64
			uint32_t unknown27; // 0x68
			uint32_t unknown28; // 0x6c
		)
	};

	shrub(stream* backing, std::size_t offset);

	std::size_t base() const override;

	std::string label() const;

	glm::vec3 position() const;
	void set_position(glm::vec3 rotation_);

	glm::vec3 rotation() const;
	void set_rotation(glm::vec3 rotation_);

private:
	proxy_stream _backing;
	std::size_t _base;
};

#endif
