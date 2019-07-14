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
#include "../worker_logger.h"
#include "wad.h"

class texture_provider;

class texture_impl : public texture {
public:
	struct offsets {
		uint32_t palette, pixels, width, height;
	};

	texture_impl(stream* backing, offsets offsets_);

	vec2i size() const override;
	void set_size(vec2i size_) override;

	std::array<colour, 256> palette() const override;
	void set_palette(std::array<colour, 256> palette_) override;

	std::vector<uint8_t> pixel_data() const override;
	void set_pixel_data(std::vector<uint8_t> pixel_data_) override;
	
	virtual std::string palette_path() const override;
	virtual std::string pixel_data_path() const override;

private:
	proxy_stream _backing;
	offsets _offsets;
};

class texture_provider_impl : public texture_provider {
public:
	struct fmt {
		struct texture_entry {
			uint32_t unknown1;
			uint16_t width;
			uint16_t height;
			uint32_t unknown2;
			uint32_t pixel_data;
		};
	};

	texture_provider_impl(
		stream* backing,
		uint32_t table_offset,
		uint32_t data_offset,
		uint32_t num_textures,
		std::string display_name_);

	std::string display_name() const override;
	std::vector<texture*> textures() override;

private:
	std::vector<std::unique_ptr<texture_impl>> _textures;
	std::string _display_name;
};

class level_texture_provider : public texture_provider {
public:
	struct fmt {
		struct header {
			uint32_t num_textures; // 0x0
			file_ptr<texture_provider_impl::fmt::texture_entry> textures; // 0x4
			uint32_t unknown1;     // 0x8
			uint32_t unknown2;     // 0xc
			uint32_t unknown3;     // 0x10
			uint32_t unknown4;     // 0x14
			uint32_t unknown5;     // 0x18
			uint32_t unknown6;     // 0x1c
			uint32_t unknown7;     // 0x20
			uint32_t unknown8;     // 0x24
			uint32_t unknown9;     // 0x28
			uint32_t unknown10;    // 0x2c
			uint32_t unknown11;    // 0x30
			uint32_t unknown12;    // 0x34
			uint32_t unknown13;    // 0x38
			uint32_t unknown14;    // 0x3c
			uint32_t unknown15;    // 0x40
			uint32_t unknown16;    // 0x44
			uint32_t unknown17;    // 0x48
			uint32_t unknown18;    // 0x4c
			uint32_t unknown19;    // 0x50
			uint32_t unknown20;    // 0x54
			uint32_t unknown21;    // 0x58
			uint32_t unknown22;    // 0x5c
			uint32_t unknown23;    // 0x60
			uint32_t unknown24;    // 0x64
			uint32_t unknown25;    // 0x68
			uint32_t unknown26;    // 0x6c
			uint32_t unknown27;    // 0x70
			uint32_t unknown28;    // 0x74
			uint32_t unknown29;    // 0x78
			uint32_t unknown30;    // 0x7c
			uint32_t unknown31;    // 0x80
			uint32_t unknown32;    // 0x84
		};
	};

	level_texture_provider(stream* level_file, uint32_t secondary_header_offset, std::string display_name_);

	std::string display_name() const override;
	std::vector<texture*> textures() override;

private:
	std::optional<texture_provider_impl> _impl; 
};

class fip_scanner : public texture_provider {
public:
	fip_scanner(
		stream* backing,
		uint32_t offset,
		uint32_t size,
		std::string display_name,
		worker_logger& log);

	std::string display_name() const override;
	std::vector<texture*> textures() override;

private:
	proxy_stream _search_space;
	std::vector<std::unique_ptr<texture_impl>> _textures;
	std::string _display_name;
};

#endif
