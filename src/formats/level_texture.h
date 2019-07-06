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

#ifndef FORMATS_LEVEL_TEXTURE_H
#define FORMATS_LEVEL_TEXTURE_H

#include <array>
#include <vector>
#include <glm/glm.hpp>

#include "../stream.h"
#include "../texture.h"

class texture_provider;

class level_texture : public texture {
public:
	level_texture(stream* backing, uint32_t pixel_data_offset);

	glm::vec2 size() const override;
	void set_size(glm::vec2 size_) override;

	std::array<colour, 256> palette() const override;
	void set_palette(std::array<colour, 256> palette_) override;

	std::vector<uint8_t> pixel_data() const override;
	void set_pixel_data(std::vector<uint8_t> pixel_data_) override;

private:
	proxy_stream _pixel_data;

public:
	struct fmt {
		struct texture {
			uint32_t entry_size;
		};
	};
};

class level_texture_provider : public texture_provider {
public:
	struct fmt {
		struct header {
			uint32_t num_textures; // 0x0
			file_ptr<level_texture::fmt::texture> textures; // 0x4
			uint32_t unknown2;          // 0x8
			uint32_t unknown3;          // 0xc
			uint32_t unknown4;          // 0x10
			uint32_t unknown5;          // 0x14
			uint32_t unknown6;          // 0x18
			uint32_t unknown7;          // 0x1c
			uint32_t unknown8;          // 0x20
			uint32_t unknown9;          // 0x24
			uint32_t unknown10;         // 0x28
			uint32_t unknown11;         // 0x2c
			uint32_t unknown12;         // 0x30
			uint32_t unknown13;         // 0x34
			uint32_t unknown14;         // 0x38
			uint32_t unknown15;         // 0x3c
			uint32_t unknown16;         // 0x40
			uint32_t unknown17;         // 0x44
			uint32_t unknown18;         // 0x48
			uint32_t unknown19;         // 0x4c
			uint32_t unknown20;         // 0x50
			uint32_t unknown21;         // 0x54
			uint32_t unknown22;         // 0x58
			uint32_t unknown23;         // 0x5c
			uint32_t unknown24;         // 0x60
			uint32_t unknown25;         // 0x64
			uint32_t unknown26;         // 0x68
			uint32_t unknown27;         // 0x6c
			uint32_t unknown28;         // 0x70
			uint32_t unknown29;         // 0x74
			uint32_t unknown30;         // 0x78
			uint32_t unknown31;         // 0x7c
			uint32_t unknown32;         // 0x80
			uint32_t unknown1;      // 0x84
		};

		struct texture {
			uint32_t entry_size;
		};
	};

	level_texture_provider(stream* level_file, uint32_t secondary_header_offset);

	std::vector<texture*> textures() override;

private:
	proxy_stream _backing;
	std::unique_ptr<proxy_stream> _pixel_data_base;
	std::vector<std::unique_ptr<level_texture>> _textures;
};

#endif
